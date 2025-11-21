#include "../../../subsetter/ProcessArguments.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

namespace
{
class test_ProcessArguments : public testing::Test, public ProcessArguments
{
  protected:
    void SetUp()
    {
        // Create Added shared pointer for test modules to invoke
        // ProcessArguments::process_args() method
        processArgs = std::make_shared<ProcessArguments>();

        // Generate a fake .h5 file with unique path within the temporary
        // directory
        temp_file_path =
            std::filesystem::temp_directory_path() / "fake_file.h5";

        // Create a temp file
        std::ofstream temp_file(temp_file_path);
        if (temp_file.is_open())
        {
            temp_file << "This is a temporary file." << std::endl;
            temp_file.close();
        }

        // Generate a fake .geojson file
        geojson_temp_file_path =
            std::filesystem::temp_directory_path() / "Tasmania_sliver.geojson";

        // Create and write geojson data for tests modules to invoke
        std::ofstream geojson_temp_file(geojson_temp_file_path);
        if (geojson_temp_file.is_open())
        {
            geojson_temp_file
                << "{\"type\": \"FeatureCollection\",\"features\": [{\"type\": "
                   "\"Feature\",\"properties\": {},\"geometry\": "
                   "{\"coordinates\": "
                   "[[[145.0029842290342,-41.80315161643407],[145."
                   "18049209479472,-42."
                   "111337298683694],[148.37787460845385,-42.12167177793065],["
                   "148."
                   "3253351836085, -41.73567040285831],[145.0029842290342, "
                   "-41.80315161643407]]],\"type\": \"Polygon\"}}]}"
                << std::endl;
            geojson_temp_file.close();
        }
    }

    void TearDown() {}

  protected:
    std::filesystem::path temp_file_path;
    std::filesystem::path geojson_temp_file_path;
    std::shared_ptr<ProcessArguments> processArgs;
};

//
// Start ProcessArguments tests
//
// Example ./subset
// --configfile ../harmony_service/subsetter_config.json
// --filename /home/vtran11/Downloads/ATL10-02_20181014123806_02430101_005_01.h5
// --outfile
// /home/vtran11/workspace/DAS-2247-trajsub-ATL10v06/subsetter/processed_ATL10-02_20181014123806_02430101_005_01.h5
//

TEST_F(test_ProcessArguments, test_process_args_simple)
{
    std::vector<std::string> arguments = {
        "--configfile",
        "../../../harmony_service/subsetter_config.json",
        "--filename",
        temp_file_path.string(),
        "--outfile",
        "subset_fake_file.h5"};

    // Build arguments string for processArgs->process_args() input
    std::vector<char *> argv;
    for (const auto &arg : arguments)
        argv.push_back(const_cast<char *>(arg.c_str()));

    int results = processArgs->process_args(argv.size(), argv.data());
    EXPECT_EQ(results, ProcessArguments::PASS);
}

// Test bounding box with inline .geojson name attribute
TEST_F(test_ProcessArguments,
       test_process_args_bounding_box_inline_geojson_name)
{
    std::stringstream boundingshape(
        "{\"name\": \"AOI_icesat2.geojson\", \"type\": \"FeatureCollection\", "
        "\"features\": [{\"type\": \"Feature\", \"geometry\": {\"type\": "
        "\"Polygon\", \"coordinates\": [[[7.283062, 3.533509], [7.280999, "
        "4.517362], [6.300962, 4.514375], [6.304198, 3.531174], [7.283062, "
        "3.533509]]]}, \"properties\": {\"edscId\": \"0\"}}]}");
    std::vector<std::string> arguments = {
        "--configfile",
        "../../../harmony_service/subsetter_config.json",
        "--filename",
        temp_file_path.string(),
        "--outfile",
        "subset_fake_file.h5",
        "--boundingshape",
        boundingshape.str()};

    // Build arguments string for processArgs->process_args() input
    std::vector<char *> argv;
    for (const auto &arg : arguments)
        argv.push_back(const_cast<char *>(arg.c_str()));

    int results = processArgs->process_args(argv.size(), argv.data());
    EXPECT_EQ(results, ProcessArguments::PASS);

    boost::property_tree::ptree boundingShapePt =
        processArgs->getBoundingShapePt();

    boost::property_tree::ptree expected;
    property_tree::read_json(boundingshape, expected);
    ASSERT_TRUE(boundingShapePt == expected);
}

// Test bounding box without inline .geojson name attribute
TEST_F(test_ProcessArguments,
       test_process_args_bounding_box_without_inline_geojson_name)
{
    std::stringstream boundingshape(
        "{\"type\": \"FeatureCollection\", \"features\": [{\"type\": "
        "\"Feature\", \"geometry\": {\"type\": \"Polygon\", \"coordinates\": "
        "[[[7.283062, 3.533509], [7.280999, 4.517362], [6.300962, 4.514375], "
        "[6.304198, 3.531174], [7.283062, 3.533509]]]}, \"properties\": "
        "{\"edscId\": \"0\"}}]}");
    std::vector<std::string> arguments = {
        "--configfile",
        "../../../harmony_service/subsetter_config.json",
        "--filename",
        temp_file_path.string(),
        "--outfile",
        "subset_fake_file.h5",
        "--boundingshape",
        boundingshape.str()};

    // Build arguments string for processArgs->process_args() input
    std::vector<char *> argv;
    for (const auto &arg : arguments)
        argv.push_back(const_cast<char *>(arg.c_str()));

    int results = processArgs->process_args(argv.size(), argv.data());
    EXPECT_EQ(results, ProcessArguments::PASS);

    boost::property_tree::ptree boundingShapePt =
        processArgs->getBoundingShapePt();

    boost::property_tree::ptree expected;
    property_tree::read_json(boundingshape, expected);
    ASSERT_TRUE(boundingShapePt == expected);
}

// Test bounding box with valid .geojson file
TEST_F(test_ProcessArguments, test_process_args_bounding_box_valid_geojson_file)
{
    std::vector<std::string> arguments = {
        "--configfile",
        "../../../harmony_service/subsetter_config.json",
        "--filename",
        temp_file_path.string(),
        "--outfile",
        "subset_fake_file.h5",
        "--boundingshape",
        geojson_temp_file_path.string()};

    // Build arguments string for processArgs->process_args() input
    std::vector<char *> argv;
    for (const auto &arg : arguments)
        argv.push_back(const_cast<char *>(arg.c_str()));

    int results = processArgs->process_args(argv.size(), argv.data());
    EXPECT_EQ(results, ProcessArguments::PASS);

    boost::property_tree::ptree boundingShapePt =
        processArgs->getBoundingShapePt();

    boost::property_tree::ptree expected;
    property_tree::read_json(geojson_temp_file_path.string(), expected);
    ASSERT_TRUE(boundingShapePt == expected);
}

// Test bounding box without .geojson file created
TEST_F(test_ProcessArguments,
       test_process_args_bounding_box_no_geojson_file_created)
{
    std::vector<std::string> arguments = {
        "--configfile",
        "../../../harmony_service/subsetter_config.json",
        "--filename",
        temp_file_path.string(),
        "--outfile",
        "subset_fake_file.h5",
        "--boundingshape",
        "no_file_create.geojson"};

    // Build arguments string for processArgs->process_args() input
    std::vector<char *> argv;
    for (const auto &arg : arguments)
        argv.push_back(const_cast<char *>(arg.c_str()));

    int results = processArgs->process_args(argv.size(), argv.data());
    EXPECT_EQ(results, ProcessArguments::ERROR);
}

// Test bounding box witout data
TEST_F(test_ProcessArguments, test_process_args_bounding_box_no_data)
{
    std::vector<std::string> arguments = {
        "--configfile",
        "../../../harmony_service/subsetter_config.json",
        "--filename",
        temp_file_path.string(),
        "--outfile",
        "subset_fake_file.h5",
        "--boundingshape",
        ""};

    // Build arguments string for processArgs->process_args() input
    std::vector<char *> argv;
    for (const auto &arg : arguments)
        argv.push_back(const_cast<char *>(arg.c_str()));

    int results = processArgs->process_args(argv.size(), argv.data());
    EXPECT_EQ(results, ProcessArguments::ERROR);
}

} // namespace
