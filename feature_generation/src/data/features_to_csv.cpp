#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include "dual_feature_pipeline.hpp"
#include "feature_set.hpp"
#include "data_reciever.hpp"
#include "common_constants.hpp"
#include "environment.hpp"

namespace microregime {

// Custom DataReceiver that writes features to CSV files
class CsvWriter : public DataReciever {
public:
    CsvWriter(const std::string& base_filename, const uint64_t snapshot_interval_ns, const std::string& date) {
        // Create output directory if it doesn't exist
        std::string seconds = std::to_string(static_cast<double>(snapshot_interval_ns) / 1000000000);
        std::string dir_name = "output_Snapshot" + seconds + 
            "_Window" + std::to_string(WINDOW_SIZE) + 
            "_Events" + std::to_string(ROLLING_WINDOW);
        std::filesystem::path dir = get_project_root() / "data" / dir_name / date;
        std::cout << "Creating directory: " << dir.string() << std::endl;
        std::filesystem::create_directories(dir);
        
        // Open CSV files for writing
        raw_csv_.open(dir / (base_filename + "_raw.csv"), std::ios::out);
        norm_csv_.open(dir / (base_filename + "_norm.csv"), std::ios::out);
        
        // Write CSV headers
        writeCsvHeader(raw_csv_);
        writeCsvHeader(norm_csv_);
    }
    
    ~CsvWriter() {
        if (raw_csv_.is_open()) raw_csv_.close();
        if (norm_csv_.is_open()) norm_csv_.close();
    }
    
    void ingest_feature_set(const std::string& symbol,
                           uint64_t timestamp_ns,
                           const FeatureSet& raw_features,
                           const FeatureSet& normalized) override {
        // Write data to respective CSV files
        writeFeatureSet(raw_csv_, timestamp_ns, raw_features);
        writeFeatureSet(norm_csv_, timestamp_ns, normalized);
    }

private:
    std::ofstream raw_csv_;
    std::ofstream norm_csv_;
    
    void writeCsvHeader(std::ofstream& csv) {
        if (!csv.is_open()) return;
        
        // Common fields
        csv << "timestamp_ns,instrument,";
        
        // Feature fields
        csv << "midprice,"
            << "log_spread,"
            << "log_return,"
            << "ewm_volatility,"
            << "realized_variance,"
            << "directional_volatility,"
            << "spread_volatility,"
            << "ofi,"
            << "signed_volume_pressure,"
            << "order_arrival_rate,"
            << "depth_imbalance,"
            << "market_depth,"
            << "lob_slope,"
            << "price_gap,"
            << "tick_direction_entropy,"
            << "reversal_rate,"
            << "aggressor_bias,"
            << "shannon_entropy,"
            << "liquidity_stress\n";
    }
    
    void writeFeatureSet(std::ofstream& csv, uint64_t timestamp_ns, const FeatureSet& fs) {
        if (!csv.is_open()) return;
        
        // Write timestamp and instrument
        csv << timestamp_ns << "," << fs.instrument << ",";
        
        // Write all feature values
        csv << std::setprecision(15) << std::scientific
            << fs.midprice << ","
            << fs.log_spread << ","
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
    CsvWriter base_writer("base_" + base_asset, snapshot_interval_ns, timestamp);
    CsvWriter future_writer("future_" + future, snapshot_interval_ns, timestamp);
    
    // Create and run the pipeline
    DualFeaturePipeline pipeline(timestamp, base_asset, future);
    pipeline.run(snapshot_interval_ns, base_writer, future_writer);
}

} // namespace microregime

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] 
                  << " <timestamp> <base_asset> <future> [snapshot_interval_ns]\n"
                  << "Example: " << argv[0] << " 20250505 SPY ES 1000000000\n";
        return 1;
    }
    
    std::string timestamp = argv[1];
    std::string base_asset = argv[2];
    std::string future = argv[3];
    uint64_t snapshot_interval_ns = (argc > 4) ? std::stoull(argv[4]) : microregime::SNAPSHOT_INTERVAL_NS;
    
    try {
        microregime::run_feature_extraction(timestamp, base_asset, future, snapshot_interval_ns);
        std::cout << "Feature extraction completed successfully. Check the 'output' directory for CSV files.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}