import numpy as np
import matplotlib.pyplot as plt

# 测试数据
multipliers = [1, 2, 4, 8, 16, 32, 64, 128, 256]
buffer_sizes = [4096 * m for m in multipliers]
speeds = [8192, 10649.6, 12492.8, 11468.8, 13619.2, 13209.6, 13721.6, 11980.8, 12390.4]  # MB/s

# 绘制柱状图
plt.figure(figsize=(10, 6))
bars = plt.bar([str(m) for m in multipliers], speeds, color='skyblue', edgecolor='black')
plt.xlabel('multiple')
plt.ylabel('speed (MB/s)')
plt.grid(True, axis='y', linestyle='--', alpha=0.7)
plt.legend()

# 在柱子上标注速度值
for bar, speed in zip(bars, speeds):
    plt.text(bar.get_x() + bar.get_width() / 2, speed + 200, f'{speed:.1f}', 
             ha='center', va='bottom', fontsize=9)

plt.tight_layout()
plt.show()