import pandas as pd
import numpy as np
from constants import FOLDER_NAME, ASSET, DROP_COLUMNS, REGIME_COUNT
import os
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import silhouette_score, davies_bouldin_score, calinski_harabasz_score
from env import PROJECT_ROOT
import polars as pl

def variance_checks(outfile, df):
    df = df.drop(DROP_COLUMNS)
    vars = []
    # do this for all features
    for feature in df.columns:
        vars.append((feature, df[feature].var()))
    
    vars.sort(key=lambda x: x[1], reverse=True)
    with open(outfile, "a") as f:
        f.write("=========== VARIANCE OF FEATURES ===========\n")
        f.write("Feature, Variance\n")
        for var in vars:
            f.write(f"{var[0]},{var[1]}\n")
        f.write("\n============== END ==============\n")

def duration_checks(outfile, df):
    """
    Computes the mean and median duration of each regime in a time-ordered DataFrame.

    Parameters:
    - df (pd.DataFrame): DataFrame containing a column with regime labels.
    - regime_col (str): Name of the column containing regime labels.

    Returns:
    - None
    """
    regime_col = "regime"
    if regime_col not in df.columns:
        raise ValueError(f"Column '{regime_col}' not found in DataFrame.")

    # Detect regime changes
    df = df.clone()
    # not valid with polars, change this
    df = df.with_columns(pl.col(regime_col).shift().ne(pl.col(regime_col)).cum_sum().alias("regime_shift"))

    # Count duration of each regime period
    durations = df.group_by(['regime_shift', regime_col]).agg(pl.col("regime_shift").count().alias("duration"))

    # Compute mean and median durations per regime
    stats = durations.group_by(regime_col).agg([pl.col("duration").mean().alias("mean_duration"), pl.col("duration").median().alias("median_duration")])

    with open(outfile, "a") as f:
        f.write("\n=========== DURATION OF REGIMES ===========\n")
        f.write("Regime, Mean Duration, Median Duration\n")
        for row in stats.iter_rows(named=True):
            f.write(f"{row['regime']},{row['mean_duration']},{row['median_duration']}\n")
        f.write("\n============== END ==============\n")


def calculate_index_scores(outfile, df):
    # open outfile
    with open(outfile, "a") as f:

        df = df.drop(DROP_COLUMNS)
        if "timestamp_ns" in df.columns:
            df = df.drop("timestamp_ns")

        X = df.drop("regime")
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



def main(df):
    outfile = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}\\{REGIME_COUNT}\\information.txt"
    outfile_dir = os.path.dirname(outfile)
    if not os.path.exists(outfile_dir):
        os.makedirs(outfile_dir)
    other_outfile = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}_information.csv"
    calculate_index_scores(other_outfile, df)
    
    variance_checks(outfile, df)
    duration_checks(outfile, df)

    


if __name__ == "__main__":
    df = pl.read_csv("features_with_regimes.csv")
    main(df)

