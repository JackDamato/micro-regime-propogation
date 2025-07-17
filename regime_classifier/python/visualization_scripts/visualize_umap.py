import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # Needed for 3D plotting
from sklearn.preprocessing import StandardScaler
import umap
import seaborn as sns
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, SHOW_GRAPHS, DROP_COLUMNS

def visualize_umap(file_name, outdir):
    # === Load CSVs ===
    df = pd.read_csv(file_name)

    # === Drop timestamp column if present ===
    df.drop(columns=["timestamp_ns"], errors="ignore", inplace=True)

    # === Extract Features and Labels (using long_ features) ===
    # Want to visualize with just long_ewm_volatility,long_realized_variance,long_directional_volatility,long_spread_volatility
    X, y = df.filter(like="long_"), df["regime"]
    for column in DROP_COLUMNS:
        X.drop(columns=["long_" + column], errors="ignore", inplace=True)
    print(X)

    # === Normalize Features ===
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)

    # === Configure 3D UMAP ===
    umap_3d = umap.UMAP(n_components=3, random_state=42, n_neighbors=50, min_dist=0.1, spread=5.0)

    # === Loop through each regime count and plot in 3D ===
    X_umap_3d = umap_3d.fit_transform(X_scaled)

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')
    scatter = ax.scatter(
        X_umap_3d[:, 0], X_umap_3d[:, 1], X_umap_3d[:, 2],
        c=y, cmap='tab10', s=10, alpha=0.7
    )
    ax.set_title("3D UMAP Projection - {} Regimes".format(len(y.unique())))
    ax.set_xlabel("UMAP 1")
    ax.set_ylabel("UMAP 2")
    ax.set_zlabel("UMAP 3")

    # Add legend
    handles, labels = scatter.legend_elements(prop="colors")
    legend_labels = sorted(set(y))
    ax.legend(handles, legend_labels, title="Regime", bbox_to_anchor=(1.05, 1), loc='upper left')
        
    plt.tight_layout()
    plt.savefig("{}/umap.png".format(outdir))
    if SHOW_GRAPHS:
        plt.show()
