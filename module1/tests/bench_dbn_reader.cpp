#include <gtest/gtest.h>
#include <dbn_reader.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <iostream>

namespace fs = std::filesystem;
using namespace std::chrono;

class DbnReaderBenchmark : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        test_file_ = fs::path("C:\\Users\\jackd\\OneDrive\\Documents\\MicroRegimeProject\\data\\ES\\glbx-mdp3-20250505.mbo.dbn.zst");
        if (!fs::exists(test_file_)) {
            GTEST_SKIP() << "Test file not found: " << test_file_;
        }
        file_size_ = fs::file_size(test_file_);
    }

    static fs::path test_file_;
    static uintmax_t file_size_;
};

fs::path DbnReaderBenchmark::test_file_;
uintmax_t DbnReaderBenchmark::file_size_ = 0;

// Test DBN parsing speed
TEST_F(DbnReaderBenchmark, TestDbnParseSpeed) {
    const size_t max_events = 1000000000; // Process up to 100K events for benchmark
    size_t event_count = 0;
    
    auto start = high_resolution_clock::now();

    DbnMboReader reader(test_file_.string(), "ES");
    while (reader.has_next() && event_count < max_events) {
        auto event = reader.next_event();
        // Prevent compiler from optimizing out the read
        (void)event; // Mark as used
        event_count++;
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start).count();
    
    double seconds = duration / 1000.0;
    double events_per_second = event_count / seconds;
    
    std::cout << "Processed " << event_count << " events in " 
              << duration << "ms (" << events_per_second << " events/s)" << std::endl;
}
