# BLOCK 2: THE DEVICE STRUCTURE WITH REAL BYTE COUNTS

## STEP 1: Count the bytes in each field

struct aesd_buffer_entry {
    const char *buffptr;        // 8 bytes (pointer on 64-bit)
    size_t size;                // 8 bytes (size_t on 64-bit)
};                              // TOTAL: 16 bytes per entry

struct aesd_circular_buffer {
    struct aesd_buffer_entry entry[10];  // 16 * 10 = 160 bytes
    uint8_t in_offs;                     // 1 byte
    uint8_t out_offs;                    // 1 byte  
    bool full;                           // 1 byte (padded to 4)
};                                       // TOTAL: 160 + 4 = 164 bytes

## STEP 2: Our device needs more fields

struct aesd_dev {
    // From aesd_circular_buffer.h
    struct aesd_circular_buffer buffer;     // 164 bytes
    
    // NEW: For partial write accumulation
    char *partial_buffer;                   // 8 bytes (pointer)
    size_t partial_size;                    // 8 bytes
    
    // NEW: For locking
    struct mutex lock;                      // ~40 bytes (kernel struct)
    
    // NEW: For character device
    struct cdev cdev;                       // ~64 bytes (kernel struct)
};                                          // TOTAL: ~284 bytes per device

## STEP 3: Memory layout visualization

ADDRESS         CONTENT
-------         -------
0x0000          buffer.entry[0].buffptr    (8 bytes)
0x0008          buffer.entry[0].size       (8 bytes)
0x0010          buffer.entry[1].buffptr    (8 bytes)
0x0018          buffer.entry[1].size       (8 bytes)
...             ...
0x00A0          buffer.entry[9].buffptr    (8 bytes)
0x00A8          buffer.entry[9].size       (8 bytes)
0x00B0          buffer.in_offs             (1 byte)
0x00B1          buffer.out_offs            (1 byte)
0x00B2          buffer.full                (1 byte)
0x00B4          [padding]                  (3 bytes)
0x00B8          partial_buffer             (8 bytes)
0x00C0          partial_size               (8 bytes)
0x00C8          lock                       (~40 bytes)
0x00F0          cdev                       (~64 bytes)

## STEP 4: The global device pointer

static struct aesd_dev *aesd_device = NULL;

WHY STATIC?
-----------
- Only one instance of this driver
- All file operations reference same device
- kmalloc(sizeof(struct aesd_dev)) happens in module_init

## STEP 5: The file_operations structure

static struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

BYTE COUNTS:
------------
struct file_operations = function pointers
    .read  = 8 bytes (function pointer)
    .write = 8 bytes (function pointer)
    .open  = 8 bytes (function pointer)
    .release = 8 bytes (function pointer)
    .owner = 8 bytes (module pointer)
    TOTAL: ~40 bytes

