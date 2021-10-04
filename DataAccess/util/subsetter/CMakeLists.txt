cmake_minimum_required(VERSION 3.20)
project(trajectorysubsetter)

set(CMAKE_CXX_STANDARD 11)

include_directories(../..
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
        Configuration.h
        Coordinate.h
        DatasetLinks.h
        DimensionScales.h
        ForwardReferenceCoordinates.h
        geobox.h
        GeoPolygon.h
        geotiff_converter.h
        HeightSegmentCoordinates.h
        IcesatSubsetter.h
        IndexSelection.h
        Makefile
        makeit
        PhotonReferenceDatasets.h
        ReferenceDatasets.h
        ReverseReferenceCoordinates.h
        Subset.cpp
        SubsetDataLayers.h
        Subsetter.h
        SuperGroupCoordinate.h
        SuperGroupSubsetter.h
        Temporal.h)
