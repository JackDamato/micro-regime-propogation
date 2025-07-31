import os
# Data Constants

FOLDER_NAME = "output_Snapshot0.50_Window30000_Events1500"

# future_ES or base_SPY
ASSET = os.getenv("ASSET", "base_SPY")
SHOW_GRAPHS = True
PCA = False
PCA_VAR = 0.99
REGIME_COUNT = int(os.getenv("REGIME_COUNT", 4))

# THESE DATES WE WILL USE FOR PROCESSING.
# PROCESSED DATA ASSOCIATED WITH THIS WILL BE IN the FOLDER_NAME/<DATE>/file_name.csv
DATES = [
    "20250430", "20250501", "20250502", "20250505",
    "20250505", "20250506", "20250507", "20250508", "20250509",
    "20250512", "20250513", "20250514", "20250515", "20250516",
    "20250519", "20250520", "20250521", "20250522", "20250523",
    "20250527", "20250528", "20250529", "20250530",
    "20250602", "20250603", "20250604", "20250605", "20250606",
    "20250609", "20250610", "20250611", "20250612", "20250613",
    "20250616", "20250617", "20250618", "20250620",
    # "20250623", "20250624", "20250625", "20250626", "20250627",
    # "20250630", "20250701", "20250702", "20250703",
    # "20250707", "20250708", "20250709", "20250710", "20250711",
    # "20250714", "20250715", "20250716", "20250717", "20250718",
    # "20250721", "20250722", "20250723", "20250724", "20250725"
    ]


"""
Available Features as of Right now:

["timestamp_ns","instrument",
"midprice","log_spread", "log_return",
"ewm_volatility","realized_variance","directional_volatility","spread_volatility",
"ofi","signed_volume_pressure","order_arrival_rate","depth_imbalance",
"market_depth","lob_slope","price_gap","tick_direction_entropy",
"reversal_rate","aggressor_bias","shannon_entropy","liquidity_stress"]
"""

DROP_COLUMNS = ["timestamp_ns", "instrument", "aggressor_bias", "signed_volume_pressure", 
    "midprice", "market_depth", "price_gap", "order_arrival_rate", "reversal_rate",
    "log_spread", "log_return", "depth_imbalance", "lob_slope", "spread_volatility", "ofi", "liquidity_stress"]


"""
"""