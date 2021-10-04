cmake_minimum_required(VERSION 3.20)
project(trajectorysubsetter)

set(CMAKE_CXX_STANDARD 11)

include_directories(DataAccess
#        /usr/include/boost
        /tools/hdf5/bin
        /ecs/hdfeos/gctp/include
        /ecs/formal/HEG/lib/linux
        /tools/boost
        /ecs/formal/HEG/tiff/include
        /ecs/formal/HEG/geotiff/include
        /ecs/formal/HEG/geotiff/lib/linux
        /ecs/formal/DPL/DataAccess/scripts
        )
add_executable(trajectorysubsetter
        DataAccess/util/subsetter/Configuration.h
        DataAccess/util/subsetter/Coordinate.h
        DataAccess/util/subsetter/DatasetLinks.h
        DataAccess/util/subsetter/DimensionScales.h
        DataAccess/util/subsetter/ForwardReferenceCoordinates.h
        DataAccess/util/subsetter/geobox.h
        DataAccess/util/subsetter/GeoPolygon.h
        DataAccess/util/subsetter/geotiff_converter.h
        DataAccess/util/subsetter/HeightSegmentCoordinates.h
        DataAccess/util/subsetter/IcesatSubsetter.h
        DataAccess/util/subsetter/IndexSelection.h
        DataAccess/util/subsetter/Makefile
        DataAccess/util/subsetter/makeit
        DataAccess/util/subsetter/PhotonReferenceDatasets.h
        DataAccess/util/subsetter/ReferenceDatasets.h
        DataAccess/util/subsetter/ReverseReferenceCoordinates.h
        DataAccess/util/subsetter/Subset.cpp
        DataAccess/util/subsetter/SubsetDataLayers.h
        DataAccess/util/subsetter/Subsetter.h
        DataAccess/util/subsetter/SuperGroupCoordinate.h
        DataAccess/util/subsetter/SuperGroupSubsetter.h
        DataAccess/util/subsetter/Temporal.h)
