/*
 *   This file contains unit tests for functions in the Subsetter class.
 *
 *   Function tests included:
 *   - addGroupsRequiringTemporalSubsetting
 *
 */

#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <string.h>
#include <vector>

#include <boost/program_options/parsers.hpp>

#include "gtest_utilities.h"

#include "../../../subsetter/Coordinate.h"
#include "../../../subsetter/IndexSelection.h"
#include "../../../subsetter/SubsetDataLayers.h"
#include "../../../subsetter/Subsetter.h"
#include "../../../subsetter/geobox.h"
#include "H5Cpp.h"

class StubSubsetter : public Subsetter
{
  public:
    StubSubsetter(SubsetDataLayers *subsetDataLayers,
                  std::vector<geobox> *geoboxes,
                  Temporal *temporal,
                  GeoPolygon *geoPolygon,
                  Configuration *config,
                  std::string shortName,
                  H5::H5File &inputFile,
                  std::vector<std::string> groupsWithOnlyTemporalCoordinates,
                  std::vector<std::string> requestedVariables)
        : Subsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, config)
    {

        this->shortName = shortName;
        this->infile = inputFile;
        this->groupsWithOnlyTemporalCoordinates =
            groupsWithOnlyTemporalCoordinates;
        this->requestedVariables = requestedVariables;
    }

    Coordinate *getCoordinate(H5::Group &root,
                              H5::Group &inputGroup,
                              const std::string &groupName,
                              SubsetDataLayers *subsetDataLayers,
                              std::vector<geobox> *geoboxes,
                              Temporal *temporal,
                              GeoPolygon *geoPolygon,
                              Configuration *config,
                              bool repair = false) override
    {
        coor = std::make_unique<Coordinate>(
            groupName, geoboxes, temporal, geoPolygon, config);

        std::vector<std::string> groups =
            this->groupsWithOnlyTemporalCoordinates;
        if (std::find(groups.begin(), groups.end(), groupName) != groups.end())
        {
            coor->setTemporalOnlyCoordinates(true);
        }

        return coor.get();
    }

    // Public passthrough so tests can exercise the protected writeDataset.
    void callWriteDataset(const std::string &objname,
                          const H5::DataSet &indataset,
                          H5::Group &outgroup,
                          const std::string &groupname,
                          IndexSelection *indexes)
    {
        writeDataset(objname, indataset, outgroup, groupname, indexes);
    }

  private:
    std::vector<std::string> groupsWithOnlyTemporalCoordinates;
    std::vector<std::string> requestedVariables;
    std::unique_ptr<Coordinate> coor = nullptr;
};

class SubsetterTest : public ::testing::Test
{
  protected:
    SubsetterTest()
    {
        std::string config_file_path = gtest_utilities::getFullPath(
            "harmony_service/subsetter_config.json");
        config = std::make_unique<Configuration>(config_file_path);

        inputFileATL03 =
            H5::H5File(gtest_utilities::getFullPath("tests/data/ATL03_gt1l.h5"),
                       H5F_ACC_RDONLY);
        rootGroupATL03 = inputFileATL03.openGroup("/");

        inputFileATL10 =
            H5::H5File(gtest_utilities::getFullPath("tests/data/ATL10_gt1l.h5"),
                       H5F_ACC_RDONLY);
        rootGroupATL10 = inputFileATL10.openGroup("/");
    }

    ~SubsetterTest() {}

    /*
     * @brief This wrapper constructs StubSubsetter using collection-specific
     *        information required for the tests.
     *
     * @param shortName The collection short name.
     * @param inputFileName The collection test data file.
     * @param groupsWithOnlyTemporalCoordinates
     */
    void createCollectionSubsetter(
        std::string &shortName,
        H5::H5File &inputFileName,
        const std::vector<std::string> &groupsWithOnlyTemporalCoordinates)
    {
        subsetDataLayers = std::make_unique<SubsetDataLayers>(variables);

        subsetter =
            std::make_unique<StubSubsetter>(subsetDataLayers.get(),
                                            geoboxes.get(),
                                            temporal.get(),
                                            geopolygon.get(),
                                            config.get(),
                                            shortName,
                                            inputFileName,
                                            groupsWithOnlyTemporalCoordinates,
                                            variables);
    }

    H5::H5File inputFileATL03;
    H5::H5File inputFileATL10;
    H5::Group rootGroupATL03;
    H5::Group rootGroupATL10;

    std::unique_ptr<Configuration> config = nullptr;
    std::unique_ptr<std::vector<geobox>> geoboxes = nullptr;
    std::unique_ptr<Temporal> temporal = nullptr;
    std::unique_ptr<GeoPolygon> geopolygon = nullptr;
    std::unique_ptr<SubsetDataLayers> subsetDataLayers = nullptr;
    std::unique_ptr<StubSubsetter> subsetter = nullptr;

    std::vector<std::string> variables;

    const std::vector<std::string> atl03GroupsWithOnlyTemporalCoordinates = {
        "/gt1l/bckgrd_atlas/",
        "/gt1l/signal_find_output/land/",
        "/gt1l/signal_find_output/ocean/",
        "/gt1l/signal_find_output/inlandwater/",
    };
    const std::vector<std::string> atl10GroupsWithOnlyTemporalCoordinates;
};

TEST_F(SubsetterTest,
       addGroupsRequiringTemporalSubsetting_ATL03_bbox_only_include_group)
{
    // Checks that `Subsetter::groupsRequiringTemporalSubsetting` is non-empty
    // for spatial-only (bounding box) requests of datasets within groups
    // with only temporal coordinates, namely `/gt1l/bckgrd_atlas` and
    // `/gt1l/signal_find_output`).
    std::string shortName = "ATL03";

    // Bounding box value doesn't matter; we only care if the constraint exists.
    geoboxes = std::make_unique<std::vector<geobox>>();
    geoboxes->push_back(geobox(1, 2, 3, 4));

    variables = {"/gt1l/bckgrd_atlas/bckgrd_counts",
                 "/gt1l/geolocation/",
                 "/gt1l/signal_find_output/land/bckgrd_mean"};

    createCollectionSubsetter(
        shortName, inputFileATL03, atl03GroupsWithOnlyTemporalCoordinates);

    subsetter->addGroupsRequiringTemporalSubsetting(
        rootGroupATL03, rootGroupATL03, "/");

    std::vector<std::string> actual_groups =
        subsetter->getGroupsRequiringTemporalSubsetting();

    std::vector<std::string> expected_groups = {
        "/gt1l/bckgrd_atlas/", "/gt1l/signal_find_output/land/"};

    EXPECT_EQ(actual_groups, expected_groups);
}

TEST_F(SubsetterTest,
       addGroupsRequiringTemporalSubsetting_ATL03_polygon_only_include_group)
{
    // Checks that `Subsetter::groupsRequiringTemporalSubsetting` is non-empty
    // for spatial-only (bounding polygon) requests of datasets within groups
    // with only temporal coordinates, namely `/gt1l/bckgrd_atlas` and
    // `/gt1l/signal_find_output`).
    std::string shortName = "ATL03";

    // Polygon doesn't matter; we only care if the constraint exists.
    std::string shapeFilePath =
        gtest_utilities::getFullPath("tests/data/NorthPoleSmall.geojson");
    boost::property_tree::ptree shapeFileJSON;
    boost::property_tree::read_json(shapeFilePath, shapeFileJSON);
    geopolygon = std::make_unique<GeoPolygon>(shapeFileJSON);

    variables = {"/gt1l/bckgrd_atlas/bckgrd_counts",
                 "/gt1l/geolocation/",
                 "/gt1l/signal_find_output/land/bckgrd_mean"};

    createCollectionSubsetter(
        shortName, inputFileATL03, atl03GroupsWithOnlyTemporalCoordinates);

    subsetter->addGroupsRequiringTemporalSubsetting(
        rootGroupATL03, rootGroupATL03, "/");

    std::vector<std::string> actual_groups =
        subsetter->getGroupsRequiringTemporalSubsetting();

    std::vector<std::string> expected_groups = {
        "/gt1l/bckgrd_atlas/", "/gt1l/signal_find_output/land/"};

    EXPECT_EQ(actual_groups, expected_groups);
}

TEST_F(SubsetterTest,
       addGroupsRequiringTemporalSubsetting_ATL03_spatial_only_without_group)
{
    // Checks that `Subsetter::groupsRequiringTemporalSubsetting` is empty when
    // we don't request a dataset within a group that only has temporal
    // coordinates, even for subsets with only spatial constraints.
    std::string shortName = "ATL03";

    // Bounding box value doesn't matter; we only care if the constraint exists.
    geoboxes = std::make_unique<std::vector<geobox>>();
    geoboxes->push_back(geobox(1, 2, 3, 4));

    variables = {"/gt1l/geolocation/latitude",
                 "/gt1l/geolocation/solar_elevation",
                 "/gt1l/heights/h_ph"};

    createCollectionSubsetter(
        shortName, inputFileATL03, atl03GroupsWithOnlyTemporalCoordinates);

    subsetter->addGroupsRequiringTemporalSubsetting(
        rootGroupATL03, rootGroupATL03, "/");

    std::vector<std::string> actual_groups =
        subsetter->getGroupsRequiringTemporalSubsetting();

    std::vector<std::string> expected_groups = {};

    EXPECT_EQ(actual_groups, expected_groups);
}

TEST_F(SubsetterTest,
       addGroupsRequiringTemporalSubsetting_ATL03_temporal_only_include_group)
{
    // Checks that `Subsetter::groupsRequiringTemporalSubsetting` is empty for
    // subsets with only temporal constraints.
    std::string shortName = "ATL03";

    // Temporal value doesn't matter; we only care if the constraint exists.
    std::string start = "2018-10-14T00:25:01";
    std::string end = "2018-10-14T00:25:02";
    temporal = std::make_unique<Temporal>(start, end);

    variables = {"/gt1l/bckgrd_atlas/bckgrd_counts",
                 "/gt1l/geolocation/",
                 "/gt1l/signal_find_output/land/bckgrd_mean"};

    createCollectionSubsetter(
        shortName, inputFileATL03, atl03GroupsWithOnlyTemporalCoordinates);

    subsetter->addGroupsRequiringTemporalSubsetting(
        rootGroupATL03, rootGroupATL03, "/");

    std::vector<std::string> actual_groups =
        subsetter->getGroupsRequiringTemporalSubsetting();

    std::vector<std::string> expected_groups = {};

    EXPECT_EQ(actual_groups, expected_groups);
}

TEST_F(
    SubsetterTest,
    addGroupsRequiringTemporalSubsetting_ATL03_spatial_and_temporal_include_group)
{
    // Checks that `Subsetter::groupsRequiringTemporalSubsetting` is empty for
    // subsets with spatial and temporal constraints.
    std::string shortName = "ATL03";

    // Bounding box value doesn't matter; we only care if the constraint exists.
    geoboxes = std::make_unique<std::vector<geobox>>();
    geoboxes->push_back(geobox(1, 2, 3, 4));

    // Temporal value doesn't matter; we only care if the constraint exists.
    std::string start = "2018-10-14T00:25:01";
    std::string end = "2018-10-14T00:25:02";
    temporal = std::make_unique<Temporal>(start, end);

    variables = {"/gt1l/bckgrd_atlas/bckgrd_counts",
                 "/gt1l/geolocation/",
                 "/gt1l/signal_find_output/land/bckgrd_mean"};

    createCollectionSubsetter(
        shortName, inputFileATL03, atl03GroupsWithOnlyTemporalCoordinates);

    subsetter->addGroupsRequiringTemporalSubsetting(
        rootGroupATL03, rootGroupATL03, "/");

    std::vector<std::string> actual_groups =
        subsetter->getGroupsRequiringTemporalSubsetting();

    std::vector<std::string> expected_groups = {};

    EXPECT_EQ(actual_groups, expected_groups);
}

TEST_F(SubsetterTest,
       addGroupsRequiringTemporalSubsetting_ATL10_spatial_only_include_group)
{
    // Checks that `Subsetter::groupsRequiringTemporalSubsetting` is empty for
    // subsets of a collection that doesn't have any groups containing only
    // temporal coordinates, even for subsets with only spatial constraints
    std::string shortName = "ATL10";

    // Bounding box value doesn't matter; we only care if the constraint exists.
    geoboxes = std::make_unique<std::vector<geobox>>();
    geoboxes->push_back(geobox(1, 2, 3, 4));

    variables = {"/gt1l/reference_surface_section/beam_fb_refsurf",
                 "/gt1l/leads/ssh_ndx",
                 "/gt1l/freeboard_segment/beam_refsurf_ndx"};

    createCollectionSubsetter(
        shortName, inputFileATL10, atl10GroupsWithOnlyTemporalCoordinates);

    subsetter->addGroupsRequiringTemporalSubsetting(
        rootGroupATL10, rootGroupATL10, "/");

    std::vector<std::string> actual_groups =
        subsetter->getGroupsRequiringTemporalSubsetting();

    std::vector<std::string> expected_groups = {};

    EXPECT_EQ(actual_groups, expected_groups);
}

TEST_F(SubsetterTest, test_isMatchingDataFound_outfilename_getNumObj_equal_zero)
{
    // Test isMatchingDataFound() using an outfilename that contains no data.
    // The method checks if outfilename.getNumObjs() == 0 and returns false
    H5::H5File infilename =
        H5::H5File(gtest_utilities::getFullPath(
                       "tests/data/variable_subset_ATL24_data.h5"),
                   H5F_ACC_RDONLY);
    H5::H5File outfilename =
        H5::H5File(gtest_utilities::getFullPath(
                       "tests/data/variable_subset_ATL24_no_data.h5"),
                   H5F_ACC_RDONLY);
    std::string shortName = "ATL24";

    createCollectionSubsetter(
        shortName, infilename, std::vector<std::string>());

    bool expected = subsetter->isMatchingDataFound(infilename, outfilename);
    EXPECT_FALSE(expected);
}

TEST_F(SubsetterTest,
       test_isMatchingDataFound_outfilename_getNumObj_greater_zero)
{
    // Test isMatchingDataFound() with an outfilename that contains data, using
    // the same .h5 file as the infilename. The method evaluates
    // outfilename.getNumObjs() > 0 and returns true since the
    // isMatchingDataFound() method this->temporal == NULL condition is met.
    H5::H5File infilename =
        H5::H5File(gtest_utilities::getFullPath(
                       "tests/data/variable_subset_ATL24_data.h5"),
                   H5F_ACC_RDONLY);
    H5::H5File outfilename =
        H5::H5File(gtest_utilities::getFullPath(
                       "tests/data/variable_subset_ATL24_data.h5"),
                   H5F_ACC_RDONLY);

    std::string shortName = "ATL24";

    createCollectionSubsetter(
        shortName, infilename, std::vector<std::string>());

    bool expected = subsetter->isMatchingDataFound(infilename, outfilename);
    EXPECT_TRUE(expected);
}

TEST_F(SubsetterTest, test_openCreateHdf5File_SuperblockTest)
{
    // Test openCreateHdf5File() to verify that the output HDF5 file
    // is created with HDF5 1.8 compatibility bounds. The method evaluates
    // the creation properties of the input file and configures the output
    // file access property list to generate a version 2 superblock.
    H5F_info2_t fileInfo;
    H5::H5File h5Outfile;

    H5::H5File h5Infile =
        H5::H5File(gtest_utilities::getFullPath(
                       "tests/data/variable_subset_ATL24_data.h5"),
                   H5F_ACC_RDONLY);

    std::string outFileName =
        (std::filesystem::temp_directory_path() / "fake_superblock_file.h5")
            .string();

    subsetter->openCreateHdf5File(
        h5Infile.getFileName(), outFileName, h5Infile, h5Outfile);

    hid_t fileId = H5Fopen(outFileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    herr_t status = H5Fget_info2(fileId, &fileInfo);

    unsigned superVersion = fileInfo.super.version;

    EXPECT_EQ(superVersion, 2);

    H5Fclose(fileId);

    h5Outfile.close();
    h5Infile.close();
}

TEST_F(SubsetterTest, writeDataset_contiguous_source_chunks_to_subset_extent)
{
    // Regression test for DAS-2505. A contiguous source dataset must be
    // rewritten chunked to the *subset* extent (newdims) and compressed, not
    // chunked to the *source* extent (olddims) with no compression. The latter
    // allocated one full source-orbit-sized chunk per subset dataset, inflating
    // contiguous-source output (e.g. GEDI L2A) by ~150x.
    std::string inPath =
        (std::filesystem::temp_directory_path() / "das2505_in.h5").string();
    std::string outPath =
        (std::filesystem::temp_directory_path() / "das2505_out.h5").string();

    const hsize_t sourceLen = 1000;
    const long subsetLen = 10;

    // Build an input file with a 1-D contiguous dataset (default layout).
    {
        H5::H5File in(inPath, H5F_ACC_TRUNC);
        hsize_t dims[1] = {sourceLen};
        H5::DataSpace space(1, dims);
        H5::DataSet ds =
            in.createDataSet("data", H5::PredType::NATIVE_INT, space);
        std::vector<int> buf(sourceLen);
        for (hsize_t i = 0; i < sourceLen; i++)
            buf[i] = static_cast<int>(i);
        ds.write(buf.data(), H5::PredType::NATIVE_INT);
        ASSERT_EQ(ds.getCreatePlist().getLayout(), H5D_CONTIGUOUS);
    }

    H5::H5File in(inPath, H5F_ACC_RDONLY);
    H5::DataSet indataset = in.openDataSet("data");

    H5::H5File out(outPath, H5F_ACC_TRUNC);
    H5::Group outRoot = out.openGroup("/");

    // Select a small subset of the source dataset.
    IndexSelection indexes(sourceLen);
    indexes.addSegment(0, subsetLen);

    // No spatial/temporal constraints are needed to exercise the storage
    // layout, so the default (empty) fixture members are used.
    std::string shortName = "TEST";
    createCollectionSubsetter(shortName, in, std::vector<std::string>());
    subsetter->callWriteDataset("data", indataset, outRoot, "/", &indexes);

    // Reopen the output dataset and inspect its storage layout.
    H5::DataSet outds = out.openDataSet("data");
    H5::DSetCreatPropList outplist = outds.getCreatePlist();

    ASSERT_EQ(outplist.getLayout(), H5D_CHUNKED);

    // Chunk is the subset extent, NOT the source extent.
    hsize_t chunk[1] = {0};
    outplist.getChunk(1, chunk);
    EXPECT_EQ(chunk[0], static_cast<hsize_t>(subsetLen));
    EXPECT_NE(chunk[0], sourceLen);

    // The logical output extent also matches the subset.
    hsize_t outdims[1] = {0};
    outds.getSpace().getSimpleExtentDims(outdims, NULL);
    EXPECT_EQ(outdims[0], static_cast<hsize_t>(subsetLen));

    // A deflate (gzip) compression filter is applied.
    bool foundDeflate = false;
    for (int i = 0; i < outplist.getNfilters(); i++)
    {
        unsigned int flags = 0;
        size_t cdNelmts = 8;
        unsigned int cdValues[8] = {0};
        char nameBuf[64] = {0};
        unsigned int filterConfig = 0;
        H5Z_filter_t filterType = outplist.getFilter(i,
                                                     flags,
                                                     cdNelmts,
                                                     cdValues,
                                                     sizeof(nameBuf),
                                                     nameBuf,
                                                     filterConfig);
        if (filterType == H5Z_FILTER_DEFLATE)
            foundDeflate = true;
    }
    EXPECT_TRUE(foundDeflate);

    out.close();
    in.close();
    std::remove(inPath.c_str());
    std::remove(outPath.c_str());
}
