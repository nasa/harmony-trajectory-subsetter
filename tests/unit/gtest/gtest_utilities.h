#ifndef Gtest_utilities
#define Gtest_utilities

#include <string>

namespace gtest_utilities
{
std::string getFullPath(std::string relative_path);
int64_t *readDataset(std::string input_file, std::string dataset_name);

} // namespace gtest_utilities

#endif
