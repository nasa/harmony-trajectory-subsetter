#include "../../../subsetter/IndexSelection.h"
#include <gtest/gtest.h>
#include <iostream>

// Global Value
long maxLength = 50;

//  [ ] - existing segment
//  { } - new segment
//   _  - content
//   +  - joined
// testSegments: (start_index, length)

TEST(TestIndexSelection, AddSegment_CaseA1_PASS)
{
    /* Simple Cases (overlap exists between new and existing segments, move
     * right to left) */
    std::cout
        << "*** IndexSelection::addSegment Unit Tests: Simple Test Cases "
           "(overlap exists between new and existing segments, move right "
           "to left) ***\n\n";

    // PASS - Case A1 : [ _ ] _ { _ } : This test is a simple case for Case A1.
    std::cout << "\tCase A1 : [ _ ] _ { _ }.\n";
    IndexSelection testCaseA1(maxLength);
    testCaseA1.addSegment(3, 5);           // [3,5]  - existing segment
    testCaseA1.addSegment(10, 3);          // {10,3} - new segment
    std::map<long, long> expectedResultA1; // (3,5), (10,3) - expected segments
    expectedResultA1[3] = 5;
    expectedResultA1[10] = 3;
    EXPECT_EQ(testCaseA1.segments, expectedResultA1);
}

TEST(TestIndexSelection, AddSegment_CaseA1_FAIL)
{
    // FAIL- Case A1 : [ _ ] _ { _ } : This test is a simple case for Case A1.
    // Change expectedResultA1[10] = 4 for segment not equal
    std::cout << "\tCase A1 : [ _ ] _ { _ }.\n";
    IndexSelection testCaseA1(maxLength);
    testCaseA1.addSegment(3, 5);           // [3,5]  - existing segment
    testCaseA1.addSegment(10, 3);          // {10,3} - new segment
    std::map<long, long> expectedResultA1; // (3,5), (10,3) - expected segments
    expectedResultA1[3] = 5;
    expectedResultA1[10] = 4; // Change from 3 to 4 segment not equal
    EXPECT_NE(testCaseA1.segments, expectedResultA1);
}

TEST(TestIndexSelection, AddSegment_CaseA2_PASS)
{
    // PASS - Case A2 : [ _ { _ ] _ } : This test is a simple case for Case A2.
    std::cout << "\tCase A2 : [ _ { _ ] _ }.\n";
    IndexSelection testCaseA2(maxLength);
    testCaseA2.addSegment(3, 5);           // [3,5] - existing segment
    testCaseA2.addSegment(4, 6);           // {4,6} - new segment
    std::map<long, long> expectedResultA2; // (3,7) - expected segment
    expectedResultA2[3] = 7;
    EXPECT_EQ(testCaseA2.segments, expectedResultA2);
}

TEST(TestIndexSelection, AddSegment_CaseA2_FAIL)
{
    // FAIL - Case A2 : [ _ { _ ] _ } : This test is a simple case for Case A2.
    // Change expectedResultA2[3] = 8 for segment not equal
    std::cout << "\tCase A2 : [ _ { _ ] _ }.\n";
    IndexSelection testCaseA2(maxLength);
    testCaseA2.addSegment(3, 5);           // [3,5] - existing segment
    testCaseA2.addSegment(4, 6);           // {4,6} - new segment
    std::map<long, long> expectedResultA2; // (3,7) - expected segment
    expectedResultA2[3] = 8;
    EXPECT_NE(testCaseA2.segments, expectedResultA2);
}

TEST(TestIndexSelection, AddSegment_CaseA3_PASS)
{
    // PASS - Case A3 : { _ [ _ ] _ } : This test is a simple case for Case A3.
    std::cout << "\tCase A3 : { _ [ _ ] _ }.\n";
    IndexSelection testCaseA3(maxLength);
    testCaseA3.addSegment(3, 5);           // [3,5] - existing segment
    testCaseA3.addSegment(2, 9);           // {2,9} - new segment
    std::map<long, long> expectedResultA3; // (2,9) - expected segment
    expectedResultA3[2] = 9;
    EXPECT_EQ(testCaseA3.segments, expectedResultA3);
}

TEST(TestIndexSelection, AddSegment_CaseA3_FAIL)
{
    // FAIL - Case A3 : { _ [ _ ] _ } : This test is a simple case for Case A3.
    // Change expectedResultA3[2] = 11 for segment not equal
    std::cout << "\tCase A3 : { _ [ _ ] _ }.\n";
    IndexSelection testCaseA3(maxLength);
    testCaseA3.addSegment(3, 5);           // [3,5] - existing segment
    testCaseA3.addSegment(2, 9);           // {2,9} - new segment
    std::map<long, long> expectedResultA3; // (2,9) - expected segment
    expectedResultA3[2] = 11;
    EXPECT_NE(testCaseA3.segments, expectedResultA3);
}

TEST(TestIndexSelection, AddSegment_CaseB1_PASS)
{
    // PASS - Case B1 : [ _ { _ } _ ] : This test is a simple case for Case B1.
    std::cout << "\tCase B1 : [ _ { _ } _ ].\n";
    IndexSelection testCaseB1(maxLength);
    testCaseB1.addSegment(3, 5);           // [3,5] - existing segment
    testCaseB1.addSegment(4, 2);           // {4,2} - new segment
    std::map<long, long> expectedResultB1; // (3,5) - expected segment
    expectedResultB1[3] = 5;
    EXPECT_EQ(testCaseB1.segments, expectedResultB1);
}

TEST(TestIndexSelection, AddSegment_CaseB1_FAIL)
{
    // FAIL - Case B1 : [ _ { _ } _ ] : This test is a simple case for Case B1.
    // Change expectedResultB1[3] = 15 for segment not equal
    std::cout << "\tCase B1 : [ _ { _ } _ ].\n";
    IndexSelection testCaseB1(maxLength);
    testCaseB1.addSegment(3, 5);           // [3,5] - existing segment
    testCaseB1.addSegment(4, 2);           // {4,2} - new segment
    std::map<long, long> expectedResultB1; // (3,5) - expected segment
    expectedResultB1[3] = 15;
    EXPECT_NE(testCaseB1.segments, expectedResultB1);
}

TEST(TestIndexSelection, AddSegment_CaseB2_PASS)
{
    // PASS - Case B2 : { _ [ _ } _ ] : This test is a simple case for Case B2.
    std::cout << "\tCase B2 : { _ [ _ } _ ].\n";
    IndexSelection testCaseB2(maxLength);
    testCaseB2.addSegment(3, 5);           // [3,5] - existing segment
    testCaseB2.addSegment(2, 4);           // {2,4} - new segment
    std::map<long, long> expectedResultB2; // (2,6) - expected segment
    expectedResultB2[2] = 6;
    EXPECT_EQ(testCaseB2.segments, expectedResultB2);
}

TEST(TestIndexSelection, AddSegment_CaseB2_FAIL)
{
    // FAIL - Case B2 : { _ [ _ } _ ] : This test is a simple case for Case B2.
    // Change expectedResultB2[2] = 16 for segment not equal
    std::cout << "\tCase B2 : { _ [ _ } _ ].\n";
    IndexSelection testCaseB2(maxLength);
    testCaseB2.addSegment(3, 5);           // [3,5] - existing segment
    testCaseB2.addSegment(2, 4);           // {2,4} - new segment
    std::map<long, long> expectedResultB2; // (2,6) - expected segment
    expectedResultB2[2] = 16;
    EXPECT_NE(testCaseB2.segments, expectedResultB2);
}

TEST(TestIndexSelection, AddSegment_CaseB3_PASS)
{
    // PASS - Case B3 : { _ } _ [ _ ] : This is a simple case for Case B3 (fall
    // through case).
    std::cout << "\tCase B3 : { _ } _ [ _ ].\n";
    IndexSelection testCaseB3(maxLength);
    testCaseB3.addSegment(3, 5);           // [3,5] - existing segment
    testCaseB3.addSegment(1, 1);           // {1,1} - new segment
    std::map<long, long> expectedResultB3; // (1,1), (3,5) - expected segment
    expectedResultB3[1] = 1;
    expectedResultB3[3] = 5;
    EXPECT_EQ(testCaseB3.segments, expectedResultB3);
}

TEST(TestIndexSelection, AddSegment_CaseB3_FAIL)
{
    // FAIL - Case B3 : { _ } _ [ _ ] : This is a simple case for Case B3 (fall
    // through case). Change expectedResultB3[1] = 11; and expectedResultB3[3] =
    // 15; for segment not equal
    std::cout << "\tCase B3 : { _ } _ [ _ ].\n";
    IndexSelection testCaseB3(maxLength);
    testCaseB3.addSegment(3, 5);           // [3,5] - existing segment
    testCaseB3.addSegment(1, 1);           // {1,1} - new segment
    std::map<long, long> expectedResultB3; // (1,1), (3,5) - expected segment
    expectedResultB3[1] = 11;
    expectedResultB3[3] = 15;
    EXPECT_NE(testCaseB3.segments, expectedResultB3);
}

TEST(TestIndexSelection, AddSegment_EdgeCase1_PASS)
{
    /* Edge Cases */
    std::cout
        << "*** IndexSelection::addSegment Unit Tests: Edge Cases ***\n\n";

    // PASS - Edge Case 1 : [ _ ] + { _ } : This test is an edge case that
    // should be handled as a A2 Case.
    std::cout << "\tEdge Case 1 : [ _ ] + { _ }.\n";
    IndexSelection testEdgeCase1(maxLength);
    testEdgeCase1.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase1.addSegment(8, 2);               // {8,2} - new segment
    std::map<long, long> expectedResultEdgeCase1; // (3,7) - expected segments
    expectedResultEdgeCase1[3] = 7;
    EXPECT_EQ(testEdgeCase1.segments, expectedResultEdgeCase1);
}

TEST(TestIndexSelection, AddSegment_EdgeCase1_FAIL)
{
    // FAIL - Edge Case 1 : [ _ ] + { _ } : This test is an edge case that
    // should be handled as a A2 Case. Change expectedResultEdgeCase1[3] = 17;
    // for segment not equal
    std::cout << "\tEdge Case 1 : [ _ ] + { _ }.\n";
    IndexSelection testEdgeCase1(maxLength);
    testEdgeCase1.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase1.addSegment(8, 2);               // {8,2} - new segment
    std::map<long, long> expectedResultEdgeCase1; // (3,7) - expected segments
    expectedResultEdgeCase1[3] = 17;
    EXPECT_NE(testEdgeCase1.segments, expectedResultEdgeCase1);
}

TEST(TestIndexSelection, AddSegment_EdgeCase1b_PASS)
{
    // PASS - Edge Case 1b : [ _ { _ ] + } : This test is an edge case that
    // should be handled as a A2 case.
    std::cout << "\tEdge Case 1b : [ _ { _ ] + }.\n";
    IndexSelection testEdgeCase1b(maxLength);
    testEdgeCase1b.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase1b.addSegment(4, 4);               // {4,4} - new segment
    std::map<long, long> expectedResultEdgeCase1b; // (3,5) - expected segment
    expectedResultEdgeCase1b[3] = 5;
    EXPECT_EQ(testEdgeCase1b.segments, expectedResultEdgeCase1b);
}

TEST(TestIndexSelection, AddSegment_EdgeCase1b_FAIL)
{
    // FAIL - Edge Case 1b : [ _ { _ ] + } : This test is an edge case that
    // should be handled as a A2 case. Change expectedResultEdgeCase1b[3] = 25;
    // for segment not equal
    std::cout << "\tEdge Case 1b : [ _ { _ ] + }.\n";
    IndexSelection testEdgeCase1b(maxLength);
    testEdgeCase1b.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase1b.addSegment(4, 4);               // {4,4} - new segment
    std::map<long, long> expectedResultEdgeCase1b; // (3,5) - expected segment
    expectedResultEdgeCase1b[3] = 25;
    EXPECT_NE(testEdgeCase1b.segments, expectedResultEdgeCase1b);
}

TEST(TestIndexSelection, AddSegment_EdgeCase2_PASS)
{
    // PASS - Edge Case 2 : { + [ _ ] _ } : This test is an edge case that
    // should be handled as a A2 Case.
    std::cout << "\tEdge Case 2 : { + [ _ ] _ }.\n";
    IndexSelection testEdgeCase2(maxLength);
    testEdgeCase2.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase2.addSegment(3, 6);               // {3,6} - new segment
    std::map<long, long> expectedResultEdgeCase2; // (3,6) - expected segment
    expectedResultEdgeCase2[3] = 6;
    EXPECT_EQ(testEdgeCase2.segments, expectedResultEdgeCase2);
}

TEST(TestIndexSelection, AddSegment_EdgeCase2_FAIL)
{
    // FAIL - Edge Case 1b : [ _ { _ ] + } : This test is an edge case that
    // should be handled as a A2 case. Change expectedResultEdgeCase2[3] = 26;
    // for segment not equal
    std::cout << "\tEdge Case 2 : { + [ _ ] _ }.\n";
    IndexSelection testEdgeCase2(maxLength);
    testEdgeCase2.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase2.addSegment(3, 6);               // {3,6} - new segment
    std::map<long, long> expectedResultEdgeCase2; // (3,6) - expected segment
    expectedResultEdgeCase2[3] = 26;
    EXPECT_NE(testEdgeCase2.segments, expectedResultEdgeCase2);
}

TEST(TestIndexSelection, AddSegment_EdgeCase3_PASS)
{
    // PASS - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case.
    std::cout << "\tEdge Case 3 : { _ [ _ ] + }.\n";
    IndexSelection testEdgeCase3(maxLength);
    testEdgeCase3.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase3.addSegment(2, 6);               // {2,6} - new segment
    std::map<long, long> expectedResultEdgeCase3; // (2,6) - expected segment
    expectedResultEdgeCase3[2] = 6;
    EXPECT_EQ(testEdgeCase3.segments, expectedResultEdgeCase3);
}

TEST(TestIndexSelection, AddSegment_EdgeCase3_FAIL)
{
    // FAIL - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase2[3] = 26;
    // for segment not equal
    std::cout << "\tEdge Case 2 : { + [ _ ] _ }.\n";
    IndexSelection testEdgeCase2(maxLength);
    testEdgeCase2.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase2.addSegment(3, 6);               // {3,6} - new segment
    std::map<long, long> expectedResultEdgeCase2; // (3,6) - expected segment
    expectedResultEdgeCase2[3] = 36;
    EXPECT_NE(testEdgeCase2.segments, expectedResultEdgeCase2);
}

TEST(TestIndexSelection, AddSegment_EdgeCase4_PASS)
{
    std::cout << "\tEdge Case 4 : { + [ _ ] + }.\n";
    IndexSelection testEdgeCase4(maxLength);
    testEdgeCase4.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase4.addSegment(3, 5);               // {3,5} - new segment
    std::map<long, long> expectedResultEdgeCase4; // (3,5) - expected segment
    expectedResultEdgeCase4[3] = 5;
    EXPECT_EQ(testEdgeCase4.segments, expectedResultEdgeCase4);
}

TEST(TestIndexSelection, AddSegment_EdgeCase4_FAIL)
{
    // FAIL - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase4[3] = 35;
    // for segment not equal
    std::cout << "\tEdge Case 4 : { + [ _ ] + }.\n";
    IndexSelection testEdgeCase4(maxLength);
    testEdgeCase4.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase4.addSegment(3, 5);               // {3,5} - new segment
    std::map<long, long> expectedResultEdgeCase4; // (3,5) - expected segment
    expectedResultEdgeCase4[3] = 35;
    EXPECT_NE(testEdgeCase4.segments, expectedResultEdgeCase4);
}

TEST(TestIndexSelection, AddSegment_EdgeCase4b_PASS)
{
    // PASS - Edge Case 4b : [ + { _ } _ ] : This test is an edge case that
    // should be handled as a B1 case.
    std::cout << "\tEdge Case 4b : [ + { _ } _ ].\n";
    IndexSelection testEdgeCase4b(maxLength);
    testEdgeCase4b.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase4b.addSegment(3, 4);               // {3,4} - new segment
    std::map<long, long> expectedResultEdgeCase4b; // (3,5) - expected segment
    expectedResultEdgeCase4b[3] = 5;
    EXPECT_EQ(testEdgeCase4b.segments, expectedResultEdgeCase4b);
}

TEST(TestIndexSelection, AddSegment_EdgeCase4b_FAIL)
{
    // FAIL - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase4b[3] = 45;
    // for segment not equal
    std::cout << "\tEdge Case 4b : [ + { _ } _ ].\n";
    IndexSelection testEdgeCase4b(maxLength);
    testEdgeCase4b.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase4b.addSegment(3, 4);               // {3,4} - new segment
    std::map<long, long> expectedResultEdgeCase4b; // (3,5) - expected segment
    expectedResultEdgeCase4b[3] = 45;
    EXPECT_NE(testEdgeCase4b.segments, expectedResultEdgeCase4b);
}

TEST(TestIndexSelection, AddSegment_EdgeCase5_PASS)
{
    // PASS - Edge Case 5 : { _ } + [ _ ] : This test is an edge case that
    // should be handled as a B2 Case.
    std::cout << "\tEdge Case 5 : { _ } + [ _ ].\n";
    IndexSelection testEdgeCase5(maxLength);
    testEdgeCase5.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase5.addSegment(1, 2);               // {1,2} - new segment
    std::map<long, long> expectedResultEdgeCase5; // (1,7) - expected segment
    expectedResultEdgeCase5[1] = 7;
    EXPECT_EQ(testEdgeCase5.segments, expectedResultEdgeCase5);
}

TEST(TestIndexSelection, AddSegment_EdgeCase5_FAIL)
{
    // FAIL - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that
    // should be handled as a A3 Case. Change expectedResultEdgeCase5[1] = 77;
    // for segment not equal
    std::cout << "\tEdge Case 5 : { _ } + [ _ ].\n";
    IndexSelection testEdgeCase5(maxLength);
    testEdgeCase5.addSegment(3, 5);               // [3,5] - existing segment
    testEdgeCase5.addSegment(1, 2);               // {1,2} - new segment
    std::map<long, long> expectedResultEdgeCase5; // (1,7) - expected segment
    expectedResultEdgeCase5[1] = 77;
    EXPECT_NE(testEdgeCase5.segments, expectedResultEdgeCase5);
}
