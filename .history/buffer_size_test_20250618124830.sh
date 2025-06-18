# 请在这里填入你的测试脚本
%%bash

FILE="test.txt"
BASE_BUF_SIZE=4096  # 假设基础大小为 4KB，实际应从 mycat4 获取
MULTIPLIERS=(1 2 4 8 16 32 64 128)  # 确保数组元素用空格分隔

echo "Testing buffer sizes..."
for MULT in "${MULTIPLIERS[@]}"; do
    BUF_SIZE=$((BASE_BUF_SIZE * MULT))
    echo "Buffer size: $BUF_SIZE bytes"
    dd if=$FILE of=/dev/null bs=$BUF_SIZE status=progress 2>&1 | tail -n 1
done