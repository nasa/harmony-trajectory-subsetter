# Create the dev image
#
FROM centos:7


# Set up additional packages needed to build the libraries
#
WORKDIR /home

# All needed libraries
RUN yum -y install epel-release && \
    yum -y install gcc-c++ make libjpeg-turbo zlib hdf-devel libtool libxslt-devel \
        libaec-devel autogen shtool autoconf automake file gcc-gfortran hdf5-devel \
        libgeotiff-devel java-1.8.0-openjdk-devel netcdf-devel proj-devel mc \
        boost-static && \
    yum clean all

# HDFEOS
ENV HDFEOS_URL="https://maven.earthdata.nasa.gov/repository/heg-c/HDF-EOS2/hdf-eos2-3.0-src.tar.gz"
ENV HDFEOS5_URL="https://maven.earthdata.nasa.gov/repository/heg-c/HDF-EOS5/hdf-eos5-2.0-src.tar.gz"

RUN set -e && \
  curl -sfSL ${HDFEOS_URL} > hdfeos.tar.gz && \
  mkdir hdfeos && tar xzvf hdfeos.tar.gz -C hdfeos --strip-components 1 && \
  cd hdfeos && ./configure && make && make install && \
  rm -f hdfeos.tar.gz

RUN set -e && \
  curl -sfSL ${HDFEOS5_URL} > hdfeos5.tar.gz && \
  mkdir hdfeos5 && tar xzvf hdfeos5.tar.gz -C hdfeos5 --strip-components 1 && \
  cd hdfeos5 && ./configure && make && make install && \
  rm -f hdfeos5.tar.gz

WORKDIR /home/subsetter
