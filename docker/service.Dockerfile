#
# Service image for sds/trajectory-subsetter, a Harmony backend service that subsets
# L2 segmented trajectory data, including variable, bounding box spatial, polygon
# spatial and temporal subsetting.
#
# This is a multi-stage build (https://docs.docker.com/build/building/multi-stage/).
# The "builder" stage compiles the C++ subset binary along with its heavy build-time
# dependencies (HDF5 source, compiler toolchain). The final stage starts from a
# clean base image and copies only the compiled binary and runtime libraries from
# the builder, keeping the shipped image lean.
#
FROM rockylinux:9 AS builder

WORKDIR /tmp/build
# Add needed libraries
RUN dnf -y upgrade && \
    dnf -y install epel-release && \
    dnf config-manager --set-enabled crb && \
    dnf -y install gcc-c++ make libjpeg-turbo libgeotiff-devel proj-devel \
        libaec-devel boost-static redhat-rpm-config wget zlib-devel && \
    dnf clean all

# Build HDF5 from source.
# 1.14.6 is the latest 1.14.x patch release; it is >= 1.14.4-2 (satisfying the
# vulnerability-fix requirement) and avoids the UTF-8 filename regression that
# shipped in 1.14.4 and 1.14.5.
# --prefix=/usr/local so the install lands in a standard Unix location that
# Rocky's default ld.so.conf already searches.
# --disable-sharedlib-rpath stops h5c++ from embedding an RPATH of
# /usr/local/lib into the compiled binary; we rely on ldconfig at runtime.
ARG HDF5_VERSION=1.14.6
RUN wget "https://github.com/HDFGroup/hdf5/releases/download/hdf5_${HDF5_VERSION}/hdf5-${HDF5_VERSION}.tar.gz" && \
    tar xzf "hdf5-${HDF5_VERSION}.tar.gz" && \
    cd "hdf5-${HDF5_VERSION}" && \
    ./configure \
        --prefix=/usr/local \
        --enable-shared \
        --enable-cxx \
        --enable-hl \
        --disable-sharedlib-rpath \
        --disable-tests \
        --disable-tools && \
    make -j"$(nproc)" && \
    make install && \
    cd / && \
    rm -rf /tmp/build

COPY subsetter /home/subsetter

WORKDIR /home/subsetter
# Build binary file "subset" in home directory
RUN ./makeit_harmony

FROM rockylinux:9

WORKDIR /home
# Install runtime shared-library dependencies of the subset binary.
# libaec provides libsz.so.2, which HDF5 links against when built with
# libaec-devel present in the builder stage.
RUN dnf -y upgrade && \
    dnf -y install epel-release && \
    dnf config-manager --set-enabled crb && \
    dnf -y install libgeotiff libjpeg-turbo proj libaec python3.13 && \
    dnf clean all && \
    python3.13 -m ensurepip --upgrade && \
    ln -s /usr/bin/python3.13 /usr/bin/python && \
    ln -s /usr/local/bin/pip3.13 /usr/bin/pip

# Copy the HDF5 install tree from the builder. Using /usr/local/lib (not a
# glob) preserves soname symlinks and keeps the runtime paths identical to
# the build-time paths, so no RPATH or LD_LIBRARY_PATH tricks are needed.
COPY --from=builder /usr/local/lib/ /usr/local/lib/
RUN ldconfig

# Copy compiled binary from the builder stage
COPY --from=builder /home/subset /home/subset

COPY docker/service_version.txt docker/service_version.txt

# Copy additional Pip dependencies into the container
COPY harmony_service/pip_requirements.txt harmony_service/pip_requirements.txt
# Install additional Pip dependencies
RUN python3.13 -m pip install --no-input --no-cache-dir -r harmony_service/pip_requirements.txt
# Bundle app source
COPY ./harmony_service harmony_service

ENV PYTHONPATH="/home"

# Configure a container to be executable via the `docker run` command.
ENTRYPOINT ["python", "harmony_service/adapter.py"]
