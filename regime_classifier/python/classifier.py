import numpy as np
import pandas as pd
from hmmlearn import hmm
from hmmlearn import vhmm
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, PCA, PCA_VAR, DROP_COLUMNS, DATES
import os
from env import PROJECT_ROOT

# ==============================================================
# ============= Loading Data from Multiple Dates ===============
# ============= Also concatenating and dropping columns ========
# ==============================================================
dfs = []
lengths = []
for date in DATES:
    df = pd.read_csv("{}\\data\\{}\\{}\\{}_norm.csv".format(PROJECT_ROOT, FOLDER_NAME, date, ASSET))
    dfs.append(df)
    lengths.append(len(df))

df = pd.concat(dfs)

# Optional: rename columns to distinguish
features = df.drop(columns=DROP_COLUMNS, errors='ignore')
print(str("timestamp_ns" in features.columns))
# Convert to NumPy array
X_array = features.to_numpy()

# ==============================================================
# ============= Do PCA if enabled, reducing dimensions =========
# ==============================================================

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



# ==============================================================
# ============= Run, fit, and predict HMM ======================    
# ==============================================================
np.random.seed(45)

model = hmm.GaussianHMM(
    n_components=REGIME_COUNT,
    covariance_type="full",
    n_iter=500,
    init_params="",  # skip 'm' to avoid random mean init
    params="stmc"
)

model.fit(X_array, lengths)
states = model.predict(X_array, lengths)


# ==============================================================
# ============= Output Model Information/scores ================
# ==============================================================
outfile = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}_information.csv"
outfile_dir = os.path.dirname(outfile)
just_created = False
if not os.path.exists(outfile_dir):
    os.makedirs(outfile_dir)
    just_created = True

with open(outfile, "a") as f:
    if just_created:
        f.write("regime_count,AIC,BIC,LL,SIL,DB,CH\n")
    # Manually compute AIC and BIC
    log_likelihood = model.score(X_array, lengths)
    aic = model.aic(X_array, lengths)
    bic = model.bic(X_array, lengths)

    f.write(f"{REGIME_COUNT},{aic},{bic},{log_likelihood},")

# ==============================================================
# ============= Add regimes to DataFrame and save ==============
# ============= data, model, and model config     ==============
# ==============================================================
df["regime"] = states
# save features with regimes
df.to_csv(f"{PROJECT_ROOT}\\regime_classifier\\python\\features_with_regimes.csv", index=False)

# make a directory for the model
model_dir = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}\\{REGIME_COUNT}"
if not os.path.exists(model_dir):
    os.makedirs(model_dir)

# output model pickle
import joblib
joblib.dump(model, f"{model_dir}\\model.pkl")

# output model params
outfile = f"{model_dir}\\model_params.txt"
with open(outfile, "w") as f:
    f.write("means:\n")
    f.write(str(model.means_))
    f.write("\ncovars:\n")
    f.write(str(model.covars_))
    f.write("\ntransmat:\n")
    f.write(str(model.transmat_))
    f.write("\nstartprob:\n")
    f.write(str(model.startprob_))