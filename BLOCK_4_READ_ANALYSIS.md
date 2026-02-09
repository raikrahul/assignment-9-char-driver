# BLOCK 4: READ OPERATION ANALYSIS

## THE READ CONTRACT

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

PARAMETERS:
-----------
- filp: file pointer (contains f_pos - current read position)
- buf: user space buffer (virtual address in user process)
- count: max bytes to read (user says "give me at most this many")
- f_pos: current file position (byte offset in logical concatenated buffer)

RETURN VALUE:
-------------
- > 0: number of bytes actually read
- = 0: EOF (no more data)
- < 0: error code (-EFAULT, -EINVAL, etc.)

## SCENARIO 1: Simple read from beginning

STATE:
------
entry[0] = {0x1000, 6}  // "hello\n"
entry[1] = {0x2000, 6}  // "world\n"
*f_pos = 0
count = 100

STEP BY STEP:
-------------
1. Call aesd_circular_buffer_find_entry_offset_for_fpos(buffer, 0, &entry_offset)
   - char_offset = 0
   - Check entry[0]: size = 6
   - 0 < 6, so entry found at entry[0]
   - entry_offset = 0
   - Returns &entry[0]

2. Calculate bytes available in this entry
   - entry->size = 6
   - entry_offset = 0
   - available = 6 - 0 = 6 bytes

3. Calculate bytes to actually copy
   - available = 6
   - count = 100
   - to_copy = min(6, 100) = 6

4. Copy to user
   - copy_to_user(buf, 0x1000 + 0, 6)
   - Copies "hello\n" to user buffer

5. Update f_pos
   - *f_pos = 0 + 6 = 6

6. Return 6

## SCENARIO 2: Read across entry boundary

STATE:
------
entry[0] = {0x1000, 6}  // "hello\n"
entry[1] = {0x2000, 6}  // "world\n"
*f_pos = 4 (already read "hell")
count = 10

STEP BY STEP:
-------------
1. Find entry for f_pos = 4
   - entry[0]: size = 6
   - 4 < 6, so entry found at entry[0]
   - entry_offset = 4
   - Returns &entry[0]

2. Calculate available in entry[0]
   - entry->size = 6
   - entry_offset = 4
   - available = 6 - 4 = 2 bytes ("o\n")

3. Calculate to_copy
   - available = 2
   - count = 10
   - to_copy = min(2, 10) = 2

4. Copy to user
   - copy_to_user(buf, 0x1000 + 4, 2)
   - Copies "o\n" to user buffer offset 0

5. Update f_pos
   - *f_pos = 4 + 2 = 6

6. Check if we can read more (count remaining = 8)
   - Find entry for f_pos = 6
   - entry[0]: size = 6, 6 >= 6, skip
   - entry[1]: size = 6, 6 < 12, found at entry[1]
   - entry_offset = 6 - 6 = 0

7. Calculate available in entry[1]
   - entry->size = 6
   - entry_offset = 0
   - available = 6 - 0 = 6 bytes

8. Calculate to_copy
   - remaining count = 10 - 2 = 8
   - available = 6
   - to_copy = min(6, 8) = 6

9. Copy to user
   - copy_to_user(buf + 2, 0x2000 + 0, 6)
   - Copies "world\n" to user buffer offset 2

10. Update f_pos
    - *f_pos = 6 + 6 = 12

11. Return 8 (2 + 6)

## SCENARIO 3: Read with limited count

STATE:
------
entry[0] = {0x1000, 100}  // 100 bytes
*f_pos = 0
count = 50

STEP BY STEP:
-------------
1. Find entry for f_pos = 0
   - Returns &entry[0], entry_offset = 0

2. Calculate available
   - entry->size = 100
   - entry_offset = 0
   - available = 100 bytes

3. Calculate to_copy
   - available = 100
   - count = 50
   - to_copy = min(100, 50) = 50

4. Copy to user
   - copy_to_user(buf, 0x1000, 50)

5. Update f_pos
   - *f_pos = 50

6. Return 50

NEXT READ:
----------
*f_pos = 50
count = 100

1. Find entry for f_pos = 50
   - entry[0]: size = 100, 50 < 100, found
   - entry_offset = 50

2. Available = 100 - 50 = 50 bytes

3. to_copy = min(50, 100) = 50

4. Copy remaining 50 bytes

