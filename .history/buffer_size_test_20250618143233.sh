#!/bin/bash

buf_size=$(stat -f -c %S test.txt)
echo "Base buffer size: $buf_size"

for multiplier in 1 2 4 8 16 32 64 128 256; do
    buffer_size=$((buf_size * multiplier))
    echo "Testing buffer size: $buffer_size"
    dd if=test.txt of=/dev/null bs=$buffer_size count=2048 2>&1 | tail -n 1
done

