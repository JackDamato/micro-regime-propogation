#!/bin/bash

# CD into the folder
pushd "C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\regime_classifier\\python"

# If C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\regime_classifier\\python\\data\\{FOLDER_NAME}\\{ASSET}\\{REGIME_COUNT} doesn't exist make it
# All files should be output to data/{FOLDER_NAME}/{ASSET}/{REGIME_COUNT}/
python -c "
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, LONG_SHORT, AVG; 
import os; 
folder = f\"data/{FOLDER_NAME}/{ASSET}/{REGIME_COUNT}/{LONG_SHORT}_{AVG}\"; 
if not os.path.exists(folder): os.makedirs(folder)
"

# Ensure Constants are set properly in constants.py
# Run classifier.py then run data_checks.py then run visualize.py
python classifier.py
python -c "
from constants import FOLDER_NAME, ASSET, REGIME_COUNT, LONG_SHORT, AVG; 
import os; 
file = f\"data/{FOLDER_NAME}/{ASSET}/{REGIME_COUNT}/{LONG_SHORT}_{AVG}/information.txt\"; 
if not os.path.exists(file): open(file, 'w').close()
"
# in series:
python data_checks.py
python visualize.py

# in parallel:
# # Run first script in background, redirect output to log
# python data_checks.py > data_checks.log 2>&1 &
# pid1=$!  # Save the process ID

# # Run second script in background, redirect output to log
# python visualize.py > visualize.log 2>&1 &
# pid2=$!  # Save the process ID

# # Wait for both processes to complete
# # wait will return the exit status of the last process that exits
# wait $pid1 $pid2

# # Check if either process failed
# if [ $? -ne 0 ] || ! kill -0 $pid1 2>/dev/null || ! kill -0 $pid2 2>/dev/null; then
#     echo "Error: One or both scripts failed to complete successfully"
#     exit 1
# fi

popd
