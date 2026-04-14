#
# Service image for sds/trajectory-subsetter, a Harmony backend service that subsets
# L2 segmented trajectory data, including variable, bounding box spatial, polygon
# spatial and temporal subsetting.
#
# This image builds binary file "subset" from C++ code situated in the subsetter
# directory, instantiates a conda environment, with required packages, before
# installing additional dependencies via Pip. The service code is then copied
# into the Docker image, before environment variables are set to activate the
# created conda environment.
#
FROM rockylinux:8 AS builder

WORKDIR /home
# Add needed libraries
RUN dnf -y upgrade && \
    dnf -y install epel-release && \
    dnf config-manager --set-enabled powertools && \
    dnf -y install gcc-c++ make libjpeg-turbo libgeotiff-devel proj-devel \
        libaec-devel boost-static hdf5-devel redhat-rpm-config && \
    dnf clean all

COPY subsetter subsetter

WORKDIR /home/subsetter
# Build binary file "subset" in home directory
RUN ./makeit_harmony

FROM rockylinux:8

WORKDIR /home
# Install runtime shared-library dependencies of the subset binary
RUN dnf -y upgrade && \
    dnf -y install epel-release && \
    dnf config-manager --set-enabled powertools && \
    dnf -y install libgeotiff libjpeg-turbo proj hdf5 python3.11 && \
    dnf clean all && \
    python3.11 -m ensurepip --upgrade && \
    ln -s /usr/bin/python3.11 /usr/bin/python && \
    ln -s /usr/local/bin/pip3.11 /usr/bin/pip

# Copy compiled binary from the builder stage
COPY --from=builder /home/subset /home/subset

COPY docker/service_version.txt docker/service_version.txt

# Copy additional Pip dependencies into the container
COPY harmony_service/pip_requirements.txt harmony_service/pip_requirements.txt
# Install additional Pip dependencies
RUN python3.11 -m pip install --no-input --no-cache-dir -r harmony_service/pip_requirements.txt
# Bundle app source
COPY ./harmony_service harmony_service

ENV PYTHONPATH="/home"

# Configure a container to be executable via the `docker run` command.
ENTRYPOINT ["python", "harmony_service/adapter.py"]
