#
# Service image for sds/trajectory-subsetter, a Harmony backend service subsets
# L2 segmented trajectory data, including variable, bounding box spatial, polygon
# spatial and temporal subsetting.
#
# This image builds binary file "subset" from C++ code situated in the subsetter
# directory, instantiates a conda environment, with required packages, before
# installing additional dependencies via Pip. The service code is then copied
# into the Docker image, before environment variables are set to activate the
# created conda environment.
#
FROM rockylinux:8

WORKDIR /home
# Add needed libraries
RUN dnf -y upgrade && \
    dnf -y install epel-release && \
    dnf config-manager --set-enabled powertools && \
    dnf -y install --skip-broken gcc-c++ make libjpeg-turbo hdf-devel libtool libxslt-devel \
        file gcc-gfortran redhat-rpm-config libgeotiff-devel java-1.8.0-openjdk-devel  proj-devel \
        netcdf-devel libaec-devel autogen boost-static mc which && \
    dnf clean all

# Build HDF5-1.8.22, HDFEOS and MINICONDA
ENV HDF5_URL="https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8/hdf5-1.8.22/src/hdf5-1.8.22.tar.gz"
ENV HDFEOS_URL="https://maven.earthdata.nasa.gov/repository/heg-c/HDF-EOS2/hdf-eos2-3.0-src.tar.gz"
ENV HDFEOS5_URL="https://maven.earthdata.nasa.gov/repository/heg-c/HDF-EOS5/hdf-eos5-2.0-src.tar.gz"
ENV MINICONDA="https://repo.anaconda.com/miniconda/Miniconda3-py311_24.4.0-0-Linux-x86_64.sh"


RUN set -e && \
  curl -sfSL ${HDF5_URL} > hdf5.tar.gz && \
  mkdir hdf5 && tar xzvf hdf5.tar.gz -C hdf5 --strip-components 1 && \
  cd hdf5 && ./configure  "--prefix=/home" --disable-shared --enable-cxx --enable-static-exec || { echo "ERROR: Configure failed"; exit 1; } && \
  make || { echo "ERROR: Make failed"; exit 1; } && \
  make install || { echo "ERROR: Install failed"; exit 1; } && cd .. && \
  rm -f hdf5.tar.gz

RUN set -e && \
  curl -sfSL ${HDFEOS_URL} > hdfeos.tar.gz && \
  mkdir hdfeos && tar xzvf hdfeos.tar.gz -C hdfeos --strip-components 1 && \
  cd hdfeos && ./configure && make && make install && cd .. && \
  rm -f hdfeos.tar.gz

RUN set -e && \
  curl -sfSL ${HDFEOS5_URL} > hdfeos5.tar.gz && \
  mkdir hdfeos5 && tar xzvf hdfeos5.tar.gz -C hdfeos5 --strip-components 1 && \
  cd hdfeos5 && ./configure && make && make install && cd .. && \
  rm -f hdfeos5.tar.gz

RUN set -e && \
  curl -sfSL ${MINICONDA} > miniconda.sh && \
  bash miniconda.sh -b -p /opt/conda

COPY subsetter subsetter

WORKDIR /home/subsetter
# Build binary file "subset" in home directory
RUN ./makeit_harmony

WORKDIR /home
RUN rm -rf ./subsetter ./hdfeos ./hdfeos5 ./hdf5

# Create Conda environment
ENV PATH="/opt/conda/bin:$PATH"

RUN conda create -y --name trajectorysubsetter python=3.11 -q \
    --channel conda-forge

# Copy additional Pip dependencies into the container
COPY harmony_service/pip_requirements.txt harmony_service/pip_requirements.txt
# Install additional Pip dependencies
RUN conda run --name trajectorysubsetter pip install --no-input -r harmony_service/pip_requirements.txt
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
ENTRYPOINT service: ["python", "harmony_service/adapter.py"]
