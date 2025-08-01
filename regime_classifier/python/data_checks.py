import pandas as pd
import numpy as np
from constants import FOLDER_NAME, ASSET, DROP_COLUMNS, REGIME_COUNT
import os
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import silhouette_score, davies_bouldin_score, calinski_harabasz_score
from env import PROJECT_ROOT

def variance_checks(outfile):
    df = pd.read_csv("features_with_regimes.csv")

    df = df.drop(columns=DROP_COLUMNS, errors='ignore')
    vars = []
    # do this for all features
    for feature in df.columns:
        vars.append((feature, np.var(df[feature].values)))
    
    vars.sort(key=lambda x: x[1], reverse=True)
    with open(outfile, "a") as f:
        f.write("=========== VARIANCE OF FEATURES ===========\n")
        f.write("Feature, Variance\n")
        for var in vars:
            f.write(f"{var[0]},{var[1]}\n")
        f.write("\n============== END ==============\n")

def duration_checks(outfile):
    """
    Computes the mean and median duration of each regime in a time-ordered DataFrame.

    Parameters:
    - df (pd.DataFrame): DataFrame containing a column with regime labels.
    - regime_col (str): Name of the column containing regime labels.

    Returns:
    - None
    """
    regime_col = "regime"
    df = pd.read_csv("features_with_regimes.csv")
    if regime_col not in df.columns:
        raise ValueError(f"Column '{regime_col}' not found in DataFrame.")

    # Detect regime changes
    df = df.copy()
    df['regime_shift'] = (df[regime_col] != df[regime_col].shift()).cumsum()

    # Count duration of each regime period
    durations = df.groupby(['regime_shift', regime_col]).size().reset_index(name='duration')

    # Compute mean and median durations per regime
    stats = durations.groupby(regime_col)['duration'].agg(['mean', 'median']).reset_index()
    stats.columns = ['regime', 'mean_duration', 'median_duration']

    with open(outfile, "a") as f:
        f.write("\n=========== DURATION OF REGIMES ===========\n")
        f.write("Regime, Mean Duration, Median Duration\n")
        for index, row in stats.iterrows():
            f.write(f"{row['regime']},{row['mean_duration']},{row['median_duration']}\n")
        f.write("\n============== END ==============\n")


def calculate_index_scores(outfile):
    # open outfile
    with open(outfile, "a") as f:
        file_path = "features_with_regimes.csv"

        if not os.path.isfile(file_path):
            print(f"[WARNING] File not found: {file_path}")

        print(f"[INFO] Processing: {file_path}")
        df = pd.read_csv(file_path)
        df.drop(columns=DROP_COLUMNS, errors='ignore', inplace=True)
        if "timestamp_ns" in df.columns:
            df = df.drop(columns=["timestamp_ns"])

        X = df.drop(columns=["regime"])
        y = df["regime"]

        # Standardize features
        scaler = StandardScaler()
        X_scaled = scaler.fit_transform(X)

        # Compute scores
        try:
            sil = silhouette_score(X_scaled, y, sample_size=20000)
            db = davies_bouldin_score(X_scaled, y)
            ch = calinski_harabasz_score(X_scaled, y)
            f.write(f"{sil},{db},{ch}\n")
            print(f"[INFO] Silhouette Score: {sil}")
            print(f"[INFO] Davies-Bouldin Score: {db}")
            print(f"[INFO] Calinski-Harabasz Score: {ch}")
        except Exception as e:
            print(f"[ERROR] Scoring failed for regimes={len(y.unique())}: {e}\n")




outfile = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}\\{REGIME_COUNT}\\information.txt"
outfile_dir = os.path.dirname(outfile)
if not os.path.exists(outfile_dir):
    os.makedirs(outfile_dir)

variance_checks(outfile)
duration_checks(outfile)

other_outfile = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}_information.csv"
calculate_index_scores(other_outfile)
