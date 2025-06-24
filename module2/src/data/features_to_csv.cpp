#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include "timestamp_pipeline.hpp"
#include "feature_set.hpp"
#include "data_reciever.hpp"

namespace microregime {

// Custom DataReceiver that writes features to CSV files
class CsvWriter : public DataReciever {
public:
    CsvWriter(const std::string& base_filename) {
        // Create output directory if it doesn't exist
        std::filesystem::path dir = "output";
        std::filesystem::create_directories(dir);
        
        // Open CSV files for writing
        raw_csv_.open(dir / (base_filename + "_raw.csv"), std::ios::out);
        norm_short_csv_.open(dir / (base_filename + "_norm_short.csv"), std::ios::out);
        norm_long_csv_.open(dir / (base_filename + "_norm_long.csv"), std::ios::out);
        
        // Write CSV headers
        writeCsvHeader(raw_csv_);
        writeCsvHeader(norm_short_csv_);
        writeCsvHeader(norm_long_csv_);
    }
    
    ~CsvWriter() {
        if (raw_csv_.is_open()) raw_csv_.close();
        if (norm_short_csv_.is_open()) norm_short_csv_.close();
        if (norm_long_csv_.is_open()) norm_long_csv_.close();
    }
    
    void ingest_feature_set(const std::string& symbol,
                           uint64_t timestamp_ns,
                           const FeatureSet& raw_features,
                           const FeatureSet& normalized_short,
                           const FeatureSet& normalized_long) override {
        // Write data to respective CSV files
        writeFeatureSet(raw_csv_, timestamp_ns, raw_features);
        writeFeatureSet(norm_short_csv_, timestamp_ns, normalized_short);
        writeFeatureSet(norm_long_csv_, timestamp_ns, normalized_long);
    }

private:
    std::ofstream raw_csv_;
    std::ofstream norm_short_csv_;
    std::ofstream norm_long_csv_;
    
    void writeCsvHeader(std::ofstream& csv) {
        if (!csv.is_open()) return;
        
        // Common fields
        csv << "timestamp_ns,instrument,";
        
        // Feature fields
        csv << "log_spread,price_impact,log_return,ewm_volatility,realized_variance,"
            << "directional_volatility,spread_volatility,ofi,signed_volume_pressure,"
            << "order_arrival_rate,depth_imbalance,market_depth,lob_slope,price_gap,"
            << "tick_direction_entropy,reversal_rate,spread_crossing,aggressor_bias,"
            << "shannon_entropy,liquidity_stress\n";
    }
    
    void writeFeatureSet(std::ofstream& csv, uint64_t timestamp_ns, const FeatureSet& fs) {
        if (!csv.is_open()) return;
        
        // Write timestamp and instrument
        csv << timestamp_ns << "," << fs.instrument << ",";
        
        // Write all feature values
        csv << std::setprecision(15) << std::scientific
            << fs.log_spread << ","
            << fs.price_impact << ","
            << fs.log_return << ","
            << fs.ewm_volatility << ","
            << fs.realized_variance << ","
            << fs.directional_volatility << ","
            << fs.spread_volatility << ","
            << fs.ofi << ","
            << fs.signed_volume_pressure << ","
            << fs.order_arrival_rate << ","
            << fs.depth_imbalance << ","
            << fs.market_depth << ","
            << fs.lob_slope << ","
            << fs.price_gap << ","
            << fs.tick_direction_entropy << ","
            << fs.reversal_rate << ","
            << fs.spread_crossing << ","
            << fs.aggressor_bias << ","
            << fs.shannon_entropy << ","
            << fs.liquidity_stress << "\n";
    }
};

void run_feature_extraction(const std::string& timestamp,
                          const std::string& base_asset,
                          const std::string& future,
                          uint64_t snapshot_interval_ns) {
    // Create CSV writers for both instruments
    CsvWriter base_writer("base_" + base_asset);
    CsvWriter future_writer("future_" + future);
    
    // Create and run the pipeline
    TimestampPipeline pipeline(timestamp, base_asset, future);
    pipeline.run(snapshot_interval_ns, base_writer, future_writer);
}

} // namespace microregime

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] 
                  << " <timestamp> <base_asset> <future> [snapshot_interval_ns]\n"
                  << "Example: " << argv[0] << " 20230101 SPY ES 1000000000\n";
        return 1;
    }
    
    std::string timestamp = argv[1];
    std::string base_asset = argv[2];
    std::string future = argv[3];
    uint64_t snapshot_interval_ns = (argc > 4) ? std::stoull(argv[4]) : 1000000000; // Default 1 second
    
    try {
        microregime::run_feature_extraction(timestamp, base_asset, future, snapshot_interval_ns);
        std::cout << "Feature extraction completed successfully. Check the 'output' directory for CSV files.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}