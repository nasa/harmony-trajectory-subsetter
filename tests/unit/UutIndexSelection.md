# Documentation for IndexSelection Unit Tests
The C++ unit tests for the Trajectory Subsetter IndexSelection class are located in trajectorysubsetter/tests/unit/UutIndexSelection.cpp. This file UutIndexSelection.cpp also contains a main function that executes these tests.

Currently, this file only contains tests for the IndexSelection::addSegment() function. Unit tests for more functions may be added in the future.
<br>

## Test Descriptions

Each test prints a "SUCCESS" or "FAIL" message with a brief description of the completed test. The following provides descriptions for the current tests in UutIndexSelection.cpp.
<br>

---------

###  *__IndexSelection::addSegment()__* Unit Tests
The structure of these tests is based around checking each possible case where two data segments can be combined within the *std::map<long,long> segments* object in the IndexSelection class.. The key being used to describe each possible case uses brackets and braces:

&nbsp;&nbsp;&nbsp;&nbsp; [ ] - existing segment <br>
&nbsp;&nbsp;&nbsp;&nbsp; { } - new segment <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  _ &nbsp; - content <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  + &nbsp; - joined

The cases covered are the labeled as the following:

Simple Cases:
- Case A1 :  &nbsp; [ _ ] _ { _ }
- Case A2 :  &nbsp; [ _ { _ ] _ }
- Case A3 :  &nbsp; { _ [ _ ] _ }
- Case B1 :  &nbsp; [ _ { _ } _ ]
- Case B2 :  &nbsp; { _ [ _ } _ ]
- Case B3 :  &nbsp; { _ } _ [ _ ]

Edge Cases:
- Edge Case 1 : &nbsp;  [ _ ] + { _ }
- Edge Case 1b : &nbsp; [ _ { _ ] + }
- Edge Case 2 : &nbsp;  { + [ _ ] _ }
- Edge Case 3 : &nbsp;  { _ [ _ ] + }
- Edge Case 4 : &nbsp;  { + [ _ ] + }
- Edge Case 4b : &nbsp;  [ + { _ } _ ]
- Edge Case 5 : &nbsp;  { _ } + [ _ ]

The structure of each test is to simply construct an IndexSelection object, add a segment to the empty *IndexSelection::segments* map, then add a second segment to that map that follows whichever above case is being considered. The resulting segment map is then compared to the expected map, and a "SUCCESS" or "FAIL" message is printed to the console accordingly.
<br>

--------


## Unit Test Logs
Currently (as of 05/19/22), the following is the log that is printed to the console upon these unit tests are executed.

###  *__IndexSelection::addSegment()__* Unit Test Log:

     *** IndexSelection::addSegment Unit Tests: Simple Test Cases (overlap exists between new and existing segments, move right to left) ***

	Case A1 : [ _ ] _ { _ }.
	Adding segment (3, 5).
	Adding segment (10, 3).
	SUCCESS: No merge required. IndexSelection now [  (3,5)  (10,3)  ].

	Case A2 : [ _ { _ ] _ }.
	Adding segment (3, 5).
	Adding segment (4, 6).
	SUCCESS: Merging segments. IndexSelection now [  (3,7)  ].

	Case A3 : { _ [ _ ] _ }.
	Adding segment (3, 5).
	Adding segment (2, 9).
	Adding segment (2, 9).
	SUCCESS: Merging segments. IndexSelection now [  (2,9)  ].

	Case B1 : [ _ { _ } _ ].
	Adding segment (3, 5).
	Adding segment (4, 2).
	SUCCESS: Merging segments. IndexSelection now [  (3,5)  ].

	Case B2 : { _ [ _ } _ ].
	Adding segment (3, 5).
	Adding segment (2, 4).
	Adding segment (2, 6).
	SUCCESS: Merging segments IndexSelection now [  (2,6)  ].

	Case B3 : { _ } _ [ _ ].
	Adding segment (3, 5).
	Adding segment (1, 1).
	SUCCESS: Merging segments IndexSelection now [  (1,1)  (3,5)  ].

    *** IndexSelection::addSegment Unit Tests: Edge Cases ***

	Edge Case 1 : [ _ ] + { _ }.
	Adding segment (3, 5).
	Adding segment (8, 2).
	SUCCESS: Merging segments, handled as a Case A2. IndexSelection now [  (3,7)  ].

	Edge Case 1b : [ _ { _ ] + }.
	Adding segment (3, 5).
	Adding segment (4, 4).
	SUCCESS: Merging segments, handled as a Case A2. IndexSelection now [  (3,5)  ].

	Edge Case 2 : { + [ _ ] _ }.
	Adding segment (3, 5).
	Adding segment (3, 6).
	SUCCESS: Merging segments, handled as a Case A2. IndexSelection now [  (3,6)  ].

	Edge Case 3 : { _ [ _ ] + }.
	Adding segment (3, 5).
	Adding segment (2, 6).
	Adding segment (2, 6).
	SUCCESS: Merging segments, handled as a Case A3. IndexSelection now [  (2,6)  ].

	Edge Case 4 : { + [ _ ] + }.
	Adding segment (3, 5).
	Adding segment (3, 5).
	SUCCESS: Merging segments, handled as a Case A4. IndexSelection now [  (3,5)  ].

	Edge Case 4b : [ + { _ } _ ].
	Adding segment (3, 5).
	Adding segment (3, 4).
	SUCCESS: Merging segments, handled as a 4b. IndexSelection now [  (3,5)  ].

	Edge Case 5 : { _ } + [ _ ].
	Adding segment (3, 5).
	Adding segment (1, 2).
	Adding segment (1, 7).
	SUCCESS: Merging segments, handled as a Case B2. IndexSelection now [  (1,7)  ].

    Unit tests complete.