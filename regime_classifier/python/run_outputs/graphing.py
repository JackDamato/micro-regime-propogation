import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
df = pd.read_csv("C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\data\\output\\base_SPY_norm_long.csv")
# timestamp_ns,instrument,log_spread,price_impact,log_return,ewm_volatility,realized_variance,directional_volatility,spread_volatility,ofi,signed_volume_pressure,order_arrival_rate,depth_imbalance,market_depth,lob_slope,price_gap,tick_direction_entropy,reversal_rate,spread_crossing,aggressor_bias,shannon_entropy,liquidity_stress

# Save each graph as a png
features = ['log_spread', 'price_impact', 'log_return', 'ewm_volatility', 'realized_variance', 'directional_volatility', 'spread_volatility', 'ofi', 'signed_volume_pressure', 'order_arrival_rate', 'depth_imbalance', 'market_depth', 'lob_slope', 'price_gap', 'tick_direction_entropy', 'reversal_rate', 'spread_crossing', 'aggressor_bias', 'shannon_entropy', 'liquidity_stress']

# Convert timestamp_ns to human readable format UNIX ns to datetime
df['timestamp_ns'] = pd.to_datetime(df['timestamp_ns'], unit='ns')
df = df[::50]

for feature in features:
    plt.figure(figsize=(15, 5))
    plt.title(feature)
    plt.xlabel('Time')
    plt.ylabel(feature)
    plt.plot(df['timestamp_ns'], df[feature], label=feature)
    plt.legend()
    plt.savefig(f"C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\python\\data\\feature_graphs\\{feature}.png")


# print out all variances of all normalized variables
variances = []
for feature in features:
    variances.append((feature, float(df[feature].var())))
print(variances)