#!/bin/bash

# 确保有足够的参数
if [ $# -ne 1 ]; then
    echo "Usage: $0 <filename>"
    exit 1
fi

filename=$1

# 获取页大小
page_size=$(getconf PAGE_SIZE)
echo "系统页大小: ${page_size} 字节"

# 获取文件系统块大小
fs_block_size=$(stat -f --format="%S" "$filename")
echo "文件系统块大小: ${fs_block_size} 字节"

# 计算基准缓冲区大小（页大小和文件系统块大小的最小公倍数）
lcm() {
    a=$1
    b=$2
    # 计算最大公约数
    while [ $b -ne 0 ]; do
        temp=$b
        b=$(($a % $b))
        a=$temp
    done
    # 计算最小公倍数
    echo $(($1 * $2 / $a))
}

base_size=$(lcm $page_size $fs_block_size)
echo "基准缓冲区大小: ${base_size} 字节"

# 测试不同倍数的缓冲区大小
multipliers=(1 2 4 8 16 32 64 128 256 512 1024)

echo "测试不同缓冲区大小的性能..."
echo "缓冲区大小(字节), 读取速度(MB/s)"

for m in "${multipliers[@]}"; do
    buf_size=$(($base_size * $m))
    # 使用dd测试读取速度
    result=$(dd if="$filename" of=/dev/null bs=$buf_size count=1 status=none 2>&1 | awk -F, '{print $2}' | awk '{print $1}')
    speed=$(echo $result | awk '{print $1}')
    unit=$(echo $result | awk '{print $2}')
    
    # 转换为MB/s
    if [ "$unit" == "kB/s" ]; then
        speed=$(echo "scale=2; $speed / 1024" | bc)
    elif [ "$unit" == "GB/s" ]; then
        speed=$(echo "scale=2; $speed * 1024" | bc)
    fi
    
    echo "${buf_size}, ${speed}"
done    