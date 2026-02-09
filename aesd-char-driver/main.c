/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Rahul"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */

    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

/**
 * Helper: Return 0 if f_pos is beyond all data (EOF)
 */
static ssize_t check_eof(struct aesd_dev *dev, loff_t f_pos)
{
    size_t total_size = 0;
    uint8_t index;
    struct aesd_buffer_entry *entry;

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buffer, index) {
        if (entry->buffptr) {
            total_size += entry->size;
        }
    }

    if (f_pos >= total_size) {
        return 0;  // EOF
    }
    return -1;  // Not EOF
}

/**
 * Helper: Find entry and calculate bytes available to read
 */
static ssize_t find_read_position(struct aesd_dev *dev, loff_t f_pos,
                                 struct aesd_buffer_entry **entry_rtn, size_t *entry_offset_rtn)
{
    if (!dev) return -EINVAL;
    if (!entry_rtn || !entry_offset_rtn) return -EINVAL;

    *entry_rtn = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, f_pos, entry_offset_rtn);
    if (!*entry_rtn) {
        return -EINVAL;
    }

    return 0;
}

/**
 * Helper: Copy data to user space and update file position
 */
static ssize_t copy_to_user_and_update(struct aesd_dev *dev, struct aesd_buffer_entry *entry,
                                      size_t entry_offset, char __user *buf, size_t count, loff_t *f_pos)
{
    if (!dev || !entry || !buf || !f_pos) return -EINVAL;

    size_t bytes_available = entry->size - entry_offset; // Math: entry->size - entry_offset
    size_t bytes_to_copy = min(count, bytes_available);   // Math: min(count, bytes_available)

    if (copy_to_user(buf, entry->buffptr + entry_offset, bytes_to_copy) != 0) {
        return -EFAULT;
    }

    *f_pos += bytes_to_copy; // Math: bytes_to_copy
    return bytes_to_copy;    // Math: bytes_to_copy
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry = NULL;
    size_t entry_offset = 0;
    ssize_t retval = 0;

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }

    // Check EOF condition
    if (check_eof(dev, *f_pos) == 0) {
        retval = 0;
        goto out;
    }

    // Find read position using helper
    if (find_read_position(dev, *f_pos, &entry, &entry_offset) != 0) {
        retval = 0;
        goto out;
    }

    // Copy data to user using helper
    retval = copy_to_user_and_update(dev, entry, entry_offset, buf, count, f_pos);

out:
    mutex_unlock(&dev->lock);
    return retval;
}

/**
 * Helper: Save data as partial write when no existing partial and no newline
 */
static int save_as_partial(struct aesd_dev *dev, char *data, size_t count)
{
    // TODO: Allocate dev->partial_write, copy data, set dev->partial_size

    dev->partial_write = kmalloc(count, GFP_KERNEL);
    memcpy(dev->partial_write, data, count);
    dev->partial_size = count;
    return 0;
}

/**
 * Helper: Process complete entry when no existing partial but has newline
 */
static int process_complete_entry(struct aesd_dev *dev, char *data, size_t complete_size, size_t remaining)
{
    if (!dev) return -EINVAL;
    if (!data) return -EINVAL;
    if (complete_size == 0) return 0;

    if (dev->buffer.full) {
        struct aesd_buffer_entry *old_entry = &dev->buffer.entry[dev->buffer.in_offs];
        if (old_entry->buffptr) {
            kfree(old_entry->buffptr);
        }
    }
    
    struct aesd_buffer_entry entry = {
        .buffptr = data,
        .size = complete_size
    };
    aesd_circular_buffer_add_entry(&dev->buffer, &entry);

    if (remaining > 0) {
        dev->partial_write = kmalloc(remaining, GFP_KERNEL);
        if (!dev->partial_write) {
            return -ENOMEM;
        }
        memcpy(dev->partial_write, data + complete_size, remaining);
        dev->partial_size = remaining;
    }

    return 0;
}

/**
 * Helper: Extend existing partial write when no newline
 */
static int extend_partial(struct aesd_dev *dev, char *data, size_t count)
{
    // TODO: Realloc dev->partial_write, append new data
    if ( !dev) {
        return -EINVAL;
    }
    if (!data){
        return -EINVAL;
    }
    if ( count == 0 )
    {
        return 0;
    }

    char *temp = krealloc(dev->partial_write, dev->partial_size + count, GFP_KERNEL);
    if (!temp) {
        return -ENOMEM;
    }
    dev->partial_write = temp;
    memcpy(dev->partial_write + dev->partial_size, data, count);
    dev->partial_size += count;
    return 0;
}

/**
 * Helper: Merge partial with new data and complete entry when has newline
 */
static int merge_and_complete(struct aesd_dev *dev, char *data, size_t count, char *newline_pos)
{
    if (!dev) return -EINVAL;
    if (!data) return -EINVAL;
    if (!newline_pos) return -EINVAL;
    if (!dev->partial_write) return -EINVAL;

size_t bytes_to_newline = newline_pos - data + 1;
    size_t complete_size = dev->partial_size + bytes_to_newline;

    char *temp = krealloc(dev->partial_write, complete_size, GFP_KERNEL);
    if (!temp) {
        return -ENOMEM;
    }
    dev->partial_write = temp;
    memcpy(dev->partial_write + dev->partial_size, data, bytes_to_newline);

    if (dev->buffer.full) {
        struct aesd_buffer_entry *old_entry = &dev->buffer.entry[dev->buffer.in_offs];
        if (old_entry->buffptr) {
            kfree(old_entry->buffptr);
        }
    }
    
    struct aesd_buffer_entry entry = {
        .buffptr = dev->partial_write,
        .size = complete_size
    };
    aesd_circular_buffer_add_entry(&dev->buffer, &entry);

    size_t remaining = count - bytes_to_newline;
    if (remaining > 0) {
        dev->partial_write = kmalloc(remaining, GFP_KERNEL);
        if (!dev->partial_write) {
            return -ENOMEM;
        }
        memcpy(dev->partial_write, data + bytes_to_newline, remaining);
        dev->partial_size = remaining;
    } else {
        dev->partial_write = NULL;
        dev->partial_size = 0;
    }

    return 0;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    char *write_buf = NULL;
    char *newline_pos = NULL;
    ssize_t retval = 0;

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }

    write_buf = kmalloc(count, GFP_KERNEL);
    if (!write_buf) {
        retval = -ENOMEM;
        goto out;
    }

    if (copy_from_user(write_buf, buf, count) != 0) {
        retval = -EFAULT;
        goto out_free;
    }

    newline_pos = memchr(write_buf, '\n', count);

    if (newline_pos) {
        size_t bytes_to_newline = newline_pos - write_buf + 1;  // Include \n
        size_t remaining_bytes = count - bytes_to_newline;       // After \n

        if (!dev->partial_write) {
            // No existing partial, has newline → process complete entry
            retval = process_complete_entry(dev, write_buf, bytes_to_newline, remaining_bytes);
        } else {
            // Has existing partial, has newline → merge and complete
            retval = merge_and_complete(dev, write_buf, count, newline_pos);
        }
    } else {
        // No newline found
        if (!dev->partial_write) {
            // No existing partial, no newline → save as partial
            retval = save_as_partial(dev, write_buf, count);
        } else {
            // Has existing partial, no newline → extend partial
            retval = extend_partial(dev, write_buf, count);
        }
    }

    if (retval == 0) {
        retval = count;  // Success: return bytes written
    }
    // TODO: Choose appropriate helper based on partial_write existence and newline presence
    // - If no dev->partial_write and no newline: call save_as_partial()
    // - If no dev->partial_write and has newline: call process_complete_entry()
    // - If has dev->partial_write and no newline: call extend_partial()
    // - If has dev->partial_write and has newline: call merge_and_complete()

    // TODO: Set retval to count on success

out_free:
    kfree(write_buf);
out:
    mutex_unlock(&dev->lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    aesd_circular_buffer_init(&aesd_device.buffer);
    mutex_init(&aesd_device.lock);
    aesd_device.partial_size = 0;
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    uint8_t index;
    struct aesd_buffer_entry *entry;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) {
        if (entry->buffptr) {
            kfree(entry->buffptr);
        }
    }

    if (aesd_device.partial_write) {
        kfree(aesd_device.partial_write);
    }

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
