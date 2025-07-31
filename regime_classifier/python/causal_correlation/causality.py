import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import pickle
from statsmodels.tsa.stattools import grangercausalitytests
from sklearn.preprocessing import StandardScaler
from constants import DROP_COLUMNS
from hmmlearn import hmm
from constants import PROJECT_ROOT
# from highfrequency import hayashi_yoshida

np.random.seed(45)


seconds = "0.50"
window_size = "7200"
rolling_window = "1200"
date_range = "20250505-20250506"

folder_name = "output_Sec" + seconds + "_Long" + window_size + "_Events" + rolling_window;

# === Configuration ===
SPY_MODEL_PATH = PROJECT_ROOT + "\\regime_classifier\\python\\data\\" + folder_name + "\\base_SPY\\5\\False_False\\model.pkl"
ES_MODEL_PATH = PROJECT_ROOT + "\\regime_classifier\\python\\data\\" + folder_name + "\\future_ES\\5\\False_False\\model.pkl"
SPY_DATA_PATH = PROJECT_ROOT + "\\data\\" + folder_name + "\\" + date_range + "\\base_SPY_norm.csv"
ES_DATA_PATH = PROJECT_ROOT + "\\data\\" + folder_name + "\\" + date_range + "\\future_ES_norm.csv"
TIMESTAMP_COL = "timestamp_ns"

# === Load Models ===
import joblib

spy_model = joblib.load(SPY_MODEL_PATH)
es_model = joblib.load(ES_MODEL_PATH)

# === Load Data ===
spy_df = pd.read_csv(SPY_DATA_PATH)
es_df = pd.read_csv(ES_DATA_PATH)

# =================================================================
# Granger Causality
# =================================================================


def granger_using_posteriors(spy_model, es_model, spy_df, es_df, timestamp_col="timestamp", drop_columns=[], max_lag=10):
    # === Extract Inputs ===
    spy_X = spy_df.drop(columns=drop_columns, errors='ignore').to_numpy()
    es_X = es_df.drop(columns=drop_columns, errors='ignore').to_numpy()

    # === Predict Posterior Probabilities ===
    spy_posteriors = spy_model.predict_proba(spy_X)
    es_posteriors = es_model.predict_proba(es_X)

    # === Wrap in DataFrames with Timestamps ===
    spy_post_df = pd.DataFrame(spy_posteriors, columns=[f"spy_p_{i}" for i in range(spy_posteriors.shape[1])])
    es_post_df = pd.DataFrame(es_posteriors, columns=[f"es_p_{i}" for i in range(es_posteriors.shape[1])])


    spy_post_df[TIMESTAMP_COL] = spy_df[TIMESTAMP_COL].values
    es_post_df[TIMESTAMP_COL] = es_df[TIMESTAMP_COL].values
    # Graph all posteriors
    plt.figure(figsize=(15, 5))
    plt.plot(spy_post_df[TIMESTAMP_COL], spy_post_df["spy_p_1"], label="spy_p_1")
    plt.plot(es_post_df[TIMESTAMP_COL], es_post_df["es_p_3"], label="es_p_3")
    plt.legend()
    plt.show()


    # shift es forward by one, drop LAST value and fill first with 0
    es_post_df = es_post_df.shift(-1).dropna().fillna(0)

    # === Align on Timestamp ===
    aligned = pd.concat([spy_post_df, es_post_df.drop(columns=TIMESTAMP_COL, errors='ignore')], axis=1).dropna()

    # === Granger Causality Tests: SPY ➝ ES ===
    print("=== Granger Causality: SPY ➝ ES ===")
    spy_cols = [col for col in aligned.columns if col.startswith("spy_p_")]
    es_cols = [col for col in aligned.columns if col.startswith("es_p_")]

    forward_results = []
    for spy_col in spy_cols:
        for es_col in es_cols:
            data = aligned[[es_col, spy_col]]
            try:
                test_result = grangercausalitytests(data, maxlag=max_lag, verbose=False)
                for lag in range(1, max_lag + 1):
                    pval = test_result[lag][0]["ssr_ftest"][1]
                    forward_results.append((spy_col, es_col, lag, pval))
            except Exception as e:
                forward_results.append((spy_col, es_col, None, f"Error: {e}"))

    df_forward = pd.DataFrame(forward_results, columns=["spy_p", "es_p", "lag", "p_value"])
    df_forward = df_forward[df_forward["p_value"].apply(lambda x: isinstance(x, float))]
    # sort by lag first, then by p_value
    df_forward = df_forward.sort_values(["lag", "p_value"], ascending=[True, True])

    print("\n=== Top SPY ➝ ES Causal Pairs ===")
    print(df_forward[df_forward["lag"] == 1].head(25).to_string(index=False))
    df_forward.to_csv("forward.csv", index=False)
    # === Granger Causality Tests: ES ➝ SPY ===
    print("\n=== Granger Causality: ES ➝ SPY ===")
    backward_results = []
    for es_col in es_cols:
        for spy_col in spy_cols:
            data = aligned[[spy_col, es_col]]
            try:
                test_result = grangercausalitytests(data, maxlag=max_lag, verbose=False)
                for lag in range(1, max_lag + 1):
                    pval = test_result[lag][0]["ssr_ftest"][1]
                    backward_results.append((es_col, spy_col, lag, pval))
            except Exception as e:
                backward_results.append((es_col, spy_col, None, f"Error: {e}"))

    df_backward = pd.DataFrame(backward_results, columns=["es_p", "spy_p", "lag", "p_value"])
    df_backward = df_backward[df_backward["p_value"].apply(lambda x: isinstance(x, float))]
    # sort by lag first, then by p_value
    df_backward = df_backward.sort_values(["lag", "p_value"], ascending=[True, True])

    print("\n=== Top ES ➝ SPY Causal Pairs ===")
    print(df_backward[df_backward["lag"] == 1].head(25).to_string(index=False))
    df_backward.to_csv("backward.csv", index=False)

granger_using_posteriors(spy_model, es_model, spy_df, es_df, timestamp_col=TIMESTAMP_COL, drop_columns=DROP_COLUMNS, max_lag=10)


# =================================================================
# Hayashi-Yoshida
# =================================================================

# def hayashi_yoshida_test(spy_df, es_df):
#     # 1. Ensure timestamp is datetime and sorted
#     spy_df['timestamp'] = pd.to_datetime(spy_df['timestamp'])
#     es_df['timestamp'] = pd.to_datetime(es_df['timestamp'])
#     spy_df = spy_df.sort_values('timestamp')
#     es_df = es_df.sort_values('timestamp')

#     print(spy_df.head())
#     print(es_df.head())

#     # 2. Convert regimes to events (i.e., changes)
#     def to_regime_events(df, col_name):
#         df = df.copy()
#         df['prev'] = df[col_name].shift(1)
#         df = df[df[col_name] != df['prev']].dropna()
#         return df[['timestamp', col_name]]

#     spy_events = to_regime_events(spy_df, "regime")
#     es_events = to_regime_events(es_df, "regime")

#     # 3. Prepare returns-style series for Hayashi-Yoshida: event timestamps + regime number
#     spy_times = spy_events['timestamp'].values
#     spy_vals = spy_events['regime'].astype(float).values
#     es_times = es_events['timestamp'].values
#     es_vals = es_events['regime'].astype(float).values

#     # 4. Run Hayashi-Yoshida test
#     cov, corr = hayashi_yoshida(spy_vals, spy_times, es_vals, es_times)

#     # === Results ===
#     print("=== Hayashi-Yoshida Estimation ===")
#     print(f"Covariance: {cov}")
#     print(f"Correlation: {corr}")

# hayashi_yoshida_test(spy_df, es_df)

# # === Cross-Correlation ===
# def lagged_cross_correlation(x, y, max_lag):
#     lags = np.arange(-max_lag, max_lag + 1)
#     ccs = [x.corr(y.shift(lag)) for lag in lags]
#     return lags, ccs

# lags, cc_vals = lagged_cross_correlation(aligned["spy_regime"], aligned["es_regime"], max_lag=20)

# plt.figure(figsize=(10, 4))
# plt.stem(lags, cc_vals, basefmt=" ")
# plt.title("Lagged Cross-Correlation: SPY vs ES")
# plt.xlabel("Lag (ES behind SPY → right)")
# plt.ylabel("Correlation")
# plt.grid(True)
# plt.tight_layout()
# plt.savefig("cross_correlation.png")
# plt.show()

# # === Co-Occurrence Heatmap ===
# cooccurrence = pd.crosstab(aligned["spy_regime"], aligned["es_regime"])
# plt.figure(figsize=(6, 5))
# sns.heatmap(cooccurrence, annot=True, fmt='d', cmap="YlGnBu")
# plt.title("Regime Co-Occurrence: SPY (rows) vs ES (cols)")
# plt.xlabel("ES Regime")
# plt.ylabel("SPY Regime")
# plt.tight_layout()
# plt.savefig("cooccurrence_heatmap.png")
# plt.show()
