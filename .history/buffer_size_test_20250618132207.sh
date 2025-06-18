#!/bin/bash
# 文件名: buffer_size_test.sh
# 描述: 测量不同缓冲区大小下的文件读取性能
# 用法: sudo ./buffer_size_test.sh [测试文件大小(MB)] [测试次数]

# 配置参数
DEFAULT_FILE_SIZE=1024  # 默认测试文件大小(1GB)
DEFAULT_TEST_COUNT=5    # 默认测试次数
CACHE_DROP_CMD="echo 3 > /proc/sys/vm/drop_caches"  # 清除缓存命令
TEST_FILE="testfile"    # 测试文件名
OUTPUT_CSV="buffer_test_results.csv"  # 结果输出文件

# 检查root权限
if [ "$(id -u)" -ne 0 ]; then
    echo "错误: 此脚本需要root权限运行 (用于清除系统缓存)"
    exit 1
fi

# 解析参数
FILE_SIZE=${1:-$DEFAULT_FILE_SIZE}
TEST_COUNT=${2:-$DEFAULT_TEST_COUNT}

# 缓冲区大小列表 (从4KB到16MB)
BUFFER_SIZES=(
    4k     8k     16k    32k    64k 
    128k   256k   512k   1M     2M 
    4M     8M     16M
)

# 创建测试文件
echo "创建 ${FILE_SIZE}MB 测试文件..."
dd if=/dev/urandom of=$TEST_FILE bs=1M count=$FILE_SIZE status=progress
echo "测试文件创建完成: $(du -h $TEST_FILE)"

# 准备结果文件
echo "Buffer Size (KB),Read Speed (MB/s)" > $OUTPUT_CSV

# 测试函数
run_test() {
    local buffer_size=$1
    local total_speed=0
    local count=0
    
    echo "测试缓冲区大小: $buffer_size"
    
    for ((i=1; i<=$TEST_COUNT; i++)); do
        # 清除缓存
        eval $CACHE_DROP_CMD
        
        # 运行测试并提取速度
        local speed=$(dd if=$TEST_FILE of=/dev/null bs=$buffer_size 2>&1 | \
                    grep -oE '[0-9.]+ [Mk]B/s' | \
                    awk '{if ($2 == "kB/s") print $1/1024; else print $1}')
        
        # 记录结果
        echo "  测试 $i: $speed MB/s"
        total_speed=$(echo "$total_speed + $speed" | bc)
        count=$((count + 1))
    done
    
    # 计算平均值
    local avg_speed=$(echo "scale=2; $total_speed / $count" | bc)
    
    # 转换缓冲区大小为KB
    local size_kb=$(echo $buffer_size | \
        awk '/k$/{gsub("k",""); print $1} /M$/{gsub("M",""); print $1*1024}')
    
    # 保存结果
    echo "$size_kb,$avg_speed" >> $OUTPUT_CSV
    echo "平均速度: $avg_speed MB/s"
}

# 运行所有测试
for size in "${BUFFER_SIZES[@]}"; do
    run_test $size
done

# 清理测试文件
rm $TEST_FILE
echo "测试完成! 结果已保存到 $OUTPUT_CSV"

# 生成图表
echo "生成性能图表..."
gnuplot <<- EOF
    set terminal pngcairo size 1200,800 enhanced font 'Verdana,12'
    set output 'buffer_performance.png'
    set title "缓冲区大小对文件读取性能的影响"
    set xlabel "缓冲区大小 (KB)"
    set ylabel "读取速度 (MB/s)"
    set logscale x 2
    set grid
    set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 ps 1.5
    set datafile separator ","
    plot "$OUTPUT_CSV" using 1:2 with linespoints ls 1 title "读取速度"
EOF

echo "图表已保存为 buffer_performance.png"