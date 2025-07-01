import pandas as pd
import numpy as np

def load_and_analyze_data(file_path):
    # Load the data
    print(f"Loading data from {file_path}...")
    df = pd.read_csv(file_path)
    
    # Get numeric columns (exclude 'timestamp_ns' and 'regime')
    numeric_cols = df.select_dtypes(include=[np.number]).columns.tolist()
    numeric_cols = [col for col in numeric_cols if col not in ['timestamp_ns', 'regime']]
    
    # Group by regime and calculate statistics
    print("\nAnalyzing data by regime...")
    for regime in sorted(df['regime'].unique()):
        regime_data = df[df['regime'] == regime][numeric_cols]
        print(f"\n{'='*50}")
        print(f"Regime {regime} - Basic Statistics")
        print(f"Number of samples: {len(regime_data)}")
        print("-"*30)
        
        # Calculate and display statistics
        stats = regime_data.describe().T
        stats['missing_%'] = (regime_data.isnull().mean() * 100).round(2)
        
        # Format the output for better readability
        pd.set_option('display.max_columns', None)
        pd.set_option('display.width', 1000)
        pd.set_option('display.precision', 4)
        
        print(stats)
    
    print("\nAnalysis complete!")

if __name__ == "__main__":
    input_file = "features_with_regimes.csv"
    load_and_analyze_data(input_file)