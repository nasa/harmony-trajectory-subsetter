#include "gtest_utilities.h"
#include "H5Cpp.h"

#include <gtest/internal/gtest-filepath.h>

#include <algorithm>
#include <regex>
#include <string>

/**
 * @brief This function returns the full path of a given relative path.
 *
 *        All file paths are assumed to be relative to the Trajectory Subsetter
 *        root directory.
 *
 * @param relative_path The input relative file path.
 * @return The full file path.
 */
std::string gtest_utilities::getFullPath(std::string relative_path)
{
    std::string current_directory =
        testing::internal::FilePath::GetCurrentDir().c_str();
    std::string full_path = std::regex_replace(
        current_directory, std::regex("tests/unit/gtest/build"), relative_path);
    return full_path;
}

/**
 * @brief This function opens an HDF5 data file and extracts the specified
 *        dataset.
 *
 * @param input_file The input HDF5 file path.
 * @param dataset_name The dataset path (i.e., /gt1l/geolocation/ph_index_beg).
 * @return A pointer to the dataset data array.
 */
int64_t *gtest_utilities::readDataset(std::string input_file,
                                      std::string dataset_name)
{
    std::cout << "Reading in test data for " << input_file << ".\n";
    H5::H5File file(input_file, H5F_ACC_RDONLY);
    H5::DataSet *input_dataset =
        new H5::DataSet(file.openDataSet(dataset_name));
    size_t dataset_size = input_dataset->getSpace().getSimpleExtentNpoints();
    int64_t *dataset_array = new int64_t[dataset_size];
    input_dataset->read(dataset_array, input_dataset->getDataType());

    delete input_dataset;

    return dataset_array;
}
