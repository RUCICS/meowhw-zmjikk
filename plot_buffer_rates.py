import numpy as np
import matplotlib.pyplot as plt

# 测试数据
multipliers = [1, 2, 4, 8, 16, 32, 64, 128, 256]
buffer_sizes = [4096 * m for m in multipliers]
speeds = np.array([6041.6, 8096.0, 10240.0, 10342.4, 11161.6, 12492.8, 12902.4, 11980.8, 12697.6])

# 绘制柱状图
plt.figure(figsize=(10, 6))
bars = plt.bar([str(m) for m in multipliers], speeds, color='skyblue', edgecolor='black')
plt.xlabel('multiple')
plt.ylabel('speed (MB/s)')
plt.title('Buffer Size vs Speed')
plt.grid(True, axis='y', linestyle='--', alpha=0.7)
plt.legend()

# 在柱子上标注速度值
for bar, speed in zip(bars, speeds):
    plt.text(bar.get_x() + bar.get_width() / 2, speed + 200, f'{speed:.1f}', 
             ha='center', va='bottom', fontsize=9)

plt.tight_layout()
plt.show()