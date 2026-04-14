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
    dnf -y install libgeotiff libjpeg-turbo proj hdf5 && \
    dnf clean all

# Copy compiled binary from the builder stage
COPY --from=builder /home/subset /home/subset

COPY docker/service_version.txt docker/service_version.txt

# Install MINICONDA
ENV MINICONDA="https://repo.anaconda.com/miniconda/Miniconda3-py311_24.4.0-0-Linux-x86_64.sh"

RUN set -e && \
  curl -sfSL ${MINICONDA} > miniconda.sh && \
  bash miniconda.sh -b -p /opt/conda

# Create Conda environment
ENV PATH="/opt/conda/bin:$PATH"

RUN conda config --remove channels defaults
RUN conda create -y --name trajectorysubsetter python=3.11 -q \
    --channel conda-forge --channel nodefaults

# Copy additional Pip dependencies into the container
COPY harmony_service/pip_requirements.txt harmony_service/pip_requirements.txt
# Install additional Pip dependencies
RUN conda run --name trajectorysubsetter pip install --no-input --no-cache-dir -r harmony_service/pip_requirements.txt
# Bundle app source
COPY ./harmony_service harmony_service
# Set conda environment to trajectorysubsetter, as conda run will not stream logging.
# Setting these environment variables is the equivalent of `conda activate`.
# The PYTHONPATH environment variable is also included to ensure the correct
# import paths are available to the service when invoking via the command line.
ENV _CE_CONDA='' \
    _CE_M='' \
    CONDA_DEFAULT_ENV=trajectorysubsetter \
    CONDA_EXE=/opt/conda/bin/conda \
    CONDA_PREFIX=/opt/conda/envs/trajectorysubsetter \
    CONDA_PREFIX_1=/opt/conda \
    CONDA_PROMPT_MODIFIER=(trajectorysubsetter) \
    CONDA_PYTHON_EXE=/opt/conda/bin/python \
    CONDA_ROOT=/opt/conda \
    CONDA_SHLVL=2 \
    PATH="/opt/conda/envs/trajectorysubsetter/bin:${PATH}" \
    SHLVL=1 \
    PYTHONPATH="/home"

# Configure a container to be executable via the `docker run` command.
ENTRYPOINT ["python", "harmony_service/adapter.py"]
