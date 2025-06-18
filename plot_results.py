import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

# 实验数据
labels = ['cat', 'mycat1', 'mycat2', 'mycat3', 'mycat4', 'mycat5', 'mycat6']
times = [0.171, 427.340, 0.252, 0.244, 0.240, 0.174, 0.175]  # 运行时间（秒）

# 绘制柱状图
plt.figure(figsize=(10, 6))
bars = plt.bar(labels, times, color='lightgreen', edgecolor='black')
plt.ylabel('time (s)')
plt.title('processing compare')
plt.yscale('log')  # 使用对数刻度突出差异
plt.grid(True, axis='y', linestyle='--', alpha=0.7)

# 在柱子上标注时间值
for bar, time in zip(bars, times):
    plt.text(bar.get_x() + bar.get_width() / 2, time * 1.1, f'{time:.3f}', 
             ha='center', va='bottom', fontsize=9)

plt.tight_layout()
plt.show()