/**************************************************************************
 * File : UutIndexSelection.cpp
 *
 * This file contains a main script and unit tests for the addSegment()
 * function in the IndexSelection class. More unit tests for the
 * IndexSelection class may be added in the future.
 **************************************************************************/

#include <iostream>
#include "../../subsetter/IndexSelection.h"

// This function tests if two given maps (std::map<long,long>) are equivalent.
// Prints a SUCCESS or FAIL message accordingly.
void VerifyMapEquals(IndexSelection& aTestIdxSelection, std::map<long,long> aExpectedResult, std::string aTestName)
{
    if (aTestIdxSelection.segments == aExpectedResult)
    {
        std::cout << "\tSUCCESS: " << aTestName << " IndexSelection now " << aTestIdxSelection << ".\n\n";
    }
    else
    {
        std::cout << "\tFAIL" << "\n\n";
    }
}

// Unit test for function IndexSelection::addSegment().
void TestUutIndexSelection_addSegment()
{
    //  [ ] - existing segment
    //  { } - new segment
    //   _  - content
    //   +  - joined
    // testSegments: (start_index, length)

    long maxLength = 50;

    /* Simple Cases (overlap exists between new and existing segments, move right to left) */
    std::cout << "*** IndexSelection::addSegment Unit Tests: Simple Test Cases (overlap exists between new and existing segments, move right to left) ***\n\n";

    // PASS - Case A1 : [ _ ] _ { _ } : This test is a simple case for Case A1.
    std::cout << "\tCase A1 : [ _ ] _ { _ }.\n";
    IndexSelection testCaseA1(maxLength);
    testCaseA1.addSegment(3,5);            // [3,5]  - existing segment
    testCaseA1.addSegment(10,3);           // {10,3} - new segment
    std::map<long,long> expectedResultA1;  // (3,5), (10,3) - expected segments
    expectedResultA1[3] = 5;
    expectedResultA1[10] = 3;
    VerifyMapEquals(testCaseA1, expectedResultA1, "No merge required.");

    // PASS - Case A2 : [ _ { _ ] _ } : This test is a simple case for Case A2.
    std::cout << "\tCase A2 : [ _ { _ ] _ }.\n";
    IndexSelection testCaseA2(maxLength);
    testCaseA2.addSegment(3,5);            // [3,5] - existing segment
    testCaseA2.addSegment(4,6);            // {4,6} - new segment
    std::map<long,long> expectedResultA2;  // (3,7) - expected segment
    expectedResultA2[3] = 7;
    VerifyMapEquals(testCaseA2, expectedResultA2, "Merging segments.");

    // PASS - Case A3 : { _ [ _ ] _ } : This test is a simple case for Case A3.
    std::cout << "\tCase A3 : { _ [ _ ] _ }.\n";
    IndexSelection testCaseA3(maxLength);
    testCaseA3.addSegment(3,5);            // [3,5] - existing segment
    testCaseA3.addSegment(2,9);            // {2,9} - new segment
    std::map<long,long> expectedResultA3;  // (2,9) - expected segment
    expectedResultA3[2] = 9;
    VerifyMapEquals(testCaseA3, expectedResultA3, "Merging segments.");

    // PASS - Case B1 : [ _ { _ } _ ] : This test is a simple case for Case B1.
    std::cout << "\tCase B1 : [ _ { _ } _ ].\n";
    IndexSelection testCaseB1(maxLength);
    testCaseB1.addSegment(3,5);            // [3,5] - existing segment
    testCaseB1.addSegment(4,2);            // {4,2} - new segment
    std::map<long,long> expectedResultB1;  // (3,5) - expected segment
    expectedResultB1[3] = 5;
    VerifyMapEquals(testCaseB1, expectedResultB1, "Merging segments.");

    // PASS - Case B2 : { _ [ _ } _ ] : This test is a simple case for Case B2.
    std::cout << "\tCase B2 : { _ [ _ } _ ].\n";
    IndexSelection testCaseB2(maxLength);
    testCaseB2.addSegment(3,5);            // [3,5] - existing segment
    testCaseB2.addSegment(2,4);            // {2,4} - new segment
    std::map<long,long> expectedResultB2;  // (2,6) - expected segment
    expectedResultB2[2] = 6;
    VerifyMapEquals(testCaseB2, expectedResultB2, "Merging segments");

    // PASS - Case B3 : { _ } _ [ _ ] : This is a simple case for Case B3 (fall through case).
    std::cout << "\tCase B3 : { _ } _ [ _ ].\n";
    IndexSelection testCaseB3(maxLength);
    testCaseB3.addSegment(3,5);            // [3,5] - existing segment
    testCaseB3.addSegment(1,1);            // {1,1} - new segment
    std::map<long,long> expectedResultB3;  // (1,1), (3,5) - expected segment
    expectedResultB3[1] = 1;
    expectedResultB3[3] = 5;
    VerifyMapEquals(testCaseB3, expectedResultB3, "Merging segments");

    /* Edge Cases */
    std::cout << "*** IndexSelection::addSegment Unit Tests: Edge Cases ***\n\n";

    // PASS - Edge Case 1 : [ _ ] + { _ } : This test is an edge case that should be handled as a A2 Case.
    std::cout << "\tEdge Case 1 : [ _ ] + { _ }.\n";
    IndexSelection testEdgeCase1(maxLength);
    testEdgeCase1.addSegment(3,5);                // [3,5] - existing segment
    testEdgeCase1.addSegment(8,2);                // {8,2} - new segment
    std::map<long,long> expectedResultEdgeCase1;  // (3,7) - expected segments
    expectedResultEdgeCase1[3] = 7;
    VerifyMapEquals(testEdgeCase1, expectedResultEdgeCase1, "Merging segments, handled as a Case A2.");

    // PASS - Edge Case 1b : [ _ { _ ] + } : This test is an edge case that should be handled as a A2 case.
    std::cout << "\tEdge Case 1b : [ _ { _ ] + }.\n";
    IndexSelection testEdgeCase1b(maxLength);
    testEdgeCase1b.addSegment(3,5);                 // [3,5] - existing segment
    testEdgeCase1b.addSegment(4,4);                 // {4,4} - new segment
    std::map<long,long> expectedResultEdgeCase1b;   // (3,5) - expected segment
    expectedResultEdgeCase1b[3] = 5;
    VerifyMapEquals(testEdgeCase1b, expectedResultEdgeCase1b, "Merging segments, handled as a Case A2.");

    // PASS - Edge Case 2 : { + [ _ ] _ } : This test is an edge case that should be handled as a A2 Case.
    std::cout << "\tEdge Case 2 : { + [ _ ] _ }.\n";
    IndexSelection testEdgeCase2(maxLength);
    testEdgeCase2.addSegment(3,5);                // [3,5] - existing segment
    testEdgeCase2.addSegment(3,6);                // {3,6} - new segment
    std::map<long,long> expectedResultEdgeCase2;  // (3,6) - expected segment
    expectedResultEdgeCase2[3] = 6;
    VerifyMapEquals(testEdgeCase2, expectedResultEdgeCase2, "Merging segments, handled as a Case A2.");

    // PASS - Edge Case 3 : { _ [ _ ] + } : This test is an edge case that should be handled as a A3 Case.
    std::cout << "\tEdge Case 3 : { _ [ _ ] + }.\n";
    IndexSelection testEdgeCase3(maxLength);
    testEdgeCase3.addSegment(3,5);                // [3,5] - existing segment
    testEdgeCase3.addSegment(2,6);                // {2,6} - new segment
    std::map<long,long> expectedResultEdgeCase3;  // (2,6) - expected segment
    expectedResultEdgeCase3[2] = 6;
    VerifyMapEquals(testEdgeCase3, expectedResultEdgeCase3, "Merging segments, handled as a Case A3.");

    // PASS - Edge Case 4 : { + [ _ ] + } : This test is an edge case that should be handled as a A2 Case.
    std::cout << "\tEdge Case 4 : { + [ _ ] + }.\n";
    IndexSelection testEdgeCase4(maxLength);
    testEdgeCase4.addSegment(3,5);                // [3,5] - existing segment
    testEdgeCase4.addSegment(3,5);                // {3,5} - new segment
    std::map<long,long> expectedResultEdgeCase4;  // (3,5) - expected segment
    expectedResultEdgeCase4[3] = 5;
    VerifyMapEquals(testEdgeCase4, expectedResultEdgeCase4, "Merging segments, handled as a Case A4.");

    // PASS - Edge Case 4b : [ + { _ } _ ] : This test is an edge case that should be handled as a B1 case.
    std::cout << "\tEdge Case 4b : [ + { _ } _ ].\n";
    IndexSelection testEdgeCase4b(maxLength);
    testEdgeCase4b.addSegment(3,5);                // [3,5] - existing segment
    testEdgeCase4b.addSegment(3,4);                // {3,4} - new segment
    std::map<long,long> expectedResultEdgeCase4b;  // (3,5) - expected segment
    expectedResultEdgeCase4b[3] = 5;
    VerifyMapEquals(testEdgeCase4b, expectedResultEdgeCase4b, "Merging segments, handled as a 4b.");

    // PASS - Edge Case 5 : { _ } + [ _ ] : This test is an edge case that should be handled as a B2 Case.
    std::cout << "\tEdge Case 5 : { _ } + [ _ ].\n";
    IndexSelection testEdgeCase5(maxLength);
    testEdgeCase5.addSegment(3,5);                // [3,5] - existing segment
    testEdgeCase5.addSegment(1,2);                // {1,2} - new segment
    std::map<long,long> expectedResultEdgeCase5;  // (1,7) - expected segment
    expectedResultEdgeCase5[1] = 7;
    VerifyMapEquals(testEdgeCase5, expectedResultEdgeCase5, "Merging segments, handled as a Case B2.");

    std::cout << "Unit tests complete.\n";
}

// Main function to test above unit tests.
int main()
{
    TestUutIndexSelection_addSegment();

    return 0;
}
