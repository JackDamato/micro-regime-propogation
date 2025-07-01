import numpy as np
import pandas as pd
from hmmlearn import hmm
from hmmlearn import vhmm

# Load the data
long_df = pd.read_csv("C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\data\\output\\base_SPY_norm_long.csv")
short_df = pd.read_csv("C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\data\\output\\base_SPY_norm_short.csv")


# Optional: rename columns to distinguish
long_features = long_df.drop(columns=["timestamp_ns", "instrument", "price_gap"], errors='ignore')
short_features = short_df.drop(columns=["timestamp_ns", "instrument", "price_gap"], errors='ignore')

# Prefix column names for clarity
long_features = long_features.add_prefix("long_")
short_features = short_features.add_prefix("short_")

# Concatenate horizontally (axis=1)
X = pd.concat([long_features, short_features], axis=1)

# Convert to NumPy array
X_array = X.to_numpy()


from sklearn.preprocessing import StandardScaler
from sklearn.decomposition import PCA

# Step 1: Standardize the features
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X_array)  # X_array is your features (NumPy array)

# Step 2: Apply PCA
pca = PCA(n_components=0.95)  # Retain 95% of variance
X_pca = pca.fit_transform(X_scaled)
print(pca.components_)


print(f"PCA reduced dimensions from {X_array.shape[1]} to {X_pca.shape[1]}")

np.random.seed(42)

model = vhmm.VariationalGaussianHMM(
    n_components=4,
    covariance_type="full",
    n_iter=100,
    init_params="",  # skip 'm' to avoid random mean init
    params="stmc"
)

model.fit(X_array)
# # Predict regimes
states = model.predict(X_array)

# # Add to DataFrame if desired
X["regime"] = states


X_with_meta = pd.concat([long_df[["timestamp_ns"]], X], axis=1)
X_with_meta.to_csv("features_with_regimes.csv", index=False)