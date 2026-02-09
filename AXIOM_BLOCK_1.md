# ASSIGNMENT 8: BLOCK 1 - THE FIRST AXIOMS

## AXIOM 0: What We Are Building

User Space Process          Kernel Space
------------------          ------------
    write()    --------->   aesdchar_write()
                             |
                             v
                         kmalloc(buffer)
                             |
                             v
                         circular_buffer_add()
                             |
                             v
                         [10 entries max]

    read()     --------->   aesdchar_read()
                             |
                             v
                         circular_buffer_find()
                             |
                             v
                         copy_to_user()

## AXIOM 1: The Data Flow

WRITE PATH (data enters kernel):
--------------------------------
write(fd, "hello\n", 6)     // user space
    |
    v
sys_write()                 // kernel entry
    |
    v
aesdchar_write()            // our driver
    |
    v
kmalloc(6)                  // allocate kernel memory
    |
    v
memcpy("hello\n", kmalloc_ptr, 6)
    |
    v
aesd_circular_buffer_add_entry()
    |
    v
entry[in_offs] = {ptr, 6}   // store in circular buffer

READ PATH (data exits kernel):
------------------------------
read(fd, buf, 100)          // user space
    |
    v
sys_read()                  // kernel entry
    |
    v
aesdchar_read()             // our driver
    |
    v
aesd_circular_buffer_find_entry_offset_for_fpos()
    |
    v
copy_to_user(buf, entry_ptr, size)  // kernel -> user

## AXIOM 2: The Circular Buffer State Machine

Initial State:
--------------
in_offs = 0
out_offs = 0
full = false
entry[0..9] = {NULL, 0}

After 1 write ("hello\n"):
-------------------------
in_offs = 1
out_offs = 0
full = false
entry[0] = {0xptr1, 6}

After 10 writes:
----------------
in_offs = 0
out_offs = 0
full = true
entry[0..9] = filled

After 11th write:
-----------------
in_offs = 1
out_offs = 1
full = true
entry[0] = NEW DATA (old overwritten)
entry[1..9] = old data

## AXIOM 3: Memory Ownership

WHO OWNS WHAT:
--------------
User Space:    malloc() -> free()
Kernel Space:  kmalloc() -> kfree()

TRANSFER:
---------
write(): user buffer -> kmalloc'd buffer
read():  kmalloc'd buffer -> user buffer

CRITICAL RULE:
--------------
Once kmalloc'd buffer is stored in circular buffer:
- User CANNOT access it directly
- Only kernel can touch it
- kfree() happens ONLY when:
  1. Buffer is full and entry gets overwritten
  2. Driver is unloaded (cleanup)

## AXIOM 4: The Write Command Fragment Problem

SCENARIO: Multiple writes without newline
-----------------------------------------
write(fd, "hel", 3)         // partial command
write(fd, "lo\n", 3)         // completion

REQUIREMENT:
------------
Must NOT add to circular buffer until '\n' seen

SOLUTION:
---------
struct aesd_device {
    char *partial_buffer;       // accumulates fragments
    size_t partial_size;        // current accumulated size
    struct mutex lock;          // protects all of this
}

STATE TRANSITION:
-----------------
write "hel":
    partial_buffer = kmalloc(3) = "hel"
    partial_size = 3
    NO circular buffer add

write "lo\n":
    partial_buffer = krealloc(3+3) = "hello\n"
    memcpy "lo\n" to offset 3
    NOW add to circular buffer
    partial_buffer = NULL
    partial_size = 0

## AXIOM 5: Locking Requirements

WHAT NEEDS PROTECTION:
----------------------
1. circular_buffer.in_offs
2. circular_buffer.out_offs
3. circular_buffer.full
4. circular_buffer.entry[] array
5. partial_buffer pointer
6. partial_size

LOCKING RULE:
-------------
Every write() and read() must:
    mutex_lock(&dev->lock)
    // ... access shared state ...
    mutex_unlock(&dev->lock)

WHY MUTEX (not spinlock):
-------------------------
- kmalloc() can sleep
- krealloc() can sleep
- copy_to_user() can sleep
- Therefore: mutex, not spinlock

