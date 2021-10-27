#!/bin/sh
####################################
#
# A script invoked by the test Dockerfile to run the Python `unittest` suite
# for the Harmonised trajectory subsetter. The script first runs the test
# suite, then it checks for linting errors.
#
# Security vulnerabilities are now checked via Snyk, in a Bitbucket web-hook.
#
# Adapted from Variable Subsetter/HOSS project, 2021-10-06
#
####################################

# Exit status used to report back to caller
STATUS=0

export ENV=test
export HDF5_DISABLE_VERSION_CHECK=1

# Run the standard set of unit tests, producing JUnit compatible output
coverage run -m xmlrunner discover tests -o tests/reports
RESULT=$?

if [ "$RESULT" -ne "0" ]; then
    STATUS=1
    echo "ERROR: unittest generated errors"
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
