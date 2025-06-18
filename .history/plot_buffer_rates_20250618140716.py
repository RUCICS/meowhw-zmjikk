import matplotlib.pyplot as plt

# 测试数据
multipliers = [1, 2, 4, 8, 16, 32, 64, 128, 256]
buffer_sizes = [4096 * m for m in multipliers]
speeds = [8192, 10649.6, 12492.8, 11468.8, 13619.2, 13209.6, 13721.6, 11980.8, 12390.4]  # MB/s

# 绘制柱状图
plt.figure(figsize=(10, 6))
plt.bar([str(m) for m in multipliers], speeds, color='skyblue')
plt.xlabel('倍数 (相对于基础缓冲区大小 4096 字节)')
plt.ylabel('读写速度 (MB/s)')
plt.title('不同缓冲区大小的读写速度')
plt.grid(True, axis='y')
plt.axhline(y=13721.6, color='r', linestyle='--', label='峰值速度 (64倍, 13721.6 MB/s)')
plt.legend()
plt.show()