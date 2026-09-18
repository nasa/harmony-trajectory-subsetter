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
    // (lastSelectedIdx + 1 == maxIndexBegIdx), the segment runs to the end
    // of the trajectory dataset. indexBeg values are 1-based, so trajectory
    // values 150 through 250 inclusive is 250 - 150 + 1 = 101 values.
    long trajSegLength_expected = 101;
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

TEST_F(
    ForwardReferenceCoordinatesTest,
    AddSegmentIndexSelection_CalculatesLengthToNextTraj_ATL10_v6_beam_lead_ndx)
{
    // Simplified 10-element index dataset matching
    // /gt1l/reference_surface_section/beam_lead_ndx log:
    //  - scanFwdNonFill: finds first non-fill at selectedStartIdx = 0 (Index[2]
    //  = 1)
    //  - scanBackNonFill: finds non-fill at maxIndexEnd = 7 (Index[6] = 667)
    //  - Lookahead scans past mock_data[7] = -1 to find nextTrajIndex at
    //  mock_data[8] = 668
    //  - trajSegLength = nextTrajIndex - firstTrajIndex = 668 - 1 = 667
    // Indexes:             0   1  2  3    4    5    6   7    8    9
    int64_t mock_data[] = {-1, -1, 1, 3, 100, 500, 667, -1, 668, 743};

    long selectedStart = 0;
    long maxIndexEnd = 7;
    long selectedCount = maxIndexEnd - selectedStart;

    long maxIndexBegIdx = 10;
    long coordinateSize = 743;
    IndexSelection targetIndexSelection(coordinateSize);

    long firstTrajIndex_expected = 1;
    long trajSegLength_expected = 667;

    coordinate_object->addSegmentIndexSelection(targetIndexSelection,
                                                selectedStart,
                                                selectedCount,
                                                maxIndexBegIdx,
                                                coordinateSize,
                                                mock_data);

    long trajSegLength_result =
        targetIndexSelection.segments[firstTrajIndex_expected - 1];

    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}

TEST_F(ForwardReferenceCoordinatesTest,
       DefineOneSegment_LastSegment_ATL10_RealIndexBeginValues)
{
    // The /gt1l/reference_surface_section/beam_lead_ndx and beam_lead_n
    // values from tests/data/ATL10_gt1l.h5, whose /gt1l/leads datasets hold
    // 95 values. They are inlined rather than read from the file because
    // gtest_utilities::readDataset() reads into an int64_t buffer and these
    // datasets are 32-bit.
    int64_t beam_lead_ndx[] = {-1, -1, -1, -1, -1, -1, -1, -1, 1,  -1,
                               16, -1, -1, -1, -1, 23, -1, -1, 28, 39,
                               -1, 42, 51, 65, 89, -1, -1, -1, -1, -1,
                               -1, -1, -1, -1, -1, -1, -1, -1, -1};

    // Select entries 20 through 38. The four non-fill entries in that range
    // claim leads 42 through 95 between them:
    //     entry 21: beam_lead_ndx 42, beam_lead_n  9 -> leads 42..50
    //     entry 22: beam_lead_ndx 51, beam_lead_n 14 -> leads 51..64
    //     entry 23: beam_lead_ndx 65, beam_lead_n 24 -> leads 65..88
    //     entry 24: beam_lead_ndx 89, beam_lead_n  7 -> leads 89..95
    long selectedStartIdx = 20;
    long selectedCount = 19;

    // The selection reaches the final entry of the index begin dataset, so
    // lastSelectedIdx + 1 == maxIndexBegIdx and the segment runs to the end
    // of the trajectory (leads) datasets. Entry 24 agrees: the last lead it
    // claims, 89 + 7 - 1, is lead 95, the last one in the file.
    long maxIndexBegIdx = 39;
    long maxTrajIndex = 95;

    long firstTrajIndex_expected = 42;

    // 9 + 14 + 24 + 7. Equally, leads 42 through 95 inclusive, as
    // beam_lead_ndx is one based indexing.
    long trajSegLength_expected = 54;

    long firstTrajIndex_result = 0; // Returned-by-reference
    long trajSegLength_result = 0;  // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx,
                                        selectedCount,
                                        firstTrajIndex_result,
                                        trajSegLength_result,
                                        maxIndexBegIdx,
                                        maxTrajIndex,
                                        beam_lead_ndx);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}

TEST_F(ForwardReferenceCoordinatesTest,
       AddSegmentIndexSelectionFromTemporalBounds_Success)
{
    // Setup 10-element index dataset:
    //  - scanFwdNonFill finds first non-fill at selectedStartIdx = 0 (Index[2]
    //  = 1)
    //  - With selectedCount = 7 (maxIndexEnd - selectedStart), it looks ahead
    //  to
    //    mock_data[8] = 668 -> trajSegLength = 668 - 1 = 667.
    int64_t mock_data[] = {-1, -1, 1, 3, 100, 500, 667, -1, 668, 743};

    long coordinateSize = 743;
    long maxIndexBegIdx = 10;
    IndexSelection targetIndexSelection(coordinateSize);

    IndexSelection temporalBoundsSelection(maxIndexBegIdx);
    temporalBoundsSelection.maxIndexEnd = 7;

    long firstTrajIndex_expected = 1;
    long trajSegLength_expected_correct = 667;

    coordinate_object->addSegmentIndexSelectionFromTemporalBounds(
        temporalBoundsSelection,
        targetIndexSelection,
        maxIndexBegIdx,
        coordinateSize,
        mock_data);

    long trajSegLength_result =
        targetIndexSelection.segments[firstTrajIndex_expected - 1];

    // Verify correct trajectory segment length calculation
    EXPECT_EQ(trajSegLength_expected_correct, trajSegLength_result);
}

TEST_F(ForwardReferenceCoordinatesTest,
       AddSegmentIndexSelectionFromTemporalBounds_DAS_2308_Negative_OffByOne)
{
    // Setup 10-element index dataset:
    //  - Simulates legacy DAS-2308 off-by-one truncation where selectedCount =
    //  6
    //  - Truncated count causes scanFwdNonFill to terminate early at
    //  mock_data[6] = 667
    //    -> trajSegLength = 667 - 1 = 666 (incorrect).
    int64_t mock_data[] = {-1, -1, 1, 3, 100, 500, 667, -1, 668, 743};

    long coordinateSize = 743;
    long maxIndexBegIdx = 10;
    IndexSelection targetIndexSelection(coordinateSize);

    IndexSelection temporalBoundsSelection(maxIndexBegIdx);
    temporalBoundsSelection.maxIndexEnd = 7;
    // Inject legacy '- 1' off-by-one calculation to force failure mode
    temporalBoundsSelection.maxIndexEnd =
        temporalBoundsSelection.maxIndexEnd -
        temporalBoundsSelection.minIndexStart - 1;

    coordinate_object->addSegmentIndexSelectionFromTemporalBounds(
        temporalBoundsSelection,
        targetIndexSelection,
        maxIndexBegIdx,
        coordinateSize,
        mock_data);

    long firstTrajIndex_expected = 1;
    long trajSegLength_expected_incorrect = 666;
    long trajSegLength_result =
        targetIndexSelection.segments[firstTrajIndex_expected - 1];

    // Confirm that injecting the off-by-one offset reproduces the truncated
    // length (666)
    EXPECT_EQ(trajSegLength_expected_incorrect, trajSegLength_result);
}
