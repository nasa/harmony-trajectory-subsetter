#
# GoogleTest image for the C++ subsetter of nasa/harmony-trajectory-subsetter.
#
# This image extends the "builder" stage of docker/service.Dockerfile, which
# already holds the compiler toolchain, Boost, libgeotiff, HDF5 built from
# source into /usr/local, and the subsetter source at /home/subsetter. On top
# of that it adds CMake and GoogleTest, copies in the test sources and data,
# and compiles the test executables. Running the container runs ctest.
#
# Build and run it with ./bin/build-gtest and ./bin/run-gtest.
#

FROM ghcr.io/nasa/harmony-trajectory-subsetter-builder

# gtest-devel and gmock-devel come from EPEL, which the builder stage already
# enables (along with the CRB repository).
RUN dnf -y install cmake gtest-devel gmock-devel && \
    dnf clean all

WORKDIR /home

# The tests read the subsetter configuration file and the granules under
# tests/data relative to the repository root, which is /home in this image
# (matching the /home/subsetter layout inherited from the builder stage).
COPY harmony_service/subsetter_config.json harmony_service/subsetter_config.json
COPY tests/data tests/data
COPY tests/unit/gtest tests/unit/gtest

RUN cmake -S tests/unit/gtest -B tests/unit/gtest/build && \
    cmake --build tests/unit/gtest/build -j"$(nproc)"

# ./bin/run-gtest mounts reports/gtest from the host here for the JUnit output.
RUN mkdir -p /home/reports/gtest

# Any arguments passed to `docker run` are forwarded to ctest, e.g.
# `-R Temporal` to run a subset of the tests.
ENTRYPOINT ["ctest", "--test-dir", "/home/tests/unit/gtest/build", \
            "--output-on-failure", \
            "--output-junit", "/home/reports/gtest/results.xml"]
