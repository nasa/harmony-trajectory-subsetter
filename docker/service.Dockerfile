#
# Service image for sds/trajectory-subsetter, a Harmony backend service subsets
# L2 segmented trajectory data, including variable, bounding box spatial, polygon
# spatial and temporal subsetting.
#
# This image builds binary file "subset" from C++ code situated in the subsetter
# directory, instantiates a conda environment, with required pacakges, before
# installing additional dependencies via Pip. The service code is then copied
# into the Docker image, before environment variables are set to activate the
# created conda environment.
#
FROM centos:7

WORKDIR /home
# All needed libraries
RUN yum -y install epel-release && \
    yum -y install gcc-c++ make libjpeg-turbo zlib hdf-devel libtool libxslt-devel \
        libaec-devel autogen shtool autoconf automake file gcc-gfortran hdf5-devel \
        libgeotiff-devel java-1.8.0-openjdk-devel netcdf-devel proj-devel mc \
        boost-static && \
    yum clean all

# Build HDFEOS and MINICONDA
ENV HDFEOS_URL="https://maven.earthdata.nasa.gov/repository/heg-c/HDF-EOS2/hdf-eos2-3.0-src.tar.gz"
ENV HDFEOS5_URL="https://maven.earthdata.nasa.gov/repository/heg-c/HDF-EOS5/hdf-eos5-2.0-src.tar.gz"
ENV MINICONDA="https://repo.anaconda.com/miniconda/Miniconda3-py39_4.10.3-Linux-x86_64.sh"

RUN set -e && \
  curl -sfSL ${HDFEOS_URL} > hdfeos.tar.gz && \
  mkdir hdfeos && tar xzvf hdfeos.tar.gz -C hdfeos --strip-components 1 && \
  cd hdfeos && ./configure && make && make install
RUN rm -f hdfeos.tar.gz

RUN set -e && \
  curl -sfSL ${HDFEOS5_URL} > hdfeos5.tar.gz && \
  mkdir hdfeos5 && tar xzvf hdfeos5.tar.gz -C hdfeos5 --strip-components 1 && \
  cd hdfeos5 && ./configure && make && make install
RUN rm -f hdfeos5.tar.gz

RUN set -e && \
  curl -sfSL ${MINICONDA} > miniconda.sh && \
  bash miniconda.sh -b -p /opt/conda

COPY subsetter subsetter

WORKDIR /home/subsetter
# Build binary file "subset" in home directory
RUN make

WORKDIR /home
# Bundle app source
COPY ./harmony_service harmony_service
RUN rm -rf ./subsetter ./hdfeos ./hdfeos5

# Create Conda environment
ENV PATH="/opt/conda/bin:$PATH"

RUN conda create -y --name trajectorysubsetter python=3.9 -q \
    --channel conda-forge \
    --channel defaults

# Copy additional Pip dependencies into the container
COPY harmony_service/pip_requirements.txt harmony_service/pip_requirements.txt
# Install additional Pip dependencies
RUN conda run --name trajectorysubsetter pip install --no-input -r harmony_service/pip_requirements.txt

# Set conda environment to trajectorysubsetter, as conda run will not stream logging.
# Setting these environment variables is the equivalent of `conda activate`.
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
    SHLVL=1

# Configure a container to be executable via the `docker run` command.
ENTRYPOINT service: ["python", "harmony_service/adapter.py"]