#!/bin/bash

# # Run rebuild.sh
# ./rebuild.sh

# List of dates to process
DATES=(
    "20250430" "20250501" "20250502"
    "20250505" "20250506" "20250507" "20250508" "20250509"
    "20250512" "20250513" "20250514" "20250515" "20250516"
    "20250519" "20250520" "20250521" "20250522" "20250523"
    "20250527" "20250528" "20250529" "20250530"
    "20250602" "20250603" "20250604" "20250605" "20250606"
    "20250609" "20250610" "20250611" "20250612" "20250613"
    "20250616" "20250617" "20250618" "20250620"
    "20250623" "20250624" "20250625" "20250626" "20250627"
    "20250630" "20250701" "20250702" "20250703"
    "20250707" "20250708" "20250709" "20250710" "20250711"
    "20250714" "20250715" "20250716" "20250717" "20250718"
    "20250721" "20250722" "20250723" "20250724" "20250725"
)

# Maximum number of parallel processes (adjust based on your CPU cores)
MAX_JOBS=8

echo "Starting parallel processing with max $MAX_JOBS concurrent jobs..."

# Array to store process IDs
declare -a PIDS=()

# Function to check and wait for processes to complete if we've hit the max
wait_if_busy() {
    while [ ${#PIDS[@]} -ge $MAX_JOBS ]; do
        # Check for completed processes
        for i in "${!PIDS[@]}"; do
            if ! kill -0 "${PIDS[$i]}" 2>/dev/null; then
                # Process has completed, remove from array
                unset "PIDS[$i]"
            fi
        done
        # Rebuild the array to remove empty indices
        PIDS=("${PIDS[@]}")
        
        # If we're still at max, wait a bit before checking again
        if [ ${#PIDS[@]} -ge $MAX_JOBS ]; then
            sleep 1
        fi
    done
}

# Process each date
for date in "${DATES[@]}"; do
    # Wait if we've hit the maximum number of parallel jobs
    wait_if_busy
    
    # Start the process in the background
    (
        echo "Starting processing for $date"
        C:/Users/jackd/OneDrive/Documents/MicroRegimeProject/build/feature_generation/Debug/features_to_csv.exe "$date" SPY ES
        echo "Finished processing $date"
    ) &
    
    # Store the process ID
    PIDS+=($!)
    echo "Started process for $date (PID: ${PIDS[-1]})"
done

# Wait for all remaining background processes to complete
wait

echo "All processing complete!"
