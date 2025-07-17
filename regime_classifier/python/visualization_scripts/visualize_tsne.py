import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.preprocessing import StandardScaler
from sklearn.manifold import TSNE
from mpl_toolkits.mplot3d import Axes3D  # required for 3D projection
import seaborn as sns
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, SHOW_GRAPHS, DROP_COLUMNS
from env import PROJECT_ROOT

def visualize_tsne(file_name):
    # Load your CSVs (change filenames if needed)
    df = pd.read_csv(file_name)

    # Load and clean
    df.drop(columns=["timestamp_ns"], errors="ignore", inplace=True)
    
    # Extract long features only
    X = df.drop(columns=["regime"]).filter(like="long_")
    for column in DROP_COLUMNS:
        X.drop(columns=["long_" + column], errors="ignore", inplace=True)

    y = df["regime"]

    print(f"Processing: {len(y.unique())}")
    
    # Normalize features
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)
    
    # t-SNE to 3D
    tsne = TSNE(n_components=3, perplexity=30, learning_rate=200, max_iter=400, random_state=42)
    X_tsne = tsne.fit_transform(X_scaled)
    
    # Plot
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    scatter = ax.scatter(X_tsne[:, 0], X_tsne[:, 1], X_tsne[:, 2],
                        c=y, cmap="tab10", s=5, alpha=0.7)

    ax.set_title(f"3D t-SNE Projection - {len(y.unique())}")
    ax.set_xlabel("t-SNE 1")
    ax.set_ylabel("t-SNE 2")
    ax.set_zlabel("t-SNE 3")

    legend = ax.legend(*scatter.legend_elements(), title="Regime")
    ax.add_artist(legend)

    plt.tight_layout()
    plt.savefig("{}\\regime_classifier\\python\\run_outputs\\{}\\{}\\{}\\tsne.png".format(PROJECT_ROOT, FOLDER_NAME, ASSET, REGIME_COUNT))
    if SHOW_GRAPHS:
        plt.show()
