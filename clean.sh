#!/bin/bash
# Clean script for assignment 8 buildroot project
# Removes build artifacts to enable clean rebuild

set -e
cd `dirname $0`

echo "Cleaning buildroot build..."

if [ -d buildroot ]; then
    make -C buildroot clean 2>/dev/null || echo "Buildroot clean failed or nothing to clean"
fi

# Clean driver directory
echo "Cleaning aesd-char-driver..."
cd aesd-char-driver
make clean 2>/dev/null || echo "Driver clean completed or nothing to clean"
cd ..

# Clean server directory if it exists
if [ -d server ]; then
    echo "Cleaning server..."
    cd server
    make clean 2>/dev/null || echo "Server clean completed or nothing to clean"
    cd ..
fi

# Remove test log files
rm -f test.sh.log

echo "Clean complete!"
