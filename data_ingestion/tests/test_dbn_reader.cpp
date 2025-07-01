#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <dbn_reader.hpp>

namespace fs = std::filesystem;

class DbnReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Path to the Zstandard-compressed MBO data file
        test_file_ = fs::path("..") / ".." / "data" / "ES" / "glbx-mdp3-20250505.mbo.dbn.zst";
        
        // Verify the test file exists
        if (!fs::exists(test_file_)) {
            test_file_ = fs::path("..") / "data" / "ES" / "glbx-mdp3-20250505.mbo.dbn.zst";
            if (!fs::exists(test_file_)) {
                GTEST_SKIP() << "Test file not found at either location:"
                           << "\n  " << fs::absolute("../../data/ES/glbx-mdp3-20250505.mbo.dbn.zst")
                           << "\n  " << fs::absolute("../data/ES/glbx-mdp3-20250505.mbo.dbn.zst");
            }
        }
        
        std::cout << "Using test file: " << fs::absolute(test_file_) << std::endl;
        std::cout << "File size: " << fs::file_size(test_file_) << " bytes" << std::endl;
    }
    
    fs::path test_file_;
};

TEST_F(DbnReaderTest, TestFileOpening) {
    try {
        DbnMboReader reader(test_file_.string(), "ES");
        SUCCEED();
    } catch (const std::exception& e) {
        FAIL() << "Failed to open DBN file: " << e.what();
    }
}

TEST_F(DbnReaderTest, TestReadEvents) {
    DbnMboReader reader(test_file_.string(), "ES");
    
    int event_count = 0;
    const int max_events_to_test = 1000; // Test first few events to keep tests fast
    
    while (reader.has_next() && event_count < max_events_to_test) {
        try {
            auto event = reader.next_event();
            // Basic validation of the event
            EXPECT_EQ(event.instrument, "ES") << "Unexpected instrument ID";
            EXPECT_GT(event.timestamp_ns, 0) << "Invalid timestamp";
            EXPECT_NE(event.action, '\0') << "Action should be set";
            
            // Log first few events for debugging
            if (event_count < 10) {
                std::cout << "Event " << event_count + 1 << ": "
                          << "ts=" << event.timestamp_ns
                          << " action=" << event.action
                          << " side=" << event.side
                          << " price=" << event.price
                          << " size=" << event.size
                          << " order_id=" << event.order_id
                          << std::endl;
            }
            event_count++;
        } catch (const std::exception& e) {
            FAIL() << "Error reading event: " << e.what();
        }
    }
    
    EXPECT_GT(event_count, 0) << "No events were read from the test file";
    EXPECT_EQ(event_count, reader.event_count()) << "Event count mismatch";
    
    std::cout << "Successfully read " << event_count << " events" << std::endl;
}


TEST_F(DbnReaderTest, TestNonExistentFile) {
    fs::path non_existent_file = "non_existent_file.dbn";
    
    EXPECT_THROW({
        DbnMboReader reader(non_existent_file.string(), "ES");
    }, databento::InvalidArgumentError) << "Should throw for non-existent file";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
