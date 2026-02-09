/*
 * BLOCK 5: STARTER CODE - aesdchar.c
 * 
 * AXIOM: This file is the character device driver implementation
 * AXIOM: It creates /dev/aesdchar when loaded
 * AXIOM: It uses the circular buffer from assignment 7
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

/* AXIOM: Include the circular buffer header */
#include "aesd-circular-buffer.h"

/* AXIOM: Device constants */
#define AESD_MAJOR 0        /* 0 = dynamic major number allocation */
#define AESD_MINOR 0        /* Starting minor number */
#define AESD_DEV_NAME "aesdchar"

/* AXIOM: Global device structure pointer */
static struct aesd_dev *aesd_device = NULL;

/*
 * AXIOM: Per-device structure
 * BYTE COUNT: sizeof(struct aesd_dev) = ~284 bytes
 */
struct aesd_dev {
    struct aesd_circular_buffer buffer;     /* 164 bytes - the circular buffer */
    char *partial_buffer;                    /* 8 bytes - accumulates partial writes */
    size_t partial_size;                     /* 8 bytes - size of partial buffer */
    struct mutex lock;                       /* ~40 bytes - protects everything */
    struct cdev cdev;                        /* ~64 bytes - character device */
};

/*
 * AXIOM: File operations prototypes
 * These match the signatures required by struct file_operations
 */
static int aesd_open(struct inode *inode, struct file *filp);
static int aesd_release(struct inode *inode, struct file *filp);
static ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

/*
 * AXIOM: File operations table
 * This connects system calls to our functions
 */
static struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

/*
 * TODO BLOCK 1: aesd_open
 * CALLED WHEN: user calls open("/dev/aesdchar", ...)
 * WHAT TO DO: Set up file private data if needed
 */
static int aesd_open(struct inode *inode, struct file *filp)
{
    /* AXIOM: inode->i_cdev points to our cdev structure */
    /* AXIOM: container_of() can get us back to struct aesd_dev */
    
    /* YOUR CODE HERE */
    
    return 0; /* Success */
}

/*
 * TODO BLOCK 2: aesd_release
 * CALLED WHEN: user calls close() or process exits
 * WHAT TO DO: Clean up any per-file resources
 */
static int aesd_release(struct inode *inode, struct file *filp)
{
    /* YOUR CODE HERE */
    
    return 0; /* Success */
}

/*
 * TODO BLOCK 3: aesd_read
 * CALLED WHEN: user calls read()
 * PARAMETERS:
 *   - filp: file pointer (contains f_pos)
 *   - buf: user space buffer (destination)
 *   - count: max bytes to copy
 *   - f_pos: current file position (updated by us)
 * RETURNS: bytes read, 0 for EOF, negative for error
 */
static ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    
    /* AXIOM: Need to get device structure from filp */
    /* AXIOM: Must lock mutex before accessing buffer */
    /* AXIOM: Use aesd_circular_buffer_find_entry_offset_for_fpos() */
    /* AXIOM: Use copy_to_user() to transfer data */
    
    /* YOUR CODE HERE - see BLOCK_4_READ_ANALYSIS.md for scenarios */
    
    return retval;
}

/*
 * TODO BLOCK 4: aesd_write
 * CALLED WHEN: user calls write()
 * PARAMETERS:
 *   - filp: file pointer
 *   - buf: user space buffer (source)
 *   - count: bytes to write
 *   - f_pos: ignored for this assignment
 * RETURNS: bytes written, negative for error
 */
static ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    
    /* AXIOM: Need to get device structure from filp */
    /* AXIOM: Must lock mutex before accessing buffer */
    /* AXIOM: Use copy_from_user() to get data from user */
    /* AXIOM: Search for '\n' in the data */
    /* AXIOM: If '\n' found: combine with partial_buffer, add to circular buffer */
    /* AXIOM: If '\n' not found: append to partial_buffer */
    /* AXIOM: If buffer full when adding: kfree the entry being overwritten */
    
    /* YOUR CODE HERE - see BLOCK_3_WRITE_ANALYSIS.md for scenarios */
    
    return retval;
}

/*
 * TODO BLOCK 5: aesd_init_module
 * CALLED WHEN: insmod or modprobe loads this module
 * WHAT TO DO:
 *   1. Register character device major number
 *   2. Allocate aesd_device structure
 *   3. Initialize mutex
 *   4. Initialize circular buffer
 *   5. Initialize cdev and add it to kernel
 */
static int __init aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    
    /* AXIOM: alloc_chrdev_region() gets us a major number */
    /* AXIOM: kmalloc() allocates our device structure */
    /* AXIOM: mutex_init() initializes the lock */
    /* AXIOM: aesd_circular_buffer_init() initializes buffer */
    /* AXIOM: cdev_init() and cdev_add() register the device */
    
    /* YOUR CODE HERE */
    
    return 0; /* Success */
}

/*
 * TODO BLOCK 6: aesd_cleanup_module
 * CALLED WHEN: rmmod unloads this module
 * WHAT TO DO:
 *   1. Remove cdev from kernel
 *   2. Free all circular buffer entries (kfree each buffptr)
 *   3. Free partial_buffer if any
 *   4. Unregister character device
 *   5. Free device structure
 */
static void __exit aesd_cleanup_module(void)
{
    /* AXIOM: cdev_del() removes the device */
    /* AXIOM: unregister_chrdev_region() frees major number */
    /* AXIOM: Use AESD_CIRCULAR_BUFFER_FOREACH to free all entries */
    /* AXIOM: kfree() for partial_buffer and device structure */
    
    /* YOUR CODE HERE */
}

/* AXIOM: Module entry/exit points */
module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

/* AXIOM: Module metadata */
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("AESD Character Driver");
MODULE_LICENSE("Dual BSD/GPL");

