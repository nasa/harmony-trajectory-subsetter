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
    long selectedCount = 2;
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
    long selectedCount = 2;
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

TEST_F(ForwardReferenceCoordinatesTest,
       DefineOneSegment_TrailingFillValues_HitsMaxTraj)
{
    // Simulate a dataset with trailing fill values (-1)
    // Indexes:            0   1    2    3    4    5   6
    int64_t mock_data[] = {10, 50, 100, 150, 200, -1, -1};

    long selectedStartIdx = 3;
    long selectedCount = 2;

    long maxIndexBegIdx = 7;
    // Assume the trajectory dataset has 250 total elements
    long maxTrajIndex = 250;

    // Expected: The first trajectory index is at mock_data[3]
    long firstTrajIndex_expected = 150;
    long firstTrajIndex_result = 0; // Returned-by-reference

    // Expected length: Since nextTrajIndex will be 0, the new logic executes:
    long trajSegLength_expected = maxTrajIndex + 1 - firstTrajIndex_expected;
    long trajSegLength_result = 0; // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        mock_data);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}

/*
int64_t mock_data[] = {10, 50, 100, 150, 200, -1, -1};

trajSegLength = nextTrajIndex - firstTrajIndex;

[INFO] Vu ForwardReferenceCoordinates::scanFwdNonFill(): segStartIdx: [3]
[INFO] Vu ForwardReferenceCoordinates::scanFwdNonFill(): segEndIdx: [4]
[INFO] Vu ForwardReferenceCoordinates::scanFwdNonFill(): firstNonFillIdx: [3]
[INFO] Vu ForwardReferenceCoordinates::scanFwdNonFill(): firstTrajIndex value 150
[INFO] Vu ForwardReferenceCoordinates::scanBackNonFill(): segStartIdx: [3]
[INFO] Vu ForwardReferenceCoordinates::scanBackNonFill(): segEndIdx: [4]
[INFO] Vu ForwardReferenceCoordinates::scanBackNonFill(): lastNonFillIdx: [4]
[INFO] Vu ForwardReferenceCoordinates::scanBackNonFill(): lastTrajIndex value: 200
[INFO] Vu  ForwardReferenceCoordinates::defineOneSegment(): lastSelectedIdx: [4]
[INFO] Vu  ForwardReferenceCoordinates::defineOneSegment(): maxIndexBegIdx - 1 : [6]
[INFO] Vu ForwardReferenceCoordinates::scanFwdNonFill(): segStartIdx: [5]
[INFO] Vu ForwardReferenceCoordinates::scanFwdNonFill(): segEndIdx: [6]
[INFO] ForwardReferenceCoordinates::defineOneSegment(): nextTrajIndex == 0, setting beyond maximum Target index:251
[INFO] Vu  ForwardReferenceCoordinates::defineOneSegment(): lastBegIdx: [4]
[INFO] Vu  ForwardReferenceCoordinates::defineOneSegment(): firstTrajIndex: 150
[INFO] Vu  ForwardReferenceCoordinates::defineOneSegment(): lastTrajIndex value: 200
[INFO] Vu  ForwardReferenceCoordinates::defineOneSegment(): nextTrajIndex: value: 251
[INFO] Vu  ForwardReferenceCoordinates::defineOneSegment(): trajSegLength: value: 101
/Users/vtran11/Documents/workspace/harmony-trajectory-subsetter/tests/unit/gtest/test_ForwardReferenceCoordinates.cpp:180: Failure
Expected equality of these values:
  trajSegLength_expected
    Which is: 250
  trajSegLength_result
    Which is: 101
/Users/vtran11/Documents/workspace/harmony-trajectory-subsetter/tests/unit/gtest/test_ForwardReferenceCoordinates.cpp:180: Failure
Expected equality of these values:
  trajSegLength_expected
    Which is: 250
  trajSegLength_result
    Which is: 101
*/