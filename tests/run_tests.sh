#!/bin/sh
##########################################################################
#
# A script invoked by the test Dockerfile to run the Python unittest
# and pytest tests for the Harmony Trajectory Subsetter (pytest runs both
# pytest and unittest tests).
# The script first runs the test suite, then it checks for linting errors.
#
##########################################################################

# Exit status used to report back to caller
STATUS=0

export ENV=test
export HDF5_DISABLE_VERSION_CHECK=1

# Run all tests using pytest, producing JUnit compatible output
echo "\nRunning tests..."
coverage run -m pytest tests/ --junitxml=tests/reports/pytest-results.xml -v
RESULT=$?

if [ "$RESULT" -ne "0" ]; then
    STATUS=1
    echo "ERROR: tests generated errors"
fi

echo "\n"
echo "Test Coverage Estimates"
coverage report --omit="*tests/*"
coverage html --omit="*tests/*" -d /home/tests/coverage

# Run pylint
pylint harmony_service --disable=E0401
RESULT=$?
RESULT=$((3 & $RESULT))

if [ "$RESULT" -ne "0" ]; then
    STATUS=1
    echo "ERROR: pylint generated errors"
fi

exit $STATUS
