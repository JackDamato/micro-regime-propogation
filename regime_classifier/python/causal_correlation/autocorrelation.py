import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from statsmodels.tsa.stattools import adfuller

# === Load CSV ===
FILE_PATH = "C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\data\\output_0.50S_7200_1200_2000\\base_SPY_norm_long.csv"
df = pd.read_csv(FILE_PATH)

# === Drop non-feature columns ===
drop_cols = ["timestamp_ns", "instrument"]
features_df = df.drop(columns=[col for col in drop_cols if col in df.columns])

# === Summary Metrics ===
summary = {
    "Lag-1 ρ": {},
    "Std": {},
    "Delta Std": {},
    "CoeffVar": {},
    "ADF p-value": {},
}

# === Plot Autocorrelations ===
num_features = features_df.shape[1]
cols = 3
rows = int(np.ceil(num_features / cols))
fig, axes = plt.subplots(rows, cols, figsize=(cols * 5, rows * 3))
axes = axes.flatten()

for i, col in enumerate(features_df.columns):
    series = features_df[col].dropna()
    
    # Autocorrelation
    acf = [series.autocorr(lag) for lag in range(1, 21)]
    summary["Lag-1 ρ"][col] = acf[0]
    
    # Standard deviation
    summary["Std"][col] = series.std()
    
    # Delta std
    delta = series.diff().dropna()
    summary["Delta Std"][col] = delta.std()
    
    # Coefficient of Variation (CV)
    mean_val = series.mean()
    summary["CoeffVar"][col] = series.std() / mean_val if mean_val != 0 else np.nan
    
    # Stationarity test (ADF)
    try:
        adf_pval = adfuller(series, maxlag=1)[1]  # p-value
    except Exception:
        adf_pval = np.nan
    summary["ADF p-value"][col] = adf_pval

    # Plot autocorrelation
    axes[i].stem(range(1, 21), acf, basefmt=" ")
    axes[i].set_title(f"Autocorr: {col}")
    axes[i].set_xlabel("Lag")
    axes[i].set_ylabel("ρ")
    axes[i].set_ylim(-1, 1)

# Hide unused axes
for j in range(i + 1, len(axes)):
    axes[j].axis("off")

plt.tight_layout()
plt.savefig("C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\regime_classifier\\python\\autocorrelation.png")
plt.show()

# === Summary DataFrame ===
summary_df = pd.DataFrame(summary)
summary_df = summary_df.round(4)
summary_df_sorted = summary_df.sort_values(by="Lag-1 ρ", ascending=False)

print("\n=== Feature Summary ===")
print(summary_df_sorted)

# === Correlation Heatmap ===
corr = features_df.corr()
plt.figure(figsize=(12, 10))
sns.heatmap(corr, cmap="coolwarm", annot=False, center=0)
plt.title("Feature Correlation Heatmap")
plt.tight_layout()
plt.savefig("C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\regime_classifier\\python\\correlation_heatmap.png")
plt.show()
