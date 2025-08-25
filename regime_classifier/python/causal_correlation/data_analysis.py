import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.ensemble import RandomForestClassifier
from sklearn.feature_selection import mutual_info_classif
from sklearn.model_selection import TimeSeriesSplit
from statsmodels.tsa.stattools import adfuller
from scipy.stats import kruskal, pointbiserialr
import shap

import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from env import PROJECT_ROOT

from sklearn.cluster import KMeans
from sklearn.preprocessing import RobustScaler
from sklearn.pipeline import Pipeline

from sklearn.metrics import silhouette_score
from sklearn.preprocessing import RobustScaler
from sklearn.preprocessing import StandardScaler
import umap
from mpl_toolkits.mplot3d import Axes3D


DROP_COLUMNS = [#"timestamp_ns", "instrument", 
    "midprice", 
    "spread_mean",  
    "midprice_ema_ratio",  
    "midprice_ema_deviation",
    # "mean_log_return",

    "spread_volatility", 
    # "realized_variance", 
    "realized_variance_recent", 
    "realized_variance_bias",   
    # "directional_volatility",
    "vol_of_variance",

    "ofi", 
    "add_rate", 
    "trade_rate", 

    "trade_size_entropy",
    "aggressor_ratio", 
    "aggressor_bias",

    "obi", 
    "lob_bid_slope_mean", 
    "lob_ask_slope_mean", 

    # "tick_direction_entropy",
    "reversal_rate",
    "reversal_entropy",
    ]

def evaluate_regime_separation(features_df, regime_labels, sample_fraction=0.2):
    """
    Evaluates regime separation quality using multiple clustering metrics
    
    Parameters:
    - features_df: DataFrame of normalized features (WITHOUT regime labels)
    - regime_labels: Series of regime classifications
    - sample_fraction: Fraction of data to sample for efficiency (0-1)
    
    Returns:
    - dict: Dictionary containing different clustering metrics
    """
    # Select features that characterize regimes (avoid target leakage)
    analysis_features = features_df.drop(columns=DROP_COLUMNS)
    
    # Ensure indices align between features and labels
    common_idx = analysis_features.index.intersection(regime_labels.index)
    analysis_features = analysis_features.loc[common_idx]
    regime_labels = regime_labels.loc[common_idx]
    
    # Get unique regimes and check if we have enough for evaluation
    unique_regimes = regime_labels.unique()
    if len(unique_regimes) < 2:
        print(f"Warning: Only {len(unique_regimes)} regime(s) found. At least 2 regimes are needed for evaluation.")
        return None
    
    # Sample data for efficiency (maintain regime proportions)
    sample_idx = []
    min_samples = 5  # Minimum samples per regime for evaluation
    
    for regime in unique_regimes:
        regime_indices = regime_labels[regime_labels == regime].index
        sample_size = max(min_samples, int(len(regime_indices) * sample_fraction))
        sample_size = min(sample_size, len(regime_indices))
        
        if len(regime_indices) < min_samples:
            print(f"Warning: Regime {regime} has only {len(regime_indices)} samples (minimum {min_samples} needed).")
            continue
            
        sampled = np.random.choice(regime_indices, size=sample_size, replace=False)
        sample_idx.extend(sampled)
    
    if len(sample_idx) < 2 * min_samples:
        print("Error: Not enough samples across regimes for evaluation.")
        return None
    
    # Get the sampled data
    X = RobustScaler().fit_transform(analysis_features.loc[sample_idx])
    labels = regime_labels.loc[sample_idx]
    
    # Calculate multiple clustering metrics
    metrics = {}
    
    # Silhouette Score (higher is better, range: -1 to 1)
    try:
        metrics['silhouette_euclidean'] = silhouette_score(X, labels, metric='euclidean')
        metrics['silhouette_manhattan'] = silhouette_score(X, labels, metric='manhattan')
        metrics['silhouette_cosine'] = silhouette_score(X, labels, metric='cosine')
    except Exception as e:
        print(f"Silhouette score calculation failed: {str(e)}")
    
    # Calinski-Harabasz Index (higher is better)
    try:
        from sklearn.metrics import calinski_harabasz_score
        metrics['calinski_harabasz'] = calinski_harabasz_score(X, labels)
    except Exception as e:
        print(f"Calinski-Harabasz index calculation failed: {str(e)}")
    
    # Davies-Bouldin Index (lower is better, minimum 0)
    try:
        from sklearn.metrics import davies_bouldin_score
        metrics['davies_bouldin'] = davies_bouldin_score(X, labels)
    except Exception as e:
        print(f"Davies-Bouldin index calculation failed: {str(e)}")
    
    return metrics


def create_kmeans_regimes(features_df, n_clusters=3, random_state=42):
    """
    Creates regimes using K-Means on multiple microstructure features
    Returns: Series of regime labels aligned with original DataFrame index
    """
    # Select features capturing different market dimensions
    regime_features = features_df.drop(columns=DROP_COLUMNS)
    # [
    #     'realized_variance',      # Volatility
    #     'realized_variance_bias', # Volatility
    #     'spread_volatility',      # Volatility
    #     'obi',                    # Order Flow Imbalance
    #     'tick_direction_entropy', # Trade distribution
    #     'lob_bid_slope_mean',     # LOB shape
    #     'reversal_rate',          # Market resilience
    #     'aggressor_ratio'         # Trade direction
    # ]
    # Create pipeline with robust scaling
    pipeline = Pipeline([
        ('scaler', RobustScaler()),  # Less sensitive to outliers than StandardScaler
        ('kmeans', KMeans(n_clusters=n_clusters, 
                         random_state=random_state,
                         n_init=10))  # Explicitly set n_init to avoid warning
    ])
    
    # Fit and predict
    regimes = pipeline.fit_predict(regime_features)
    
    # Return as Series with original index
    return pd.Series(regimes, index=regime_features.index, name='regime')

from sklearn.mixture import BayesianGaussianMixture
from sklearn.preprocessing import PowerTransformer

def create_bgmm_regimes(features_df, max_clusters=5, random_state=42):
    """
    Creates probabilistic regimes using Bayesian GMM that:
    1. Automatically determines optimal cluster count
    2. Handles non-normal feature distributions
    3. Provides regime probabilities
    
    Parameters:
    -----------
    features_df : DataFrame
        Microstructure features (exclude target variables)
    max_clusters : int
        Maximum number of regimes to consider
    random_state : int
        Reproducibility seed
    
    Returns:
    --------
    Tuple of (regime_labels, regime_probabilities)
    """
    # Feature selection - focus on volatility and liquidity dimensions
    regime_features = features_df.drop(columns=["add_rate", "trade_rate", "ofi", "lob_ask_slope_mean", "lob_bid_slope_mean", "aggressor_ratio", "reversal_rate", "midprice_ema_ratio", "spread_mean", "midprice", "realized_variance_recent"])
    
    # Transform features to handle heavy tails
    preprocessor = Pipeline([
        ('scaler', RobustScaler()),
        ('power', PowerTransformer(method='yeo-johnson'))
    ])
    
    # Bayesian GMM with automatic cluster determination
    bgmm = BayesianGaussianMixture(
        n_components=max_clusters,
        covariance_type='full',
        weight_concentration_prior_type='dirichlet_process',
        weight_concentration_prior=0.01,  # Encourages fewer clusters
        max_iter=500,
        random_state=random_state,
        n_init=3
    )
    
    # Full pipeline
    pipeline = Pipeline([
        ('preprocess', preprocessor),
        ('bgmm', bgmm)
    ])
    
    # Fit and predict
    X_transformed = preprocessor.fit_transform(regime_features)
    bgmm.fit(X_transformed)
    
    # Get results
    regime_labels = pd.Series(
        bgmm.predict(X_transformed),
        index=regime_features.index,
        name='regime'
    )
    
    regime_probs = pd.DataFrame(
        bgmm.predict_proba(X_transformed),
        index=regime_features.index,
        columns=[f'regime_{i}_prob' for i in range(bgmm.n_components)]
    )
    
    return regime_labels, regime_probs

def load_and_concat_data(dates):
    """Load and concatenate data from multiple dates"""
    all_dfs = []
    folder_name = "output_Snapshot1.00_Window3600_Events150"
    
    for date in dates:
        FILE_PATH = os.path.join(PROJECT_ROOT, "data", folder_name, date, "base_SPY_norm.csv")
        if os.path.exists(FILE_PATH):
            try:
                df = pd.read_csv(FILE_PATH)
                if not df.empty:
                    df['date'] = date  # Add date identifier
                    # Ensure timestamp_ns is used as index for proper alignment
                    if 'timestamp_ns' in df.columns:
                        df = df.set_index('timestamp_ns')
                    all_dfs.append(df)
            except Exception as e:
                print(f"Error loading {FILE_PATH}: {str(e)}")
                continue
    
    if not all_dfs:
        raise ValueError("No valid data files found for the given dates")
    
    # Concatenate and ensure unique index
    combined = pd.concat(all_dfs, axis=0)
    
    # Reset index to make timestamp_ns a column again for sorting
    combined = combined.reset_index()
    
    # Sort by date and timestamp_ns
    combined = combined.sort_values(['date', 'timestamp_ns'])
    
    # Reset index to ensure clean index
    combined = combined.reset_index(drop=True)
    
    return combined

def feature_analysis(combined_df, n_clusters=3):
    # === Prepare Data ===
    drop_cols = ["timestamp_ns", "instrument", "date"]  # Added 'date' to drop cols
    features_df = combined_df.drop(columns=[col for col in drop_cols if col in combined_df.columns])

    WINSORIZE = True
    if WINSORIZE: # Clip at -3, 3. (features already are z-scored)
        features_df = features_df.clip(lower=-2.5, upper=2.5)
    
    # === Feature Evaluation Metrics ===
    metrics = {
        "Feature": [],
        "Mutual_Info": [],
        "RF_Importance": [],
        "Kruskal_H": [],
        "PointBiserial_r": [],
        "ADF_pvalue": [],
        "Lag1_ρ": [],
        "Delta_Std": []
    }
    
    # === Create Proxy Regimes ===
    # Using volatility-based regimes as proxy
    regimes = create_kmeans_regimes(features_df, n_clusters=n_clusters)
    # regimes, regime_probs = create_bgmm_regimes(features_df, max_clusters=n_clusters)
    features_df = features_df.join(regimes)  # Now features_df has 'regime' column
    
    # === Evaluate Regime Separation ===
    regime_metrics = evaluate_regime_separation(features_df=features_df, regime_labels=regimes)
    # evaluate_regime_separation(
    #     features_df=features_df,
    #     regime_labels=regimes,
    #     drop_cols=DROP_COLUMNS,      # list of columns to drop from features_df
    #     window=None,                   # non-overlap interval length (samples); set None for segment-level
    #     acf_lags=5,                   # number of autocorrelation lags to include in summaries
    #     subsample_gap=None,           # temporal thinning gap; None disables thinning
    #     n_bootstrap=200,              # number of bootstrap samples for CI
    #     block_size=10,                # block size for bootstrap (in intervals or segments)
    #     random_state=42
    # )

    if regime_metrics is not None:
        print("\n=== Regime Separation Metrics ===")
        for metric_name, metric_value in regime_metrics.items():
            print(f"{metric_name}: {metric_value}")
    else:
        print("\n=== Could not calculate regime separation metrics. Not enough regimes or samples. ===")
    plot_umap(features_df, regimes)
    
    # === Time Series Cross Validation ===
    tscv = TimeSeriesSplit(n_splits=5)
    
    # === Initialize SHAP ===
    explainer = None
    
    # === Evaluate Each Feature ===
    for feature in features_df.columns:
        if feature == "regime":
            continue
        X = features_df[[feature]].dropna()
        y = regimes.loc[X.index]
        
        # Skip if not enough data
        if len(X) < 100 or y.nunique() < 2:
            continue
            
        # 1. Mutual Information
        mi = mutual_info_classif(X, y, random_state=0)[0]
        
        # 2. Univariate RF Importance
        rf_scores = []
        for train_idx, test_idx in tscv.split(X):
            X_train, X_test = X.iloc[train_idx], X.iloc[test_idx]
            y_train, y_test = y.iloc[train_idx], y.iloc[test_idx]
            
            rf = RandomForestClassifier(n_estimators=50, max_depth=3)
            rf.fit(X_train, y_train)
            rf_scores.append(rf.score(X_test, y_test))
        
        # 3. Discriminatory Power (Kruskal-Wallis H)
        group_data = [X[y == i][feature] for i in range(n_clusters)]
        H, _ = kruskal(*group_data)
        
        # 4. Point-Biserial Correlation
        r, _ = pointbiserialr(y, X[feature])
        
        # 5. Stationarity (ADF)
        adf_pval = adfuller(X[feature], maxlag=1)[1]
        
        # 6. Lag-1 Autocorrelation
        lag1 = X[feature].autocorr(lag=1)
        
        # 7. Delta Std
        delta_std = X[feature].diff().std()
        
        # Store metrics
        metrics["Feature"].append(feature)
        metrics["Mutual_Info"].append(mi)
        metrics["RF_Importance"].append(np.mean(rf_scores))
        metrics["Kruskal_H"].append(H)
        metrics["PointBiserial_r"].append(r)
        metrics["ADF_pvalue"].append(adf_pval)
        metrics["Lag1_ρ"].append(lag1)
        metrics["Delta_Std"].append(delta_std)
        
        # Initialize SHAP explainer on first feature
        if explainer is None:
            rf = RandomForestClassifier(n_estimators=100, max_depth=5)
            rf.fit(features_df.dropna(), regimes.loc[features_df.dropna().index])
            explainer = shap.TreeExplainer(rf)
    
    # === Feature Importance DataFrame ===
    metrics_df = pd.DataFrame(metrics)
    
    # === SHAP Analysis ===
    shap_values = explainer.shap_values(features_df.dropna())
    
    # === Visualization ===
    plot_feature_analysis(metrics_df, shap_values, features_df.columns, features_df)
    return metrics_df

def plot_umap(features_df, regimes):
    X, y = features_df.drop(columns=["regime"]), regimes
    X.drop(columns=DROP_COLUMNS, errors="ignore", inplace=True)

    # Take every 20th sample
    X = X.iloc[::20]
    y = y.iloc[::20]

    # Print count of each regime
    print("=== Regime Counts ===")
    print(y.value_counts())

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
    plt.savefig("umap.png")
    plt.show()


def plot_feature_analysis(metrics_df, shap_values, feature_names, features_df):
    plt.figure(figsize=(14, 8))
    
    # Feature Importance Comparison
    plt.subplot(221)
    sns.barplot(x="Mutual_Info", y="Feature", 
                data=metrics_df.sort_values("Mutual_Info", ascending=False).head(15),
                palette="viridis")
    plt.title("Top 15 Features by Mutual Information")
    plt.xlabel("Mutual Information")
    
    # Feature Stability
    plt.subplot(222)
    sns.scatterplot(x="Lag1_ρ", y="Delta_Std", hue="ADF_pvalue", 
                    size="RF_Importance", data=metrics_df, palette="coolwarm")
    plt.title("Feature Stability Analysis")
    plt.xlabel("Lag-1 Autocorrelation")
    plt.ylabel("Delta Standard Deviation")
    
    # SHAP Summary
    plt.subplot(212)
    shap.summary_plot(shap_values, features_df.dropna(), feature_names=feature_names, 
                      plot_type="bar", show=False)
    plt.title("SHAP Feature Importance")
    plt.tight_layout()
    plt.savefig(os.path.join(PROJECT_ROOT, "regime_classifier", "python", "causal_correlation", "feature_analysis.png"))
    plt.close()
    
    # Correlation Matrix of Metrics
    plt.figure(figsize=(10, 8))
    corr_metrics = metrics_df.drop(columns="Feature").corr()
    sns.heatmap(corr_metrics, annot=True, cmap="icefire", center=0)
    plt.title("Metric Correlation Matrix")
    plt.tight_layout()
    plt.savefig(os.path.join(PROJECT_ROOT, "regime_classifier", "python", "causal_correlation", "metric_correlation.png"))
    plt.close()

# === Main Execution ===
if __name__ == "__main__":
    # List all dates you want to analyze
    dates = DATES = [
    "20250430", "20250501", "20250502", "20250505",
    # "20250505", "20250506", "20250507", "20250508", "20250509",
    # "20250512", "20250513", "20250514", "20250515", "20250516",
    # "20250519", "20250520", "20250521", "20250522", "20250523",
    "20250527", "20250528", "20250529", "20250530",
    # "20250602", "20250603", "20250604", "20250605", "20250606",
    # "20250609", "20250610", "20250611", "20250612", "20250613",
    # "20250616", "20250617", "20250618", "20250620",
    "20250623", "20250624", "20250625", "20250626", "20250627",
    # "20250630", "20250701", "20250702", "20250703",
    # "20250707", "20250708", "20250709", "20250710", "20250711",
    # "20250714", "20250715", "20250716", "20250717", "20250718",
    "20250721", "20250722", "20250723", "20250724", "20250725"
    ]  # Add more dates as needed
    
    # Load and concatenate all data
    combined_df = load_and_concat_data(dates)
    
    # Perform analysis on combined data
    results = feature_analysis(combined_df, n_clusters=3)
    print("\n=== Feature Evaluation Results ===")
    print(results.sort_values("Mutual_Info", ascending=False))
    
    # Save results
    output_path = os.path.join(PROJECT_ROOT, "regime_classifier", "python", "causal_correlation", "feature_metrics_combined.csv")
    results.to_csv(output_path, index=False)
