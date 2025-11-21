#include <gtest/gtest.h>
#include <gtest/internal/gtest-filepath.h>

#include "../../../subsetter/LogLevel.h"
#include <iostream>

namespace
{
//
class test_LogLevel : public testing::Test
{
  protected:
    void SetUp()
    {
        // Redirect std::cout to captured_output
        oldCout = std::cout.rdbuf();
        std::cout.rdbuf(capturedOutput.rdbuf());
    }

    void TearDown() {}

    std::stringstream capturedOutput;
    std::streambuf *oldCout;
};

//
// Start LogLevel tests
//

// Test Set LogLevel(DEBUG)
TEST_F(test_LogLevel, set_debug_verify_output)
{
    SET_LOG_LEVEL("DEBUG");
    LOG_DEBUG("Verifying DEBUG level output");

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), "[DEBUG] Verifying DEBUG level output\n");
}

// Test Set LogLevel(DEBUG): Verify all log levels will prints
TEST_F(test_LogLevel, set_debug_verify_info_warning_error_critical_prints)
{
    SET_LOG_LEVEL("DEBUG");
    LOG_DEBUG("Verifying DEBUG level output");
    LOG_INFO("Verifying INFO level output");
    LOG_WARNING("Verifying WARNING level output");
    LOG_ERROR("Verifying ERROR level output");
    LOG_CRITICAL("Verifying CRITICAL level output");

    std::stringstream expectedOutput;
    expectedOutput << "[DEBUG] Verifying DEBUG level output\n"
                   << "[INFO] Verifying INFO level output\n"
                   << "[WARNING] Verifying WARNING level output\n"
                   << "[ERROR] Verifying ERROR level output\n"
                   << "[CRITICAL] Verifying CRITICAL level output\n";

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), expectedOutput.str());
}

// Test Set LogLevel(INFO)
TEST_F(test_LogLevel, set_info_verify_output)
{
    SET_LOG_LEVEL("INFO");
    LOG_INFO("Verifying INFO level output");

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), "[INFO] Verifying INFO level output\n");
}

// Test Set LogLevel(INFO): Verify ONLY INFO, WARNING, ERROR, CRITICAL prints
TEST_F(test_LogLevel, set_info_verify_info_warning_error_critical_prints)
{
    SET_LOG_LEVEL("INFO");
    LOG_DEBUG("Verifying DEBUG level output"); // Will not print
    LOG_INFO("Verifying INFO level output");
    LOG_WARNING("Verifying WARNING level output");
    LOG_ERROR("Verifying ERROR level output");
    LOG_CRITICAL("Verifying CRITICAL level output");

    std::stringstream expectedOutput;
    expectedOutput << "[INFO] Verifying INFO level output\n"
                   << "[WARNING] Verifying WARNING level output\n"
                   << "[ERROR] Verifying ERROR level output\n"
                   << "[CRITICAL] Verifying CRITICAL level output\n";

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), expectedOutput.str());
}

// Test Set LogLevel(WARNING)
TEST_F(test_LogLevel, set_warning_verify_output)
{
    SET_LOG_LEVEL("WARNING");
    LOG_WARNING("Verifying WARNING level output");

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(),
              "[WARNING] Verifying WARNING level output\n");
}

// Test Set LogLevel(WARNING): Verify ONLY WARNING, ERROR, CRITICAL prints
TEST_F(test_LogLevel, set_warning_verify_warning_error_critical_prints)
{
    SET_LOG_LEVEL("WARNING");
    LOG_DEBUG("Verifying DEBUG level output"); // Will not print
    LOG_INFO("Verifying INFO level output");   // Will not print
    LOG_WARNING("Verifying WARNING level output");
    LOG_ERROR("Verifying ERROR level output");
    LOG_CRITICAL("Verifying CRITICAL level output");

    std::stringstream expectedOutput;
    expectedOutput << "[WARNING] Verifying WARNING level output\n"
                   << "[ERROR] Verifying ERROR level output\n"
                   << "[CRITICAL] Verifying CRITICAL level output\n";

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), expectedOutput.str());
}

// Test Set LogLevel(ERROR)
TEST_F(test_LogLevel, set_error_verify_output)
{
    SET_LOG_LEVEL("ERROR");
    LOG_ERROR("Verifying ERROR level output");

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), "[ERROR] Verifying ERROR level output\n");
}

// Test Set LogLevel(ERROR): Verify ONLY ERROR and CRITICAL prints
TEST_F(test_LogLevel, set_error_verify_error_critical_prints)
{
    SET_LOG_LEVEL("ERROR");
    LOG_DEBUG("Verifying DEBUG level output");     // Will not print
    LOG_INFO("Verifying INFO level output");       // Will not print
    LOG_WARNING("Verifying WARNING level output"); // Will not print
    LOG_ERROR("Verifying ERROR level output");
    LOG_CRITICAL("Verifying CRITICAL level output");

    std::stringstream expectedOutput;
    expectedOutput << "[ERROR] Verifying ERROR level output\n"
                   << "[CRITICAL] Verifying CRITICAL level output\n";

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), expectedOutput.str());
}

// Test Set LogLevel(CRITICAL)
TEST_F(test_LogLevel, set_critical_verify_output)
{
    SET_LOG_LEVEL("CRITICAL");
    LOG_CRITICAL("Verifying CRITICAL level output");

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(),
              "[CRITICAL] Verifying CRITICAL level output\n");
}

// Test Set LogLevel(CRITICAL): Verify ONLY CRITICAL prints
TEST_F(test_LogLevel, set_critical_verify_critical_prints)
{
    SET_LOG_LEVEL("CRITICAL");
    LOG_DEBUG("Verifying DEBUG level output");     // Will not print
    LOG_INFO("Verifying INFO level output");       // Will not print
    LOG_WARNING("Verifying WARNING level output"); // Will not print
    LOG_ERROR("Verifying ERROR level output");     // Will not print
    LOG_CRITICAL("Verifying CRITICAL level output");

    std::stringstream expectedOutput;
    expectedOutput << "[CRITICAL] Verifying CRITICAL level output\n";

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), expectedOutput.str());
}

// Test Set LogLevel(WARNING): Verify ONLY WARNING, ERROR, CRITICAL prints
// Reset to LogLevel(DEBUG): Verify all log levels will prints
TEST_F(test_LogLevel, set_warning_verify_output_reset_debug_verify_output)
{
    SET_LOG_LEVEL("WARNING");
    LOG_DEBUG("Verifying DEBUG level output"); // Will not print
    LOG_INFO("Verifying INFO level output");   // Will not print
    LOG_WARNING("Verifying WARNING level output");
    LOG_ERROR("Verifying ERROR level output");
    LOG_CRITICAL("Verifying CRITICAL level output");

    std::stringstream expectedOutput;
    expectedOutput << "[WARNING] Verifying WARNING level output\n"
                   << "[ERROR] Verifying ERROR level output\n"
                   << "[CRITICAL] Verifying CRITICAL level output\n";

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), expectedOutput.str());

    // Clear output
    capturedOutput.str("");
    std::cout.rdbuf(capturedOutput.rdbuf());

    // Reset to DEBUG
    SET_LOG_LEVEL("DEBUG");
    LOG_DEBUG("Verifying DEBUG level output");
    LOG_INFO("Verifying INFO level output");
    LOG_WARNING("Verifying WARNING level output");
    LOG_ERROR("Verifying ERROR level output");
    LOG_CRITICAL("Verifying CRITICAL level output");

    std::stringstream expectedOutputDebug;
    expectedOutputDebug << "[DEBUG] Verifying DEBUG level output\n"
                        << "[INFO] Verifying INFO level output\n"
                        << "[WARNING] Verifying WARNING level output\n"
                        << "[ERROR] Verifying ERROR level output\n"
                        << "[CRITICAL] Verifying CRITICAL level output\n";

    // Restore std::cout
    std::cout.rdbuf(oldCout);

    // Assert the content of the captured output
    EXPECT_EQ(capturedOutput.str(), expectedOutputDebug.str());
}
} // namespace
