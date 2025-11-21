#include <gtest/gtest.h>
#include <gtest/internal/gtest-filepath.h>

#include "../../../subsetter/Temporal.h"
#include <iostream>

namespace
{
//
class test_Temporal : public testing::Test
{
  protected:
    void SetUp() {}

    void TearDown() {}
};

//
// Start Temporal tests
//

// Test initalizing Temporal(std::string s, std::string e, std::string
// epochTime="") class with valid start_time/end_time string value
TEST_F(test_Temporal, initalize_temporal_with_valid_string_start_end_time)
{
    std::string start_time("2019-10-05T17:50:18.049908");
    std::string end_time("2019-10-05T17:51:59.116110");

    std::string expected_start_time("2019-10-05T17:50:18.049907");
    std::string expected_end_time("2019-10-05T17:51:59.116110");

    std::unique_ptr<Temporal> temporal =
        std::make_unique<Temporal>(start_time, end_time);
    EXPECT_EQ(temporal->getStartTime(), expected_start_time);
    EXPECT_EQ(temporal->getEndTime(), expected_end_time);
}

//
// Negative tests
//

// Test the exception handling within Temporal::convertToDateTimeString
// for boost::posix_time when initalizing Temporal(double s, double e).
// boost::posix_time will throw exception if start_time = 0 or end_time = 0
TEST_F(test_Temporal, initalize_temporal_with_zero_start_end_time)
{
    double start_time = 0;
    double end_time = 0;

    std::string expected_start_time("00:00:00.000000");
    std::string expected_end_time("00:00:00.000000");

    std::unique_ptr<Temporal> temporal =
        std::make_unique<Temporal>(start_time, end_time);
    EXPECT_EQ(temporal->getStartTime(), expected_start_time);
    EXPECT_EQ(temporal->getEndTime(), expected_end_time);
}

// Test the exception handling within Temporal::convertToDateTimeString
// for boost::posix_time when initalizing Temporal(double s, double e) class.
// boost::posix_time will throw exception myReferenceTime = ""
TEST_F(
    test_Temporal,
    initalize_temporal_with_valid_double_start_end_time_empty_myReferenceTime)
{
    double start_time = 55533018.049907923;
    double end_time = 55533119.116110086;

    std::string expected_start_time("00:00:00.000000");
    std::string expected_end_time("00:00:00.000000");

    std::unique_ptr<Temporal> temporal =
        std::make_unique<Temporal>(start_time, end_time);
    EXPECT_EQ(temporal->getStartTime(), expected_start_time)
        << temporal->getStartTime().c_str() << std::endl;
    EXPECT_EQ(temporal->getEndTime(), expected_end_time)
        << temporal->getEndTime().c_str() << std::endl;
}
} // namespace
