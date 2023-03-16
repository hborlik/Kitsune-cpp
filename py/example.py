import numpy as np
import time

def load_rmse(filename : str) -> list[float]:
    rmses = []
    with open(filename) as textfile1:
        for x in textfile1:
            rmses.append(float(x.strip()))
    return rmses

RMSEs = load_rmse("RMSE.txt")
# RMSEs = load_rmse("RMSE_fixed_orig.txt")

FMgrace = 5000
ADgrace = 50000


# Here we demonstrate how one can fit the RMSE scores to a log-normal distribution (useful for finding/setting a cutoff threshold \phi)
from scipy.stats import norm



benignSample = np.log(RMSEs[FMgrace+ADgrace+1:100000])
RMSEs = RMSEs[0:200000]
logProbs = norm.logsf(np.log(RMSEs), np.mean(benignSample), np.std(benignSample))

# benignSample = np.log(RMSE1[:45000])
# logProbs1 = norm.logsf(np.log(RMSE1), np.mean(benignSample), np.std(benignSample))

# plot the RMSE anomaly scores
print("Plotting results")
from matplotlib import pyplot as plt
from matplotlib import cm
plt.figure(figsize=(10,5))
plt.axvline(x=5000, color='yellow', linestyle='--')
plt.axvline(x=FMgrace+ADgrace+1, color='red', linestyle='--')

fig0 = plt.scatter(range(len(RMSEs)),RMSEs[:],s=0.1,c=logProbs[:],cmap='RdYlGn')
figbar0=plt.colorbar(fig0)
figbar0.ax.set_ylabel('Log Probability\n ', rotation=270)
# fig1 = plt.scatter(range(len(RMSE1)),RMSE1[:],s=0.1,c=logProbs1[:],cmap='Wistia')
# figbar1=plt.colorbar(fig1)
# figbar1.ax.set_ylabel('Log Probability\n ', rotation=270)
plt.yscale("log")
plt.title("Anomaly Scores from Kitsune's Execution Phase")
plt.ylabel("RMSE (log scaled)")
plt.xlabel("Time elapsed [min]")
plt.show()
