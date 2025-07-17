from visualization_scripts.visualize_heatmaps import regime_data_analysis
from visualization_scripts.visualize_umap import visualize_umap
from visualization_scripts.visualize_tsne import visualize_tsne
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, LONG_SHORT, AVG
from env import PROJECT_ROOT

outdir = f"{PROJECT_ROOT}\\regime_classifier\\python\\run_outputs\\{FOLDER_NAME}\\{ASSET}\\{REGIME_COUNT}\\{LONG_SHORT}_{AVG}"
regime_data_analysis("features_with_regimes.csv", outdir)
# visualize_umap("features_with_regimes.csv", outdir)
# visualize_tsne("features_with_regimes.csv", outdir)
