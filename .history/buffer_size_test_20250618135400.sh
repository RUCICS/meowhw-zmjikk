#!/bin/bash

# 获取基础缓冲区大小（任务4的io_blocksize）
gcc -o get_buf_size get_buf_size.c
base_buf_size=$(./get_buf_size test.txt)
echo "Base buffer size: $base_buf_size"

# 测试不同倍数的缓冲区大小
for multiplier in 1 2 4 8 16 32 64 128 256; do
    buffer_size=$((base_buf_size * multiplier))
    echo "Testing buffer size: $buffer_size"
    dd if=test.txt of=/dev/null bs=$buffer_size count=2048 2>&1 | grep 'bytes/sec' | awk '{print $1 " MB/s"}'
done