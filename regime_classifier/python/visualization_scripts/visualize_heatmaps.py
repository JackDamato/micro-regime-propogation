import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, SHOW_GRAPHS, AVG, LONG_SHORT

def plot_heatmap(data, title, counts, outdir):
    """
    Plot a heatmap of the provided data.

    :param data: DataFrame of data to plot
    :param title: Title of the plot
    """
    plt.figure(figsize=(12, 16))  # Adjusted dimensions for vertical layout
    sns.heatmap(data.T, annot=False, cmap="coolwarm", center=0, vmin=-1.5, vmax=1.5, cbar_kws={"label": "Mean Z-score"})
    plt.title("Feature Heatmap Across Regimes ({})".format(title))
    plt.xlabel("Regime")
    plt.ylabel("Feature")
    plt.xticks(rotation=45, ha='right')

    # Add regime counts
    for i, count in enumerate(counts):
        plt.text(i + 0.5, -1, f"n={count}", ha='center', va='bottom')
    plt.tight_layout()

    plt.savefig(f"{outdir}/feature_heatmap_{title}.png")
    plt.close()
    # if SHOW_GRAPHS:
    #     plt.show()

def regime_data_analysis(file_name, outdir):
    """
    Main function to analyze regime data.

    :param file_name: Name of the CSV file to load
    """
    # --- Load data ---
    df = pd.read_csv(file_name)

    # Split into short and long features
    counts = df["regime"].value_counts().sort_index()
    short_features = df.filter(like="short_")
    long_features = df.filter(like="long_")
    short_features["regime"] = df["regime"]
    long_features["regime"] = df["regime"]

    # Calculate means
    short_feature_means = short_features.groupby("regime").mean()
    long_feature_means = long_features.groupby("regime").mean()

    # remove prefixes so we can actually subtract
    long_features.columns = long_features.columns.str.replace("long_", "")
    short_features.columns = short_features.columns.str.replace("short_", "")


    # Calculate averages
    avg_features = (long_features + short_features) / 2
    avg_features["regime"] = df["regime"]
    avg_feature_means = avg_features.groupby("regime").mean()
    
    # Plot heatmaps
    plot_heatmap(long_feature_means, "long", counts, outdir)
    plot_heatmap(short_feature_means, "short", counts, outdir)
    plot_heatmap(avg_feature_means, "avg", counts, outdir)