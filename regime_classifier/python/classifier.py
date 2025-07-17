import numpy as np
import pandas as pd
from hmmlearn import hmm
from hmmlearn import vhmm
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, AVG, LONG_SHORT, PCA, PCA_VAR, DROP_COLUMNS
import os
from env import PROJECT_ROOT

# Load the data
long_df = pd.read_csv("{}\\data\\{}\\{}_norm_long.csv".format(PROJECT_ROOT, FOLDER_NAME, ASSET))
short_df = pd.read_csv("{}\\data\\{}\\{}_norm_short.csv".format(PROJECT_ROOT, FOLDER_NAME, ASSET))

avg_features = (long_df.drop(columns=DROP_COLUMNS, errors='ignore') + \
                short_df.drop(columns=DROP_COLUMNS, errors='ignore')) / 2

# Optional: rename columns to distinguish
long_features = long_df.drop(columns=DROP_COLUMNS, errors='ignore')
short_features = short_df.drop(columns=DROP_COLUMNS, errors='ignore')

# Prefix column names for clarity
long_features = long_features.add_prefix("long_")
short_features = short_features.add_prefix("short_")

long_short_df = pd.concat([long_features, short_features], axis=1)

# Concatenate horizontally (axis=1)
if AVG:
    X = avg_features
elif LONG_SHORT:
    X = pd.concat([long_features, short_features], axis=1)
else:
    X = long_features

# Convert to NumPy array
X_array = X.to_numpy()

if PCA:
    from sklearn.preprocessing import StandardScaler
    from sklearn.decomposition import PCA

    # Step 1: Standardize the features
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X_array)  # X_array is your features (NumPy array)

    # Step 2: Apply PCA
    pca = PCA(n_components=PCA_VAR)  # Retain 99.9% of variance
    X_pca = pca.fit_transform(X_scaled)
    print(f"PCA reduced dimensions from {X_array.shape[1]} to {X_pca.shape[1]}")
    X_array = X_pca

np.random.seed(45)

model = hmm.GaussianHMM(
    n_components=REGIME_COUNT,
    covariance_type="full",
    n_iter=400,
    init_params="",  # skip 'm' to avoid random mean init
    params="stmc"
)

model.fit(X_array)

# Predict regimes
states = model.predict(X_array)

outfile = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}_information.csv"
outfile_dir = os.path.dirname(outfile)
if not os.path.exists(outfile_dir):
    os.makedirs(outfile_dir)

with open(outfile, "a") as f:
    # Manually compute AIC and BIC
    log_likelihood = model.score(X_array)
    n_samples = X_array.shape[0]
    n_components = model.n_components
    n_features = model.n_features

    # Count parameters
    n_means = n_components * n_features
    if model.covariance_type == "full":
        n_covars = n_components * n_features * (n_features + 1) // 2
    elif model.covariance_type == "diag":
        n_covars = n_components * n_features
    else:
        raise ValueError("Unsupported covariance_type for AIC/BIC calculation")

    n_trans = n_components * (n_components - 1)
    n_startprob = n_components - 1

    n_params = n_means + n_covars + n_trans + n_startprob

    aic = -2 * log_likelihood + 2 * n_params
    bic = -2 * log_likelihood + n_params * np.log(n_samples)

    f.write(f"{LONG_SHORT},{AVG},{REGIME_COUNT},{aic},{bic},")

# Add to DataFrame if desired
long_short_df["regime"] = states

X_with_meta = pd.concat([long_df[["timestamp_ns"]], long_short_df], axis=1)
print("saving to csv")
X_with_meta.to_csv(f"{PROJECT_ROOT}\\regime_classifier\\python\\features_with_regimes.csv", index=False)

# output model pickle
import joblib
joblib.dump(model, f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}\\{REGIME_COUNT}\\{LONG_SHORT}_{AVG}\\model.pkl")