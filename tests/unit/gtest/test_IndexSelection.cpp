#include <gtest/gtest.h>
#include <iostream>

#include "../../../subsetter/IndexSelection.h"

#include <map>

class IndexSelectionTest : public ::testing::Test
{
  protected:
    IndexSelectionTest() { indexes = IndexSelection(maxLength); }

    IndexSelection indexes = NULL;

  private:
    long maxLength = 50;
};

//  [ ] - existing segment
//  { } - new segment
//   _  - values
//   +  - joined
// testSegments: (start_index, length)

TEST_F(IndexSelectionTest, AddSegmentCaseA1_PASS)
{
    /* Simple Cases (overlap exists between new and existing segments, move
     * right to left) */
    std::cout
        << "*** IndexSelection::addSegment Unit Tests: Simple Test Cases "
           "(overlap exists between new and existing segments, move right "
           "to left) ***\n\n";

    // PASS - Case A1 : [ _ ] _ { _ } : This test is a simple case for Case A1.
    std::cout << "\tCase A1 : [ _ ] _ { _ }.\n";
    this->indexes.addSegment(3, 5);        // [3,5]  - existing segment
    this->indexes.addSegment(10, 3);       // {10,3} - new segment
    std::map<long, long> expectedResultA1; // (3,5), (10,3) - expected segments
    expectedResultA1[3] = 5;
    expectedResultA1[10] = 3;
    EXPECT_EQ(this->indexes.segments, expectedResultA1);
}

TEST_F(IndexSelectionTest, AddSegmentCaseA1_FAIL)
{
    // FAIL- Case A1 : [ _ ] _ { _ } : This test is a simple case for Case A1.
    // Change expectedResultA1[10] = 4 for segment not equal
    std::cout << "\tCase A1 : [ _ ] _ { _ }.\n";
    this->indexes.addSegment(3, 5);        // [3,5]  - existing segment
    this->indexes.addSegment(10, 3);       // {10,3} - new segment
    std::map<long, long> expectedResultA1; // (3,5), (10,3) - expected segments
    expectedResultA1[3] = 5;
    expectedResultA1[10] = 4; // Change from 3 to 4 segment not equal
    EXPECT_NE(this->indexes.segments, expectedResultA1);
}

TEST_F(IndexSelectionTest, AddSegmentCaseA2_PASS)
{
    // PASS - Case A2 : [ _ { _ ] _ } : This test is a simple case for Case A2.
    std::cout << "\tCase A2 : [ _ { _ ] _ }.\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(4, 6);        // {4,6} - new segment
    std::map<long, long> expectedResultA2; // (3,7) - expected segment
    expectedResultA2[3] = 7;
    EXPECT_EQ(this->indexes.segments, expectedResultA2);
}

TEST_F(IndexSelectionTest, AddSegmentCaseA2_FAIL)
{
    // FAIL - Case A2 : [ _ { _ ] _ } : This test is a simple case for Case A2.
    // Change expectedResultA2[3] = 8 for segment not equal
    std::cout << "\tCase A2 : [ _ { _ ] _ }.\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(4, 6);        // {4,6} - new segment
    std::map<long, long> expectedResultA2; // (3,7) - expected segment
    expectedResultA2[3] = 8;
    EXPECT_NE(this->indexes.segments, expectedResultA2);
}

TEST_F(IndexSelectionTest, AddSegmentCaseA3_PASS)
{
    // PASS - Case A3 : { _ [ _ ] _ } : This test is a simple case for Case A3.
    std::cout << "\tCase A3 : { _ [ _ ] _ }.\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(2, 9);        // {2,9} - new segment
    std::map<long, long> expectedResultA3; // (2,9) - expected segment
    expectedResultA3[2] = 9;
    EXPECT_EQ(this->indexes.segments, expectedResultA3);
}

TEST_F(IndexSelectionTest, AddSegmentCaseA3_FAIL)
{
    // FAIL - Case A3 : { _ [ _ ] _ } : This test is a simple case for Case A3.
    // Change expectedResultA3[2] = 11 for segment not equal
    std::cout << "\tCase A3 : { _ [ _ ] _ }.\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(2, 9);        // {2,9} - new segment
    std::map<long, long> expectedResultA3; // (2,9) - expected segment
    expectedResultA3[2] = 11;
    EXPECT_NE(this->indexes.segments, expectedResultA3);
}

TEST_F(IndexSelectionTest, AddSegmentCaseB1_PASS)
{
    // PASS - Case B1 : [ _ { _ } _ ] : This test is a simple case for Case B1.
    std::cout << "\tCase B1 : [ _ { _ } _ ].\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(4, 2);        // {4,2} - new segment
    std::map<long, long> expectedResultB1; // (3,5) - expected segment
    expectedResultB1[3] = 5;
    EXPECT_EQ(this->indexes.segments, expectedResultB1);
}

TEST_F(IndexSelectionTest, AddSegmentCaseB1_FAIL)
{
    // FAIL - Case B1 : [ _ { _ } _ ] : This test is a simple case for Case B1.
    // Change expectedResultB1[3] = 15 for segment not equal
    std::cout << "\tCase B1 : [ _ { _ } _ ].\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(4, 2);        // {4,2} - new segment
    std::map<long, long> expectedResultB1; // (3,5) - expected segment
    expectedResultB1[3] = 15;
    EXPECT_NE(this->indexes.segments, expectedResultB1);
}

TEST_F(IndexSelectionTest, AddSegmentCaseB2_PASS)
{
    // PASS - Case B2 : { _ [ _ } _ ] : This test is a simple case for Case B2.
    std::cout << "\tCase B2 : { _ [ _ } _ ].\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(2, 4);        // {2,4} - new segment
    std::map<long, long> expectedResultB2; // (2,6) - expected segment
    expectedResultB2[2] = 6;
    EXPECT_EQ(this->indexes.segments, expectedResultB2);
}

TEST_F(IndexSelectionTest, AddSegmentCaseB2_FAIL)
{
    // FAIL - Case B2 : { _ [ _ } _ ] : This test is a simple case for Case B2.
    // Change expectedResultB2[2] = 16 for segment not equal
    std::cout << "\tCase B2 : { _ [ _ } _ ].\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(2, 4);        // {2,4} - new segment
    std::map<long, long> expectedResultB2; // (2,6) - expected segment
    expectedResultB2[2] = 16;
    EXPECT_NE(this->indexes.segments, expectedResultB2);
}

TEST_F(IndexSelectionTest, AddSegmentCaseB3_PASS)
{
    // PASS - Case B3 : { _ } _ [ _ ] : This is a simple case for Case B3 (fall
    // through case).
    std::cout << "\tCase B3 : { _ } _ [ _ ].\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(1, 1);        // {1,1} - new segment
    std::map<long, long> expectedResultB3; // (1,1), (3,5) - expected segment
    expectedResultB3[1] = 1;
    expectedResultB3[3] = 5;
    EXPECT_EQ(this->indexes.segments, expectedResultB3);
}

TEST_F(IndexSelectionTest, AddSegmentCaseB3_FAIL)
{
    // FAIL - Case B3 : { _ } _ [ _ ] : This is a simple case for Case B3 (fall
    // through case). Change expectedResultB3[1] = 11; and expectedResultB3[3] =
    // 15; for segment not equal
    std::cout << "\tCase B3 : { _ } _ [ _ ].\n";
    this->indexes.addSegment(3, 5);        // [3,5] - existing segment
    this->indexes.addSegment(1, 1);        // {1,1} - new segment
    std::map<long, long> expectedResultB3; // (1,1), (3,5) - expected segment
    expectedResultB3[1] = 11;
    expectedResultB3[3] = 15;
    EXPECT_NE(this->indexes.segments, expectedResultB3);
}

TEST_F(IndexSelectionTest, AddSegmentEdge1CaseA2_PASS)
{
    /* Edge Cases */
    std::cout
        << "*** IndexSelection::addSegment Unit Tests: Edge Cases ***\n\n";

    // PASS - Edge Case 1 : [ _ ] + { _ } : This test is an edge case that
    // should be handled as a A2 Case.
    std::cout << "\tEdge Case 1 : [ _ ] + { _ }.\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(8, 2);               // {8,2} - new segment
    std::map<long, long> expectedResultEdgeCase1; // (3,7) - expected segments
    expectedResultEdgeCase1[3] = 7;
    EXPECT_EQ(this->indexes.segments, expectedResultEdgeCase1);
}

TEST_F(IndexSelectionTest, AddSegmentEdge1CaseA2_FAIL)
{
    // FAIL - Edge Case 1 : [ _ ] + { _ } : This test is an edge case that
    // should be handled as a A2 Case. Change expectedResultEdgeCase1[3] = 17;
    // for segment not equal
    std::cout << "\tEdge Case 1 : [ _ ] + { _ }.\n";

    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(8, 2);               // {8,2} - new segment
    std::map<long, long> expectedResultEdgeCase1; // (3,7) - expected segments
    expectedResultEdgeCase1[3] = 17;
    EXPECT_NE(this->indexes.segments, expectedResultEdgeCase1);
}

TEST_F(IndexSelectionTest, AddSegmentEdge2CaseA2_PASS)
{
    // PASS - Edge Case 1b : [ _ { _ ] + } : This test is an edge case that
    // should be handled as a A2 case.
    std::cout << "\tEdge Case 1b : [ _ { _ ] + }.\n";
    this->indexes.addSegment(3, 5);                // [3,5] - existing segment
    this->indexes.addSegment(4, 4);                // {4,4} - new segment
    std::map<long, long> expectedResultEdgeCase1b; // (3,5) - expected segment
    expectedResultEdgeCase1b[3] = 5;
    EXPECT_EQ(this->indexes.segments, expectedResultEdgeCase1b);
}

TEST_F(IndexSelectionTest, AddSegmentEdge2CaseA2_FAIL)
{
    // FAIL - Edge Case 1b : [ _ { _ ] + } : This test is an edge case that
    // should be handled as a A2 case. Change expectedResultEdgeCase1b[3] = 25;
    // for segment not equal
    std::cout << "\tEdge Case 1b : [ _ { _ ] + }.\n";
    this->indexes.addSegment(3, 5);                // [3,5] - existing segment
    this->indexes.addSegment(4, 4);                // {4,4} - new segment
    std::map<long, long> expectedResultEdgeCase1b; // (3,5) - expected segment
    expectedResultEdgeCase1b[3] = 25;
    EXPECT_NE(this->indexes.segments, expectedResultEdgeCase1b);
}

TEST_F(IndexSelectionTest, AddSegmentEdge3CaseA2_PASS)
{
    // PASS - Edge Case 2 : { + [ _ ] _ } : This test is an edge case that
    // should be handled as a A2 Case.
    std::cout << "\tEdge Case 2 : { + [ _ ] _ }.\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(3, 6);               // {3,6} - new segment
    std::map<long, long> expectedResultEdgeCase2; // (3,6) - expected segment
    expectedResultEdgeCase2[3] = 6;
    EXPECT_EQ(this->indexes.segments, expectedResultEdgeCase2);
}

TEST_F(IndexSelectionTest, AddSegmentEdge3CaseA2_FAIL)
{
    // FAIL - Edge Case 1b : { + [ _ ] _ } : This test is an edge case that
    // should be handled as a A2 case. Change expectedResultEdgeCase2[3] = 26;
    // for segment not equal
    std::cout << "\tEdge Case 2 : { + [ _ ] _ }.\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(3, 6);               // {3,6} - new segment
    std::map<long, long> expectedResultEdgeCase2; // (3,6) - expected segment
    expectedResultEdgeCase2[3] = 26;
    EXPECT_NE(this->indexes.segments, expectedResultEdgeCase2);
}

TEST_F(IndexSelectionTest, AddSegmentEdge1CaseA3_PASS)
{
    // PASS - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case.
    std::cout << "\tEdge Case 3 : { _ [ _ ] + }.\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(2, 6);               // {2,6} - new segment
    std::map<long, long> expectedResultEdgeCase3; // (2,6) - expected segment
    expectedResultEdgeCase3[2] = 6;
    EXPECT_EQ(this->indexes.segments, expectedResultEdgeCase3);
}

TEST_F(IndexSelectionTest, AddSegmentEdge1CaseA3_FAIL)
{
    // FAIL - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase2[3] = 26;
    // for segment not equal
    std::cout << "\tEdge Case 2 : { _ [ _ ] + }.\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(3, 6);               // {3,6} - new segment
    std::map<long, long> expectedResultEdgeCase2; // (3,6) - expected segment
    expectedResultEdgeCase2[3] = 36;
    EXPECT_NE(this->indexes.segments, expectedResultEdgeCase2);
}

TEST_F(IndexSelectionTest, AddSegmentEdge2CaseA3_PASS)
{
    std::cout << "\tEdge Case 4 : { + [ _ ] + }.\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(3, 5);               // {3,5} - new segment
    std::map<long, long> expectedResultEdgeCase4; // (3,5) - expected segment
    expectedResultEdgeCase4[3] = 5;
    EXPECT_EQ(this->indexes.segments, expectedResultEdgeCase4);
}

TEST_F(IndexSelectionTest, AddSegmentEdge2CaseA3_FAIL)
{
    // FAIL - Edge Case 3 : { + [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase4[3] = 35;
    // for segment not equal
    std::cout << "\tEdge Case 4 : { + [ _ ] + }.\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(3, 5);               // {3,5} - new segment
    std::map<long, long> expectedResultEdgeCase4; // (3,5) - expected segment
    expectedResultEdgeCase4[3] = 35;
    EXPECT_NE(this->indexes.segments, expectedResultEdgeCase4);
}

TEST_F(IndexSelectionTest, AddSegmentEdge3CaseA3_FAIL)
{
    // FAIL - Edge Case 3 : [ + { _ } _ ] : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase4b[3] = 45;
    // for segment not equal
    std::cout << "\tEdge Case 4b : [ + { _ } _ ].\n";
    this->indexes.addSegment(3, 5);                // [3,5] - existing segment
    this->indexes.addSegment(3, 4);                // {3,4} - new segment
    std::map<long, long> expectedResultEdgeCase4b; // (3,5) - expected segment
    expectedResultEdgeCase4b[3] = 45;
    EXPECT_NE(this->indexes.segments, expectedResultEdgeCase4b);
}

TEST_F(IndexSelectionTest, AddSegmentEdgeCaseB1_PASS)
{
    // PASS - Edge Case 4b : [ + { _ } _ ] : This test is an edge case that
    // should be handled as a B1 case.
    std::cout << "\tEdge Case 4b : [ + { _ } _ ].\n";
    this->indexes.addSegment(3, 5);                // [3,5] - existing segment
    this->indexes.addSegment(3, 4);                // {3,4} - new segment
    std::map<long, long> expectedResultEdgeCase4b; // (3,5) - expected segment
    expectedResultEdgeCase4b[3] = 5;
    EXPECT_EQ(this->indexes.segments, expectedResultEdgeCase4b);
}

TEST_F(IndexSelectionTest, AddSegmentEdgeCaseB2_PASS)
{
    // PASS - Edge Case 5 : { _ } + [ _ ] : This test is an edge case that
    // should be handled as a B2 Case.
    std::cout << "\tEdge Case 5 : { _ } + [ _ ].\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(1, 2);               // {1,2} - new segment
    std::map<long, long> expectedResultEdgeCase5; // (1,7) - expected segment
    expectedResultEdgeCase5[1] = 7;
    EXPECT_EQ(this->indexes.segments, expectedResultEdgeCase5);
}

TEST_F(IndexSelectionTest, AddSegmentEdgeCaseA3_FAIL)
{
    // FAIL - Edge Case 3 : { _ } + [ _ ] : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase5[1] = 77;
    // for segment not equal
    std::cout << "\tEdge Case 5 : { _ } + [ _ ].\n";
    this->indexes.addSegment(3, 5);               // [3,5] - existing segment
    this->indexes.addSegment(1, 2);               // {1,2} - new segment
    std::map<long, long> expectedResultEdgeCase5; // (1,7) - expected segment
    expectedResultEdgeCase5[1] = 77;
    EXPECT_NE(this->indexes.segments, expectedResultEdgeCase5);
}

TEST_F(IndexSelectionTest, getSegments_segments_not_empty)
{
    // When segments already exist, then we know that both spatial and temporal
    // constraints have been applied so we can just return the existing segment
    // map.

    // Case 1: Only spatial constraints exist.
    // Ensure that the output just returns the existing segment map.
    this->indexes.segments[42] = 5;
    this->indexes.segments[56] = 3;
    std::map<long, long> retrievedSegments = this->indexes.getSegments();
    EXPECT_EQ(this->indexes.segments, retrievedSegments);

    // Case 2: Spatial and temporal constraints exist.
    // Ensure that the output just returns the existing segment map.
    this->indexes.segments.clear();
    retrievedSegments.clear();
    this->indexes.minIndexStart = 23;
    this->indexes.maxIndexEnd = 51;
    retrievedSegments = this->indexes.getSegments();
    EXPECT_EQ(this->indexes.segments, retrievedSegments);
}

TEST_F(IndexSelectionTest, getSegments_segments_empty)
{
    // When segments is empty, temporal subsetting is not yet represented.

    // Case 1: Temporal constraints do not exist.
    // Ensure that no segments are added.
    this->indexes.minIndexStart = 0;
    this->indexes.maxIndexEnd = 0;
    std::map<long, long> retrievedSegments = this->indexes.getSegments();
    EXPECT_TRUE(retrievedSegments.empty());

    // Case 2: Temporal constraints do exist.
    // Ensure that the output segment map now contains exactly one segment.
    this->indexes.segments.clear();
    retrievedSegments.clear();
    this->indexes.minIndexStart = 0;
    this->indexes.maxIndexEnd = 35;
    int expectedLength =
        this->indexes.maxIndexEnd - this->indexes.minIndexStart;
    retrievedSegments = this->indexes.getSegments();
    EXPECT_EQ(1, retrievedSegments.size());
    EXPECT_EQ(expectedLength, retrievedSegments[0]);

    // Case 3: Temporal constraints cover all indexes.
    // Ensure that the output contains no segments.
    this->indexes.segments.clear();
    retrievedSegments.clear();
    this->indexes.minIndexStart = 0;
    this->indexes.maxIndexEnd = (this->indexes.getMaxSize());
    expectedLength = this->indexes.maxIndexEnd - this->indexes.minIndexStart;
    retrievedSegments = this->indexes.getSegments();
    EXPECT_TRUE(retrievedSegments.empty());
}
