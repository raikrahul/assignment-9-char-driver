

---

## 13. Multi-Device Driver Flow Complete Analysis

### The Problem: One Driver, Multiple Physical Devices

**Scenario:** USB hub with 4 identical serial ports. All use same driver code, but each port has:
- Different hardware address (I/O port, memory mapped registers)
- Different IRQ line
- Independent data buffers
- Separate state ( baud rate, flow control, open/close status)

**Solution:** Array of device structures, indexed by minor number.

### Data Structures

**Driver's Private Array:**
```c
struct serial_port {
    void __iomem *base_addr;    // Hardware register base
    int irq;                     // Interrupt number  
    struct circ_buf tx_buf;      // Transmit buffer
    struct circ_buf rx_buf;      // Receive buffer
    struct tty_struct *tty;      // TTY structure
    unsigned int flags;          // Open/close flags
    struct mutex port_mutex;     // Per-port lock
};

// Global array - one entry per physical port
struct serial_port ports[4];  // 4 serial ports
```

**Registration:**
```c
alloc_chrdev_region(&dev, 0, 4, "serial");
// Registers major=240, minors 0, 1, 2, 3
```

**Device Node Creation:**
```bash
mknod /dev/ttyS0 c 240 0   # minor 0 → ports[0]
mknod /dev/ttyS1 c 240 1   # minor 1 → ports[1]  
mknod /dev/ttyS2 c 240 2   # minor 2 → ports[2]
mknod /dev/ttyS3 c 240 3   # minor 3 → ports[3]
```

### Complete Flow: USB Device Insertion to Open

**Hardware Detection Phase:**

USB device plugged in triggers kernel USB stack. Bus driver enumerates device, reads descriptors, identifies as serial port. USB core matches against registered drivers, finds serial driver. Driver's probe function called with USB device structure.

Driver allocates port structure, finds free slot in ports[] array. Stores hardware info (I/O addresses, IRQ) in ports[i]. Calls device_create() to create /dev/ttySX node with MKDEV(major, i). Node appears in filesystem with i_rdev = MKDEV(240, i).

**User Open Phase:**

User application calls open("/dev/ttyS2", O_RDWR). System call enters kernel VFS layer. Path resolution walks filesystem, finds inode for /dev/ttyS2. Inode contains i_rdev = MKDEV(240, 2) = 0x0F000002.

VFS calls chrdev_open() with inode and new file structure. chrdev_open checks inode->i_cdev. First open: NULL, needs lookup. Calls kobj_lookup(cdev_map, 0x0F000002).

Hash calculation: MAJOR(0x0F000002) % 255 = 240 % 255 = 240. Walks cdev_map->probes[240] linked list. Finds probe with dev=0x0F000000, range=4. Checks: 0x0F000000 <= 0x0F000002 < 0x0F000004? Yes.

Returns probe->kobj. chrdev_open does container_of to get cdev pointer. Binds: inode->i_cdev = cdev. Calls filp->f_op->open() which is serial_open().

**Driver Open Function:**

```c
static int serial_open(struct inode *inode, struct file *filp)
{
    // Extract minor from device number
    unsigned int minor = MINOR(inode->i_rdev);
    // For /dev/ttyS2: minor = 2
    
    // Validate range
    if (minor >= NUM_PORTS)
        return -ENODEV;
    
    // INDEX INTO ARRAY - This is the key step!
    struct serial_port *port = &ports[minor];
    // minor=2 → &ports[2]
    
    // Check if port initialized
    if (!port->base_addr)
        return -ENODEV;
    
    // Lock port for exclusive access
    mutex_lock(&port->port_mutex);
    
    // Initialize port hardware if first open
    if (!port->flags & PORT_OPEN) {
        request_irq(port->irq, serial_irq, ...);
        enable_irq(port->irq);
        port->flags |= PORT_OPEN;
    }
    
    // Store for read/write
    filp->private_data = port;
    // filp now knows which physical port
    
    mutex_unlock(&port->port_mutex);
    return 0;
}
```

**Read/Write Operations:**

```c
static ssize_t serial_write(struct file *filp, const char __user *buf,
                           size_t count, loff_t *ppos)
{
    // Get port from file private data
    struct serial_port *port = filp->private_data;
    // Points to ports[0], ports[1], ports[2], or ports[3]
    // Depending on which /dev/ttySX was opened
    
    // Write to hardware registers specific to this port
    outb(data, port->base_addr + UART_TX);
    
    // Use buffers specific to this port
    circ_buf_write(&port->tx_buf, buf, count);
}
```

### Why This Design Works

**One cdev Structure:**
```c
struct cdev serial_cdev;
// Single cdev for ALL ports
// Same file_operations for all
// Driver distinguishes by minor → array index
```

**Per-Port State:**
```
ports[0]: base=0x3F8, irq=4, open_count=1, tx_buf={...}
ports[1]: base=0x2F8, irq=3, open_count=0, tx_buf={...}
ports[2]: base=0x3E8, irq=5, open_count=2, tx_buf={...}
ports[3]: base=0x2E8, irq=7, open_count=0, tx_buf={...}
```

**Multiple Opens:**
```
Process A: open("/dev/ttyS0") → minor=0 → port=&ports[0] → fd=3
Process B: open("/dev/ttyS2") → minor=2 → port=&ports[2] → fd=4
Process C: open("/dev/ttyS0") → minor=0 → port=&ports[0] → fd=5

ports[0].open_count = 2 (A and C using same port)
ports[2].open_count = 1 (B using different port)
```

### Memory Layout Visualization

```
Kernel Space:
├─ cdev_map->probes[240] → probe {
│     dev = MKDEV(240, 0)
│     range = 4
│     data = &serial_cdev
│   }
│
├─ serial_cdev {
│     ops = &serial_fops
│     list = { ... }
│     kobj = { ... }
│   }
│
├─ ports[0] @ 0xFFFFA1000000 {
│     base_addr = 0x3F8
│     irq = 4
│     flags = PORT_OPEN
│     tx_buf = { ... }
│   }
│
├─ ports[1] @ 0xFFFFA1000400 {
│     base_addr = 0x2F8
│     irq = 3
│     flags = 0
│     tx_buf = { ... }
│   }
│
├─ ports[2] @ 0xFFFFA1000800 {
│     base_addr = 0x3E8
│     irq = 5
│     flags = PORT_OPEN
│     tx_buf = { ... }
│   }
│
└─ ports[3] @ 0xFFFFA1000C00 {
      base_addr = 0x2E8
      irq = 7
      flags = 0
      tx_buf = { ... }
    }

Filesystem:
/dev/ttyS0 → inode { i_rdev = MKDEV(240, 0) }
/dev/ttyS1 → inode { i_rdev = MKDEV(240, 1) }
/dev/ttyS2 → inode { i_rdev = MKDEV(240, 2) }
/dev/ttyS3 → inode { i_rdev = MKDEV(240, 3) }
```

### The Minor Number Flow

```
User Space                    Kernel Space
----------                    ------------

cat /dev/ttyS2
      │
      │  open("/dev/ttyS2")
      ↓
                         VFS: find inode
                         inode->i_rdev = 0x0F000002
                         (major=240, minor=2)
                              │
                              ↓
                         chrdev_open()
                              │
                              ↓
                         kobj_lookup(0x0F000002)
                         Returns &serial_cdev
                              │
                              ↓
                         serial_open(inode, filp)
                         {
                           minor = MINOR(0x0F000002) = 2
                           port = &ports[2]
                           filp->private_data = port
                         }
                              │
                              ↓
                         Return fd to user
      │
      │  fd = 3
      ↓
                         Later: write(fd, buf, len)
                              │
                              ↓
                         serial_write(filp, ...)
                         {
                           port = filp->private_data
                                = &ports[2]
                           Write to port 2 hardware
                         }
```

### Summary: When Minor Numbers Matter

**Single-Device Drivers (Your Assignment):**
- Register: alloc_chrdev_region(&dev, 0, 1, "name")
- Minor: Always 0
- Open: Ignore minor, use container_of directly
- Complexity: Low

**Multi-Device Drivers:**
- Register: alloc_chrdev_region(&dev, 0, N, "name")
- Minor: 0, 1, 2, ..., N-1
- Open: Extract minor, index into array
- Complexity: Medium (array management)

**Multi-Device with Hotplug (USB, PCI):**
- Register: alloc_chrdev_region(&dev, 0, MAX, "name")
- Minor: Dynamically allocated from free slots
- Open: Extract minor, index into sparse array
- Complexity: High (dynamic slot management)

### Key Insight

**Minor number is the bridge between:**
- User space device node (/dev/ttyS2)
- Kernel driver array index (ports[2])
- Physical hardware instance (COM3 port)

**Without minor numbers:** One driver could only handle one device.
**With minor numbers:** One driver handles unlimited devices, distinguished by array indexing.
