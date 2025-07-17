#!/bin/bash

# Set the base directory
BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FULL_SCRIPT="$BASE_DIR/python_regime_full_script.sh"

# Make sure the full script is executable
chmod +x "$FULL_SCRIPT"

# Define the parameter combinations
ASSETS=("base_SPY" "future_ES")
LONG_SHORT_OPTS=("False")
AVG_OPTS=("True" "False")
REGIME_COUNTS=(3 4 5 6 7)

# Counter for total configurations
TOTAL_CONFIGS=$(( ${#ASSETS[@]} * ${#LONG_SHORT_OPTS[@]} * ${#AVG_OPTS[@]} * ${#REGIME_COUNTS[@]} ))
CURRENT_CONFIG=1

# Loop through all combinations
for asset in "${ASSETS[@]}"; do
    # make a csv file a write header "long_short,avg,regime_count,AIC,BIC,SIL,DB,CH"
    # Create output directory if it doesn't exist
    OUTPUT_DIR="C:/Users/jackd/OneDrive/Documents/MicroRegimeProject/regime_classifier/python"

    # Create CSV with header
    python -c "import pandas as pd
import os
output_path = os.path.join('$OUTPUT_DIR', '${asset}_information.csv')
pd.DataFrame(columns=['long_short', 'avg', 'regime_count', 'AIC', 'BIC', 'SIL', 'DB', 'CH']).to_csv(output_path, index=False)"

    for long_short in "${LONG_SHORT_OPTS[@]}"; do
        for avg_opt in "${AVG_OPTS[@]}"; do
            # Skip invalid combinations
            if [ "$long_short" = "True" ] && [ "$avg_opt" = "True" ]; then
                echo "Skipping invalid combination: LONG_SHORT=True and AVG=True"
                continue
            fi
            
            for regime_count in "${REGIME_COUNTS[@]}"; do
                # Create a unique identifier for this configuration
                CONFIG_ID="asset_${asset}_ls_${long_short}_avg_${avg_opt}_regimes_${regime_count}"
                
                echo "=================================================="
                echo "Running configuration $CURRENT_CONFIG of $TOTAL_CONFIGS"
                echo "Configuration: $CONFIG_ID"
                echo "=================================================="
                
                # Export the variables and run the full script
                export ASSET="$asset"
                export LONG_SHORT="$long_short"
                export AVG="$avg_opt"
                export REGIME_COUNT="$regime_count"
                
                # Run the full script
                "$FULL_SCRIPT"
                
                # Check if the script failed
                if [ $? -ne 0 ]; then
                    echo "Error encountered with configuration: $CONFIG_ID"
                    # Optionally: exit 1 to stop on first error
                    # exit 1
                fi
                
                ((CURRENT_CONFIG++))
                echo -e "\n\n"
            done
        done
    done
done

echo "All configurations completed!"