import pandas as pd
from matplotlib import pyplot as plt
import numpy as np

ex1_B1 = {"Real": [60, 180, 120], "Measured": [74, 173, 145]}
ex1_B2 = {"Real": [120, 180, 60], "Measured": [116,195, 81]}
ex1_df = pd.DataFrame(ex1_B1, index=["Waiting1", "Exam1", "Waiting2"])
ex2_df = pd.DataFrame(ex1_B2, index=["Waiting1", "Exam1", "Waiting2"])

n_groups = 3
real_B1 = ex1_df["Real"]
measured_B1 = ex1_df["Measured"]

# create plot
fig, axes = plt.subplots(2, 1, figsize=(10, 5))
index = np.arange(n_groups)
bar_width = 0.35
opacity = 0.8

rects1 = axes[0].bar(index, real_B1, bar_width,
alpha=opacity,
color='g',
label='Real Durations')

rects2 = axes[0].bar(index + bar_width, measured_B1, bar_width,
alpha=opacity,
color='r',
label='Measured Durations')

axes[0].set_ylabel("Waiting Room")
axes[1].set_ylabel("Exam Room")
fig.text(0.5, 0, 'Experiment phases', ha='center')
fig.text(0., 0.5, 'Durations [seconds]', va='center', rotation='vertical')
plt.suptitle("Real vs Measured durations for Experiment 1")

plt.tight_layout()
plt.show()