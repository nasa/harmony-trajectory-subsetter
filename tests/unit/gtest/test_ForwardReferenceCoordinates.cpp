#include <gtest/gtest.h>
#include <iostream>

#include "gtest_utilities.h"

#include "../../../subsetter/Configuration.h"
#include "../../../subsetter/ForwardReferenceCoordinates.h"
#include "../../../subsetter/GeoPolygon.h"
#include "../../../subsetter/Temporal.h"
#include "../../../subsetter/geobox.h"
#include "H5Cpp.h"

#include <regex>
#include <string>

class ForwardReferenceCoordinatesTest : public ::testing::Test
{
  protected:
    ForwardReferenceCoordinatesTest()
    {
        std::string config_file_path = gtest_utilities::getFullPath(
            "harmony_service/subsetter_config.json");
        config = std::make_unique<Configuration>(config_file_path);
        coordinate_object = std::make_unique<ForwardReferenceCoordinates>(
            groupname, geoboxes, temporal, geopolygon, config.get());

        // Read in test data.
        // This index begin dataset starts and ends with fill values (0).
        index_begin_dataset = gtest_utilities::readDataset(
            gtest_utilities::getFullPath(
                "tests/data/ATL03_indexbegin_start_end_FVs.h5"),
            "ph_index_beg");
    }

    ~ForwardReferenceCoordinatesTest() { delete index_begin_dataset; }

    int64_t *index_begin_dataset = nullptr;
    std::unique_ptr<ForwardReferenceCoordinates> coordinate_object;

  private:
    std::string groupname = "ATL03";
    std::vector<geobox> *geoboxes = nullptr;
    Temporal *temporal = nullptr;
    GeoPolygon *geopolygon = nullptr;

    std::unique_ptr<Configuration> config;
};

TEST_F(ForwardReferenceCoordinatesTest, DefineOneSegment_start_nonFV_end_nonFV)
{
    // Segment data generated from bounding box {-60,24,-55,26}.
    long selectedStartIdx = 5538;
    long selectedCount = 1;
    long maxIndexBegIdx = 149697;
    long maxTrajIndex = 3219960;

    long firstTrajIndex_expected = 1236;
    long firstTrajIndex_result = 0; // Returned-by-reference
    long trajSegLength_expected = 1;
    long trajSegLength_result = 0; // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        this->index_begin_dataset);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}

TEST_F(ForwardReferenceCoordinatesTest, DefineOneSegment_start_FV_end_FV)
{
    // Segment data generated from bounding box {60,23.3435,-59,23.55}.
    long selectedStartIdx = 19147;
    long selectedCount = 1130;
    long maxIndexBegIdx = 149697;
    long maxTrajIndex = 3219960;

    long firstTrajIndex_expected = 31879;
    long firstTrajIndex_result = 0; // Returned-by-reference
    long trajSegLength_expected = 2310;
    long trajSegLength_result = 0; // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        this->index_begin_dataset);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}

TEST_F(ForwardReferenceCoordinatesTest, DefineOneSegment_start_nonFV_end_FVall)
{
    // Segment data generated from bounding box {-62,-2,60,25}.
    long selectedStartIdx = 11092;
    long selectedCount = 1;
    long maxIndexBegIdx = 149697;
    long maxTrajIndex = 3219960;

    long firstTrajIndex_expected = 15404;
    long firstTrajIndex_result = 0; // Returned-by-reference
    long trajSegLength_expected = 1;
    long trajSegLength_result = 0; // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        this->index_begin_dataset);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}

TEST_F(ForwardReferenceCoordinatesTest, DefineOneSegment_start_FVall_end_nonFV)
{
    // Segment data generated from bounding box {-62,24.3022,60,27}.
    long selectedStartIdx = 0;
    long selectedCount = 14958;
    long maxIndexBegIdx = 149697;
    long maxTrajIndex = 3219960;

    long firstTrajIndex_expected = 1;
    long firstTrajIndex_result = 0; // Returned-by-reference
    long trajSegLength_expected = 16519;
    long trajSegLength_result = 0; // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        this->index_begin_dataset);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}
