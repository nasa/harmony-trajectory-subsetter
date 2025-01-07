#
# Test image for the Harmony sds/trajectory-subsetter service. This
# image uses the main service image, sds/trajectory-subsetter, as a base layer
# for the tests. This ensures  that the contents of the service image are
# tested, preventing discrepancies between the service and test environments.
#

FROM sds/trajectory-subsetter

ENV PYTHONDONTWRITEBYTECODE=1

WORKDIR /home

# Install additional Pip requirements (for testing)
COPY ./tests/pip_test_requirements.txt tests/pip_test_requirements.txt

RUN pip install --no-input --no-cache-dir -r tests/pip_test_requirements.txt

COPY ./tests tests
# Configure a container to be executable via the `docker run` command.
ENTRYPOINT ["/home/tests/run_tests.sh"]