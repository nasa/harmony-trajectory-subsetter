#-----------------------------------------------------------------------------
# CMakeList.txt
# https://google.github.io/googletest/quickstart-cmake.html
#-----------------------------------------------------------------------------


#-----------------------------------------------------------------------------
# Google tests define variables
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.14)
project(my_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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
add_compile_options(-v -DSDPS -DH5_USE_18_API -Og -Wno-vla-extension)
include_directories(${HDFEOS5_INCLUDE} ${HDF5_INCLUDE})
link_directories(${HDFEOS5_LIB} ${HDF5_LIB})

include(GoogleTest)

add_executable(subsetter_test
               gtest_utilities.cpp
               test_ForwardReferenceCoordinates.cpp
               test_IndexSelection.cpp
               test_ATL10_v005_Configuration.cpp
               test_ATL10_v006_Configuration.cpp
               ${SUBSETTER_DIR}/ProcessArguments.cpp
               test_ProcessArguments.cpp
)

target_link_libraries(subsetter_test
                      GTest::gtest_main
                      geotiff
                      tiff
                      jpeg
                      lzma
                      boost_program_options
                      boost_filesystem
                      boost_date_time
                      boost_regex
                      hdf5
                      hdf5_cpp
                      hdf5_hl
)

gtest_discover_tests(subsetter_test)

# A second executable had to be added due to global variable access.
# Both the Subsetter and ForwardReferenceCoordinates tests access the global
# variables Temporal::unixEpochTime and Coordinate::lookUpMap, resulting
# in duplicate linker symbols.
add_executable(subsetter_requiring_temporal_subsetting_test
               gtest_utilities.cpp
               test_Subsetter.cpp
               test_Temporal.cpp
               test_LogLevel.cpp
)

target_link_libraries(subsetter_requiring_temporal_subsetting_test
                      GTest::gtest_main
                      GTest::gmock_main
                      geotiff
                      tiff
                      jpeg
                      lzma
                      boost_program_options
                      boost_filesystem
                      boost_date_time
                      boost_regex
                      hdf5
                      hdf5_cpp
                      hdf5_hl
)

gtest_discover_tests(subsetter_requiring_temporal_subsetting_test)

#-----------------------------------------------------------------------------
# Run google test after building
#-----------------------------------------------------------------------------
add_custom_command(TARGET subsetter_test
                   subsetter_requiring_temporal_subsetting_test
                   POST_BUILD
                   COMMAND ctest)
