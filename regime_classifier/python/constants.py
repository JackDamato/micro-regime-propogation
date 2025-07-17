import os
# Data Constants

# output_timeS_longWindow_shortWindow_snapshotWindow
FOLDER_NAME = "output_0.50S_7200_1200_2000"

# future_ES or base_SPY
ASSET = os.getenv("ASSET", "base_SPY")
SHOW_GRAPHS = False


# Classifier Constants
def str_to_bool(value):
    if isinstance(value, bool):
        return value
    """Convert string to boolean, case-insensitive."""
    return value.lower() in ('true', '1', 't', 'y', 'yes')

# True for long_short, False for avg_diff
LONG_SHORT = False #str_to_bool(os.getenv("LONG_SHORT", False))
AVG = str_to_bool(os.getenv("AVG", False))
# LONG_SHORT --> long and short, !LONG_SHORT && !AVG --> long, !LONG_SHORT && AVG --> avg


PCA = False
PCA_VAR = 0.99
REGIME_COUNT = int(os.getenv("REGIME_COUNT", 3))



"""
["timestamp_ns","instrument",
"midprice","log_spread", "log_return",
"ewm_volatility","realized_variance","directional_volatility","spread_volatility",
"ofi","signed_volume_pressure","order_arrival_rate","depth_imbalance",
"market_depth","lob_slope","price_gap","tick_direction_entropy",
"reversal_rate","aggressor_bias","shannon_entropy","liquidity_stress"]
"""

DROP_COLUMNS = ["timestamp_ns", "instrument", "price_gap", "midprice",
    "log_spread", "log_return", "tick_direction_entropy", "ewm_volatility", "reversal_rate",
    "ofi", "depth_imbalance", "liquidity_stress", "lob_slope", "order_arrival_rate", "market_depth"]


"""
"""