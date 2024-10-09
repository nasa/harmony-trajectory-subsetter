#-----------------------------------------------------------------------------
# CMakeList.txt
# https://google.github.io/googletest/quickstart-cmake.html
#-----------------------------------------------------------------------------


#-----------------------------------------------------------------------------
# Google tests define variables
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.14)
project(my_project)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()


#-----------------------------------------------------------------------------
# Define some CMake variables for use later in the project
#-----------------------------------------------------------------------------
set (SUBSETTER_DIR            ${CMAKE_SOURCE_DIR}/../../../subsetter)
set (HDFEOS5_INCLUDE          ${SUBSETTER_DIR}/hdfeos5/include)
set (HDFEOS5_LIB              ${SUBSETTER_DIR}/hdfeos5/lib)
set (HDF5_INCLUDE             ${CONDA_PREFIX}/include)
set (HDF5_LIB                 ${CONDA_PREFIX}/lib)

#-----------------------------------------------------------------------------
# Define compile options, include, and libraries to link in the project
#-----------------------------------------------------------------------------
add_compile_options(-v -DSDPS -DH5_USE_18_API -Og)
include_directories(${HDFEOS5_INCLUDE} ${HDF5_INCLUDE})
link_directories(${HDFEOS5_LIB} ${HDF5_LIB})

add_executable(
  subsetter_test
  test_ForwardReferenceCoordinates.cpp
  gtest_utilities.cpp
  test_IndexSelection.cpp
)
target_link_libraries(
  subsetter_test
  GTest::gtest_main
  geotiff
  tiff
  jpeg
  Gctp
  lzma
  boost_program_options
  boost_filesystem
  boost_date_time
  boost_regex
  hdf5
  hdf5_cpp
  hdf5_hl
)

include(GoogleTest)
gtest_discover_tests(subsetter_test)


#-----------------------------------------------------------------------------
# Run google test after building
#-----------------------------------------------------------------------------
add_custom_command(TARGET subsetter_test
                   POST_BUILD
                   COMMAND ctest)


