# MicroRegime Python Research Components

This directory contains the Python-based research and analysis components for the MicroRegime project, focusing on regime classification and analysis of financial market data.

## Core Components

### 1. Configuration (`constants.py`)
- **Purpose**: Central configuration file for all research parameters
- **Key Settings**:
  - `FOLDER_NAME`: Defines the output directory structure pattern (format: `output_<seconds>s_L<long_window>_S<short_window>_E<snapshot_window>`)
  - `ASSET`: Specifies the asset being analyzed (e.g., `base_SPY`, `future_ES`)
  - `REGIME_COUNT`: Number of market regimes to identify
  - `LONG_SHORT`: Toggle between using long/short features or average features
  - `AVG`: Toggle for using averaged features
  - `PCA`: Enable/disable Principal Component Analysis
  - `PCA_VAR`: Variance threshold for PCA

### 2. Main Scripts

#### `classifier.py`
- Performs regime classification using Hidden Markov Models (HMM) from hmmlearn
- Input: Normalized feature CSVs from data pipeline
- Output: Regime predictions and model artifacts

#### `data_checks.py`
- Performs validation and quality checks on the input data
- Calculates various statistical measures and validation metrics
- Outputs diagnostic information and validation results

#### `visualize.py`
- Generates visualizations of the identified regimes
- Creates heatmaps, time series plots, and other analytical visualizations
- Outputs plots to the specified results directory

### 3. Visualization Scripts (`visualization_scripts/`)
- Contains modular visualization components
- Implements various plotting functions for regime analysis
- Includes t-SNE and UMAP visualizations for high-dimensional data

### 4. Causal Analysis (`causal_correlation/`)
- Implements causal inference methods
- Analyzes relationships between different market features
- Helps understand the drivers of different market regimes

## Directory Structure

```
regime_classifier/python/
├── constants.py           # Configuration settings
├── classifier.py          # Main classification script
├── data_checks.py         # Data validation and metrics
├── visualize.py           # Visualization entry point
├── env.py                 # Environment configuration
├── visualization_scripts/ # Visualization components
│   ├── visualize_tsne.py
│   ├── visualize_umap.py
│   └── visualize_heatmaps.py
├── causal_correlation/    # Causal analysis
│   ├── causality.py
│   └── ...
└── run_outputs/           # Output directory
    └── {FOLDER_NAME}/     # Auto-created based on config
        └── {ASSET}/       # Per-asset results
            └── {REGIME_COUNT}/  # Per-regime-count results
                ├── {LONG_SHORT}_{AVG}/  # Configuration-specific results
                │   ├── information.txt   # Summary metrics
                │   ├── model.pkl         # Trained model
                │   └── plots/            # Generated visualizations
                └── ...
```

## Automation Scripts

### 1. `python_regime_full_script.sh`
- **Purpose**: Single-run script for a specific configuration
- **Workflow**:
  1. Creates necessary output directories
  2. Runs classifier with current settings
  3. Performs data validation checks
  4. Generates visualizations
- **Usage**: Configure `constants.py` first, then run this script

### 2. `python_multi_config.sh`
- **Purpose**: Run multiple configurations in sequence
- **Features**:
  - Tests different combinations of parameters
  - Automatically generates comparison metrics
  - Organizes outputs by configuration
- **Configurable Parameters**:
  - Assets (e.g., SPY, ES)
  - Feature types (long/short vs average)
  - Number of regimes
  - Other model parameters

## Workflow

1. **Configure** `constants.py` with desired parameters
2. **Run** either:
   - Single configuration: `python_regime_full_script.sh`
   - Multiple configurations: `python_multi_config.sh`
3. **Analyze** results in the `run_outputs` directory
4. **Visualize** findings using the generated plots and metrics

## Important Notes

- The `FOLDER_NAME` in `constants.py` determines where all outputs are saved
- Results are organized hierarchically by configuration
- Always check the `information.txt` files for summary metrics
- Visualizations are saved in the respective configuration's directory
- The system is designed to be modular - you can run components independently if needed