# BLOCK 3: WRITE OPERATION ANALYSIS

## SCENARIO 1: Single complete write

INPUT: write(fd, "hello\n", 6)

STEP BY STEP:
-------------
1. copy_from_user() copies 6 bytes from user to kernel temp buffer
   - user buffer at 0x7fff1234: ['h','e','l','l','o','\n']
   - kernel temp buffer: kmalloc(6) returns 0xffff0000
   - copy_from_user(0xffff0000, 0x7fff1234, 6) = 0 (success)

2. Search for '\n' in buffer
   - buffer[0] = 'h' != '\n'
   - buffer[1] = 'e' != '\n'
   - buffer[2] = 'l' != '\n'
   - buffer[3] = 'l' != '\n'
   - buffer[4] = 'o' != '\n'
   - buffer[5] = '\n' == '\n' FOUND at offset 5

3. Since '\n' found, add to circular buffer
   - buffer.entry[0] = {0xffff0000, 6}
   - buffer.in_offs = 1
   - buffer.full = false

4. partial_buffer = NULL (nothing pending)

5. Return 6 (bytes written)

## SCENARIO 2: Fragmented write

INPUT 1: write(fd, "hel", 3)

STEP BY STEP:
-------------
1. copy_from_user() copies 3 bytes
   - kmalloc(3) returns 0xffff1000
   - copy_from_user(0xffff1000, user_buf, 3) = 0

2. Search for '\n' in 3 bytes
   - buffer[0] = 'h' != '\n'
   - buffer[1] = 'e' != '\n'
   - buffer[2] = 'l' != '\n'
   - NOT FOUND

3. Since NO '\n', store in partial_buffer
   - dev->partial_buffer = 0xffff1000 ("hel")
   - dev->partial_size = 3

4. Return 3 (bytes written)

INPUT 2: write(fd, "lo\nworld", 8)

STEP BY STEP:
-------------
1. copy_from_user() copies 8 bytes
   - kmalloc(8) returns 0xffff2000 (temp)
   - copy_from_user(0xffff2000, user_buf, 8) = 0

2. Search for '\n' in 8 bytes
   - buffer[0] = 'l' != '\n'
   - buffer[1] = 'o' != '\n'
   - buffer[2] = '\n' FOUND at offset 2

3. Combine partial + new data up to '\n'
   - total_size = 3 + 3 = 6 (only up to '\n')
   - krealloc(partial_buffer, 6) returns 0xffff1000 (expanded)
   - memcpy(0xffff1000 + 3, 0xffff2000, 3) = "lo\n"
   - Result: "hello\n"

4. Add combined buffer to circular buffer
   - buffer.entry[0] = {0xffff1000, 6}
   - buffer.in_offs = 1

5. Store remaining data ("world") in new partial_buffer
   - dev->partial_buffer = kmalloc(5) = 0xffff3000
   - memcpy(0xffff3000, 0xffff2000 + 3, 5) = "world"
   - dev->partial_size = 5

6. kfree(temp buffer at 0xffff2000)

7. Return 8 (bytes written from this call)

## SCENARIO 3: Buffer full (11th write)

STATE BEFORE:
-------------
entry[0] = {0x1000, 10}  // "write1\n"
entry[1] = {0x2000, 10}  // "write2\n"
...
entry[9] = {0xa000, 10}  // "write10\n"
in_offs = 0
out_offs = 0
full = true

INPUT: write(fd, "write11\n", 8)

STEP BY STEP:
-------------
1. kmalloc(8) = 0xb000, copy "write11\n"

2. Add to circular buffer
   - entry[0] currently = {0x1000, 10} (OLD)
   - kfree(0x1000)  // FREE OLD MEMORY
   - entry[0] = {0xb000, 8} (NEW)
   - in_offs = 1
   - out_offs = 1 (advanced because full)
   - full = true (still full)

3. Result: entry[0] overwritten, entry[1..9] preserved

