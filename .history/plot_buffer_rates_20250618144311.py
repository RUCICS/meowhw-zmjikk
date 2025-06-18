import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

# 加载本地字体
font_path = '/home/zmjikk/meowhw-zmjikk/NotoSansCJKsc-Regular.otf'
font_prop = fm.FontProperties(fname=font_path)
plt.rcParams['font.sans-serif'] = ['Noto Sans CJK SC']
plt.rcParams['axes.unicode_minus'] = False

# 测试数据
multipliers = [1, 2, 4, 8, 16, 32, 64, 128, 256]
buffer_sizes = [4096 * m for m in multipliers]
speeds = [8192, 10649.6, 12492.8, 11468.8, 13619.2, 13209.6, 13721.6, 11980.8, 12390.4]

plt.figure(figsize=(10, 6))
bars = plt.bar([str(m) for m in multipliers], speeds, color='skyblue', edgecolor='black')
plt.xlabel('倍数 (相对于基础缓冲区大小 4096 字节)', fontproperties=font_prop)
plt.ylabel('读写速度 (MB/s)', fontproperties=font_prop)
plt.title('不同缓冲区大小的读写速度', fontproperties=font_prop)
plt.grid(True, axis='y', linestyle='--', alpha=0.7)
plt.axhline(y=13721.6, color='red', linestyle='--', label='峰值速度 (64倍, 13721.6 MB/s)')
plt.legend(prop=font_prop)

for bar, speed in zip(bars, speeds):
    plt.text(bar.get_x() + bar.get_width() / 2, speed + 200, f'{speed:.1f}', 
             ha='center', va='bottom', fontsize=9, fontproperties=font_prop)

plt.tight_layout()
plt.show()