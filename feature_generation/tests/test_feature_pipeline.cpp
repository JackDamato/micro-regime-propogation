#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <string>
#include <deque>
#include <cmath>

#include <dual_feature_pipeline.hpp>
#include <feature_set.hpp>
#include <data_reciever.hpp>
#include <feature_map.hpp>

using namespace microregime;
namespace fs = std::filesystem;

// --- Helper to pretty-print FeatureSet ---
void print_features(const FeatureSet& fs, const std::string& label) {
    std::cout << "\n=== " << label << " ===\n";
    for (const auto& feature : kFeatures) {
        std::cout << feature.first << ": " << fs.*feature.second << "\n";
    }
}

// --- Custom DataReceiver for testing ---
class TestReceiver : public DataReciever {
public:
    struct SnapshotRecord {
        std::string symbol;
        uint64_t timestamp_ns;
        FeatureSet raw;
        FeatureSet norm;
    };

    std::deque<SnapshotRecord> snapshots;

    void ingest_feature_set(const std::string& symbol,
                            uint64_t timestamp_ns,
                            const FeatureSet& raw_features,
                            const FeatureSet& norm_features) override {
        snapshots.push_back({symbol, timestamp_ns, raw_features});
        print_features(raw_features, symbol);
        if (snapshots.size() > 100) {
            snapshots.pop_front();
        }
    }
};

TEST(DualFeaturePipelineTest, AlignedSnapshots) {
    // Setup: use 20250505 files, or skip test if not found
    // fs::path base_file = "../../data/SPY/glbx-mdp3-20250505.mbo.dbn.zst";
    // fs::path fut_file  = "../../data/ES/glbx-mdp3-20250505.mbo.dbn.zst";
    // if (!fs::exists(base_file) || !fs::exists(fut_file)) {
    //     GTEST_SKIP() << "Missing required data files";
    // }

    DualFeaturePipeline pipeline("20250505", "SPY", "ES");

    TestReceiver base_recv;
    TestReceiver fut_recv;

    pipeline.run(
        50'000'000'000,   // 50 s snapshot interval
        base_recv,
        fut_recv
    );

    ASSERT_EQ(base_recv.snapshots.size(), fut_recv.snapshots.size());

    size_t count = base_recv.snapshots.size();
    EXPECT_GT(count, 0);

    for (size_t i = 0; i < count; ++i) {
        auto base_ts = base_recv.snapshots[i].timestamp_ns;
        auto fut_ts  = fut_recv.snapshots[i].timestamp_ns;
        uint64_t delta = std::abs((int64_t)base_ts - (int64_t)fut_ts);

        EXPECT_LE(delta, 1'000'000) << "Timestamps misaligned at index " << i
                                 << " (Δ = " << delta << " ns)";
    }

    std::cout << "\nDualFeaturePipeline test completed.\n"
              << "- Aligned snapshots: " << count << "\n";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
