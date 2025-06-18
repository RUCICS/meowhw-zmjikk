#!/bin/bash

# 获取任务4中的基础缓冲区大小
buf_size=$(stat -f -c %S test.txt)  # 文件系统块大小
echo "Base buffer size: $buf_size"

# 测试不同倍数的缓冲区大小
for multiplier in 1 2 4 8 16 32 64 128 256; do
    buffer_size=$((buf_size * multiplier))
    echo "Testing buffer size: $buffer_size"
    # 使用 dd 测试读写速度
    dd if=test.txt of=/dev/null bs=$buffer_size count=2048 2>&1 | tail -n 1
done