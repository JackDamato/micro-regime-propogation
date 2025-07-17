import numpy as np
import pandas as pd
from collections import defaultdict
from scipy.stats import chi2
from sklearn.metrics import mutual_info_score
from tqdm import tqdm
import joblib
from constants import DROP_COLUMNS

K_B = 5  # number of regimes for asset B
K_A = 5  # number of regimes for asset A
delta = 10     # lag

SPY_MODEL_PATH = "C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\regime_classifier\\python\\data\\output_0.50S_7200_1200_2000\\base_SPY\\5\\False_False\\model.pkl"
ES_MODEL_PATH = "C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\regime_classifier\\python\\data\\output_0.50S_7200_1200_2000\\future_ES\\5\\False_False\\model.pkl"
SPY_DATA_PATH = "C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\data\\output_0.50S_7200_1200_2000\\base_SPY_norm_long.csv"
ES_DATA_PATH = "C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\data\\output_0.50S_7200_1200_2000\\future_ES_norm_long.csv"

model_A = joblib.load(ES_MODEL_PATH)
model_B = joblib.load(SPY_MODEL_PATH)

df_A = pd.read_csv(ES_DATA_PATH)
df_B = pd.read_csv(SPY_DATA_PATH)

X_A = df_A.drop(columns=DROP_COLUMNS, errors='ignore').to_numpy()
X_B = df_B.drop(columns=DROP_COLUMNS, errors='ignore').to_numpy()

R_A = model_A.predict(X_A) # ES
R_B = model_B.predict(X_B) # SPY


T = len(R_B)
# Step 1: Count transitions for Model 0 (Markov on B only)
count_model_0 = np.zeros((K_B, K_B))
for t in range(1, T):
    prev_b = R_B[t - 1]
    curr_b = R_B[t]
    count_model_0[prev_b, curr_b] += 1

# Step 2: Count transitions for Model 1 (conditioned on B and lagged A)
count_model_1 = np.zeros((K_B, K_B, K_A))
for t in range(max(1, delta), T):
    prev_b = R_B[t - 1]
    curr_b = R_B[t]
    lagged_a = R_A[t - delta]
    count_model_1[prev_b, curr_b, lagged_a] += 1

# Step 3: Compute log-likelihoods
def log_likelihood_from_counts(counts, axis=1):
    total_log_likelihood = 0.0
    if counts.ndim == 2:
        row_sums = counts.sum(axis=axis, keepdims=True)
        probs = np.divide(counts, row_sums, out=np.zeros_like(counts), where=row_sums != 0)
        with np.errstate(divide='ignore', invalid='ignore'):
            log_probs = np.log(probs, where=(probs > 0))
        total_log_likelihood = np.sum(counts * log_probs)
    elif counts.ndim == 3:
        for k in range(counts.shape[2]):
            slice_ = counts[:, :, k]
            total_log_likelihood += log_likelihood_from_counts(slice_, axis=axis)
    return total_log_likelihood

logL0 = log_likelihood_from_counts(count_model_0)
logL1 = log_likelihood_from_counts(count_model_1)

# Step 4: Likelihood ratio test
Lambda = 2 * (logL1 - logL0)
df = (K_B - 1) * K_B * (K_A - 1)
p_value = 1 - chi2.cdf(Lambda, df)

print("Lambda:", Lambda)
print("df:", df)
print("p_value:", p_value)



def conditional_mutual_information(X, Y, Z):
    """
    Estimate the conditional mutual information I(X; Z | Y)
    where X, Y, Z are 1D integer-valued arrays of equal length.
    """
    yz = Y * (np.max(Z) + 1) + Z  # Joint encoding of (Y,Z)
    return mutual_info_score(X, yz) - mutual_info_score(X, Y)

def test_conditional_mutual_information(R_B, R_A, delta=3, num_permutations=1000, seed=42):
    """
    Compute CMI between R_B and R_A at lag delta, conditioned on R_B lag 1.
    Run a permutation test to compute p-value.

    Parameters:
    - R_B: np.array, regime sequence for target asset (e.g., SPY)
    - R_A: np.array, regime sequence for source asset (e.g., ES)
    - delta: int, lag between source and target
    - num_permutations: int, number of permutations for significance test

    Returns:
    - empirical_cmi: float
    - p_value: float
    """
    np.random.seed(seed)
    T = len(R_B)
    assert len(R_A) == T

    valid_start = max(1, delta)
    X = R_B[valid_start:]                    # R_t^{(B)}
    Y = R_B[valid_start - 1:T - 1]           # R_{t-1}^{(B)}
    Z = R_A[valid_start - delta:T - delta]   # R_{t-delta}^{(A)}
    
    assert len(X) == len(Y) == len(Z), "Length mismatch after alignment"

    empirical_cmi = conditional_mutual_information(X, Y, Z)

    permuted_cmis = []
    for _ in tqdm(range(num_permutations), desc=f"Permuting lag {delta}"):
        Z_perm = np.random.permutation(Z)
        perm_cmi = conditional_mutual_information(X, Y, Z_perm)
        permuted_cmis.append(perm_cmi)

    p_value = np.mean(np.array(permuted_cmis) >= empirical_cmi)
    return empirical_cmi, p_value


for lag in range(1, 6):
    cmi, pval = test_conditional_mutual_information(R_B, R_A, delta=lag)
    print(f"Lag {lag}: CMI = {cmi:.7f}, p-value = {pval:.9f}")