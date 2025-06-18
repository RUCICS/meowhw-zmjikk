import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

# 设置支持中文的字体
plt.rcParams['font.sans-serif'] = ['Noto Sans CJK SC', 'WenQuanYi Micro Hei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

# 测试数据
multipliers = [1, 2, 4, 8, 16, 32, 64, 128, 256]
buffer_sizes = [4096 * m for m in multipliers]
speeds = [8192, 10649.6, 12492.8, 11468.8, 13619.2, 13209.6, 13721.6, 11980.8, 12390.4]  # MB/s

# 绘制柱状图
plt.figure(figsize=(10, 6))
bars = plt.bar([str(m) for m in multipliers], speeds, color='skyblue', edgecolor='black')
plt.xlabel('倍数 (相对于基础缓冲区大小 4096 字节)')
plt.ylabel('读写速度 (MB/s)')
plt.title('不同缓冲区大小的读写速度')
plt.grid(True, axis='y', linestyle='--', alpha=0.7)
plt.axhline(y=13721.6, color='red', linestyle='--', label='峰值速度 (64倍, 13721.6 MB/s)')
plt.legend()

# 在柱子上标注速度值
for bar, speed in zip(bars, speeds):
    plt.text(bar.get_x() + bar.get_width() / 2, speed + 200, f'{speed:.1f}', 
             ha='center', va='bottom', fontsize=9)

plt.tight_layout()
plt.show()