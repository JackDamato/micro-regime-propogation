import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np

# Read the CSV file and sample every 25th row
df = pd.read_csv("features_with_regimes.csv")
# Convert timestamp_ns to datetime
df['timestamp_ns'] = pd.to_datetime(df['timestamp_ns'], unit='ns')
# Take every 25th row to reduce data points
# df = df.iloc[::25].copy()

# Get list of feature columns (exclude non-feature columns)
feature_columns = [col for col in df.columns if col not in ['timestamp_ns', 'regime']]

# Create a color map for the different regimes (0-4)
colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd']

# Create a figure with subplots for each feature
num_features = len(feature_columns)
fig, axes = plt.subplots(num_features, 1, figsize=(15, 3 * num_features), sharex=True)

# If there's only one feature, axes won't be an array
if num_features == 1:
    axes = [axes]

# Plot each feature with regime highlights
for i, feature in enumerate(feature_columns):
    ax = axes[i]
    
    # Plot the feature line (using scatter for better performance with downsampled data)
    ax.scatter(df['timestamp_ns'], df[feature], color='black', s=1, alpha=0.7)
    
    # Add shaded regions for each regime
    for regime in range(5):
        regime_mask = df['regime'] == regime
        regime_changes = regime_mask.astype(int).diff().fillna(0)
        regime_starts = df.index[regime_changes == 1]
        regime_ends = df.index[regime_changes == -1]
        
        # Handle edge cases
        if len(regime_starts) > len(regime_ends):
            regime_ends = regime_ends.append(pd.Index([df.index[-1]]))
        
        for start, end in zip(regime_starts, regime_ends):
            if start < len(df) and end < len(df):
                ax.axvspan(df['timestamp_ns'].iloc[start], 
                          df['timestamp_ns'].iloc[end], 
                          facecolor=colors[regime], 
                          alpha=0.3)
    
    # Add labels and title
    ax.set_ylabel(feature)
    ax.grid(True, linestyle='--', alpha=0.7)
    
    # Format x-axis to show dates nicely
    if i == num_features - 1:  # Only show x-axis labels on the bottom plot
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
        ax.xaxis.set_major_locator(mdates.DayLocator(interval=1))
        plt.setp(ax.xaxis.get_majorticklabels(), rotation=45)
    else:
        ax.set_xticklabels([])

# Add a legend for the regimes
legend_elements = [plt.Rectangle((0, 0), 1, 1, facecolor=colors[i], alpha=0.3, label=f'Regime {i}')
                  for i in range(5)]
plt.legend(handles=legend_elements, loc='upper right', bbox_to_anchor=(1.15, num_features + 0.5))

plt.suptitle('Features with Regime Highlights', y=1.02)
plt.tight_layout()
plt.savefig('regime_highlighted_features.png', bbox_inches='tight', dpi=300)
plt.close()