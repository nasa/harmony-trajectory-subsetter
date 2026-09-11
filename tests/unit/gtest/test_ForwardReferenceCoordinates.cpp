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
    long trajSegLength_expected = maxTrajIndex;
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

TEST_F(ForwardReferenceCoordinatesTest,
       DefineOneSegment_LastSegment_MatchesMaxIndexBegIdx)
{
    // Simulate an index dataset where the selected slice reaches the end of the
    // array. Indexes:     0   1   2    3    4    5    6
    int64_t mock_data[] = {5, 20, 50, 100, 150, 200, 230};

    // Select the last 3 elements of the array (indexes 4, 5, 6)
    long selectedStartIdx = 4;
    long selectedCount = 3;

    // The selection end index matches the size of the dataset (4 + 3 == 7)
    long maxIndexBegIdx = 7;

    // Total number of elements in the target/trajectory dataset
    long maxTrajIndex = 250;

    // Expected: First trajectory index is at mock_data[4] -> 150
    long firstTrajIndex_expected = 150;
    long firstTrajIndex_result = 0;

    // Expected length: Since this is the last segment
    // (lastSelectedIdx + 1 == maxIndexBegIdx),
    long trajSegLength_expected = 100;
    long trajSegLength_result = 0;

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

TEST_F(ForwardReferenceCoordinatesTest,
       DefineOneSegment_CalculatesLengthToNextTraj_ATL10_v6_beam_lead_ndx)
{
    // Simplified 10-element index dataset
    // /gt1l/reference_surface_section/beam_lead_ndx log output:
    //  - scanFwdNonFill: finds the first non -1 starting at
    //    selectedStartIdx = 0 (Index[2] = 1) and ending at selectedCount = 7
    //  - scanBackNonFill: finds the first non -1 starting backwards
    //    at selectedCount = 7 (Index[6] = 667)
    //  - scanFwdNonFill: finds the first non -1 starting at
    //.   selectedCount = 7 (Index[8] = 668) and ending maxIndexBegIdx = 10
    // - Next segment lookahead at index 8 (value 668)
    // - trajSegLength = nextTrajIndex - firstTrajIndex = 668 - 1 = 667
    // Indexes:             0   1  2  3    4    5    6   7    8    9
    int64_t mock_data[] = {-1, -1, 1, 3, 100, 500, 667, -1, 668, 743};

    long selectedStart = 0;
    long maxIndexEnd = 7;

    long selectedCount = maxIndexEnd - selectedStart;

    // Total elements in the index array
    long maxIndexBegIdx = 10;

    // Total elements in the trajectory/target dataset
    long maxTrajIndex = 743;

    // Expected: scanFwdNonFill finds first non-fill at mock_data[2] -> 1
    long firstTrajIndex_expected = 1;
    long firstTrajIndex_result = 0;

    // Expected length: scanFwdNonFill looks ahead past mock_data[7]=-1 to
    // find nextTrajIndex at mock_data[8]=668
    // trajSegLength = nextTrajIndex - firstTrajIndex (668 - 1 = 667)
    long trajSegLength_expected = 667;
    long trajSegLength_result = 0;

    coordinate_object->defineOneSegment(selectedStart,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        mock_data);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}

TEST_F(ForwardReferenceCoordinatesTest,
       DefineOneSegment_DAS_2308_Negative_test_selectedCount_minus_1)
{
    // Simplified 10-element index dataset mirroring log output:
    //  - scanFwdNonFill: finds the first non -1 starting at
    //    selectedStartIdx = 0 (Index[2] = 1) and ending at selectedCount = 6
    //  - scanBackNonFill: finds the first non -1 starting backwards
    //    at selectedCount = 6 (Index[5] = 500)
    //  - scanFwdNonFill: finds the first non -1 starting at
    //.   selectedCount = 6 (Index[6] = 667) and ending maxIndexBegIdx = 10
    //
    // Old calculation DAS-2308
    //  long selectedCount = segIndexes->maxIndexEnd - selectedStart;
    //
    //  - trajSegLength = nextTrajIndex - firstTrajIndex = 667 - 1 = 666
    //  (Incorrect off-by-one truncation)
    //
    // The correct full temporal range calculation:
    //  - trajSegLength should look ahead to mock_data[8] = 668 -> 668 - 1 = 667
    // Indexes:             0   1  2  3    4    5    6   7    8    9
    int64_t mock_data[] = {-1, -1, 1, 3, 100, 500, 667, -1, 668, 743};

    long selectedStart = 0;
    long maxIndexEnd = 7;
    long selectedCount = maxIndexEnd - selectedStart - 1;

    // Total elements in the index array
    long maxIndexBegIdx = 10;

    // Total elements in the trajectory/target dataset
    long maxTrajIndex = 743;

    // Expected: scanFwdNonFill finds first non-fill at mock_data[2] -> 1
    long firstTrajIndex_expected = 1;
    long firstTrajIndex_result = 0;

    // Expected length: scanFwdNonFill looks ahead past mock_data[7]=-1 to
    // find nextTrajIndex at mock_data[8]=668
    // trajSegLength = nextTrajIndex - firstTrajIndex (668 - 1 = 667)
    long trajSegLength_expected = 667;
    long trajSegLength_result = 0;

    coordinate_object->defineOneSegment(selectedStart,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        mock_data);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_NE(666, trajSegLength_result);
}
