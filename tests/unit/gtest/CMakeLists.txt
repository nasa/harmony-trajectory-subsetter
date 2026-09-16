#-----------------------------------------------------------------------------
# CMakeList.txt
# https://google.github.io/googletest/quickstart-cmake.html
#
# This project builds in two environments:
#
# * The docker/gtest.Dockerfile image (see bin/build-gtest and bin/run-gtest),
#   which extends the service image's builder stage: system GoogleTest and
#   Boost, and HDF5 built from source into /usr/local.
# * The local conda environment described by environment.yaml.
#
# Neither environment needs extra CMake flags. GoogleTest, HDF5, Boost,
# libtiff and libgeotiff are all resolved with find_package / find_library.
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.14)
project(harmony_trajectory_subsetter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# When building inside an activated conda environment, add CONDA_PREFIX as a
# search prefix so find_package locates the conda packages whether or not
# conda's activation scripts have set CMAKE_PREFIX_PATH. Unset in Docker.
#
# HDF5_ROOT is also pinned to the conda prefix. FindHDF5 first looks for an
# installed hdf5-config.cmake, which conda's HDF5 does not ship, and would
# otherwise fall through to any other HDF5 on the system (e.g. Homebrew on
# macOS), dragging that installation's lib directory into the runtime search
# path ahead of conda's. With HDF5_ROOT set, FindHDF5 searches only the conda
# prefix and resolves HDF5 through conda's own h5cc/h5c++ wrappers.
if(DEFINED ENV{CONDA_PREFIX})
    list(APPEND CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX})
    set(HDF5_ROOT $ENV{CONDA_PREFIX})
endif()

find_package(GTest REQUIRED)

# FindHDF5 resolves the library either from an installed hdf5-config.cmake
# (conda) or by interrogating the h5cc/h5c++ compiler wrappers on PATH
# (the Docker image's /usr/local build, or a distro package).
find_package(HDF5 REQUIRED COMPONENTS C CXX HL)

# Boost, libgeotiff and libtiff are resolved to absolute paths rather than
# linked by bare name, so that a conda build cannot pick up a different
# Boost from elsewhere on the system (e.g. Homebrew on macOS) at link or load
# time.
find_package(Boost REQUIRED COMPONENTS program_options filesystem date_time regex)
find_package(TIFF REQUIRED)
find_library(GEOTIFF_LIBRARY geotiff REQUIRED)
# libgeotiff installs its headers under a distribution-specific subdirectory
# (Rocky: include/libgeotiff, conda: include).
find_path(GEOTIFF_INCLUDE_DIR geotiffio.h PATH_SUFFIXES libgeotiff geotiff REQUIRED)

enable_testing()


#-----------------------------------------------------------------------------
# Define some CMake variables for use later in the project
#-----------------------------------------------------------------------------
get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}/../../.." ABSOLUTE)
set (SUBSETTER_DIR            ${REPO_ROOT}/subsetter)

#-----------------------------------------------------------------------------
# Define compile options, include, and libraries to link in the project
#-----------------------------------------------------------------------------
add_compile_options(-DH5_USE_18_API -Og -Wno-vla-extension)

# The tests locate their input data (tests/data/*.h5, the subsetter
# configuration file) relative to the repository root, which is baked in at
# configure time so the test executables can be run from any directory.
add_compile_definitions(TRAJECTORY_SUBSETTER_ROOT="${REPO_ROOT}")

include_directories(${HDF5_INCLUDE_DIRS} ${GEOTIFF_INCLUDE_DIR})

set(COMMON_LIBRARIES
    ${GEOTIFF_LIBRARY}
    TIFF::TIFF
    Boost::program_options
    Boost::filesystem
    Boost::date_time
    Boost::regex
    ${HDF5_HL_LIBRARIES}
    ${HDF5_LIBRARIES}
)

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
                      ${COMMON_LIBRARIES}
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
                      ${COMMON_LIBRARIES}
)

gtest_discover_tests(subsetter_requiring_temporal_subsetting_test)

# Building and running the tests are separate steps. After `cmake --build`,
# run them with `ctest --test-dir build --output-on-failure`, or do both in
# one go with `cmake --build build --target check`.
add_custom_target(check
                  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                  DEPENDS subsetter_test subsetter_requiring_temporal_subsetting_test
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
