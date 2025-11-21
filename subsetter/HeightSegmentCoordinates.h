#ifndef HEIGHTSEGMENTCOORDINATES_H
#define HEIGHTSEGMENTCOORDINATES_H

#include "Configuration.h"
#include "Coordinate.h"
#include "LogLevel.h"

#include "H5Cpp.h"
#include <boost/algorithm/string/find.hpp>

class HeightSegmentCoordinates : public Coordinate
{
  public:
    HeightSegmentCoordinates(std::string groupname,
                             std::vector<geobox> *geoboxes,
                             Temporal *temporal,
                             GeoPolygon *geoPolygon,
                             Configuration *config)
        : Coordinate(groupname, geoboxes, temporal, geoPolygon, config)
    {
    }

    ~HeightSegmentCoordinates()
    {
        delete localIndexes;
        delete leadsIndexes;
    }

    /*
     * get the IndexSelection object for ATL10 height segment rate groups
     *  i.e.) /gt<x>/freeboard_beam_segment/beam_freeboard
     * @param Group root: root group
     * @param Group ingroup: input group
     * @param string shortName: product short name
     * @param SubsetDataLayers subsetDataLayers: dataset name to include in the
     * output
     */
    static Coordinate *getCoordinate(H5::Group &root,
                                     H5::Group &ingroup,
                                     const std::string &shortName,
                                     SubsetDataLayers *subsetDataLayers,
                                     const std::string &groupname,
                                     std::vector<geobox> *geoboxes,
                                     Temporal *temporal,
                                     GeoPolygon *geoPolygon,
                                     Configuration *config)
    {
        LOG_DEBUG("HeightSegmentCoordinates::getCoordinate(): ENTER groupname: "
                  << groupname);

        if (Coordinate::lookUp(groupname))
        {
            LOG_DEBUG(
                "HeightSegmentCoordinates::getCoordinate(): groupname: "
                << groupname
                << " already exists in lookUpMap(HeightSegmentCoordinate)");

            return lookUpMap["HeightSegmentRate"];
        }

        HeightSegmentCoordinates *hgtSegCoor = new HeightSegmentCoordinates(
            groupname, geoboxes, temporal, geoPolygon, config);

        hgtSegCoor->coordinateSize = 0;
        hgtSegCoor->setCoordinateSize(ingroup);
        hgtSegCoor->shortname = shortName;

        // get coordinate reference from in group's coordinates
        // attribute(latitude/longitude and/or delta_time)
        Coordinate *coor = Coordinate::getCoordinate(root,
                                                     ingroup,
                                                     groupname,
                                                     shortName,
                                                     subsetDataLayers,
                                                     geoboxes,
                                                     temporal,
                                                     geoPolygon,
                                                     config);
        if (coor->indexesProcessed)
            hgtSegCoor->localIndexes = coor->indexes;
        else
            hgtSegCoor->localIndexes = coor->getIndexSelection();

        // get forward coordinate reference from leads group
        std::string leadsGroupname =
            config->getLeadsGroup(shortName, groupname);
        Coordinate *leadsCoor;
        hgtSegCoor->leadsGroup = root.openGroup(leadsGroupname);
        if (Coordinate::lookUp(leadsGroupname))
            leadsCoor = lookUpMap[leadsGroupname];
        else
            leadsCoor = ForwardReferenceCoordinates::getCoordinate(
                root,
                hgtSegCoor->leadsGroup,
                shortName,
                subsetDataLayers,
                leadsGroupname,
                geoboxes,
                temporal,
                geoPolygon,
                config);
        if (hgtSegCoor->indexesProcessed)
            hgtSegCoor->leadsIndexes = leadsCoor->indexes;
        else
            hgtSegCoor->leadsIndexes = leadsCoor->getIndexSelection();
        Coordinate::lookUpMap.insert(
            std::make_pair("HeightSegmentRate", hgtSegCoor));

        return hgtSegCoor;
    }

    /*
     * create IndexSelection for height segment rates group(i.e.
     * /gt1l/freeboard_beam_segment/beam_freeboard)
     *      - merge IndexSelection objects: local latitude/longitude/delta_time
     * and leads group
     */
    virtual IndexSelection *getIndexSelection()
    {
        LOG_DEBUG("HeightSegmentCoordinates::getIndexSelection(): ENTER");

        indexes = new IndexSelection(coordinateSize);

        H5::DataSet *indexBegSet = NULL, *countSet = NULL;

        std::string indexBegName, countName;

        config->getDatasetNames(shortname, groupname, indexBegName, countName);

        if (!indexBegName.empty() && H5Lexists(leadsGroup.getLocId(),
                                               indexBegName.c_str(),
                                               H5P_DEFAULT) > 0)
            indexBegSet = new H5::DataSet(leadsGroup.openDataSet(indexBegName));
        if (!countName.empty() &&
            H5Lexists(leadsGroup.getLocId(), countName.c_str(), H5P_DEFAULT) >
                0)
            countSet = new H5::DataSet(leadsGroup.openDataSet(countName));

        H5::DataSet *nonNullDataset =
            (indexBegSet != NULL) ? indexBegSet : countSet;
        size_t coordinateSize =
            nonNullDataset->getSpace().getSimpleExtentNpoints();
        hid_t native_type = H5Tget_native_type(
            H5Dget_type(indexBegSet->getId()), H5T_DIR_ASCEND);

        // index begin datasets for ATL03 and ATL08 are 64-bit and 32-bit for
        // ATL10
        int64_t *indexBeg = new int64_t[coordinateSize];
        int32_t *count = new int32_t[coordinateSize];

        if (H5Tequal(native_type, H5T_NATIVE_LLONG)) // 64-bit int
        {
            indexBegSet->read(indexBeg, indexBegSet->getDataType());
        }
        else if (H5Tequal(native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            int32_t *data = new int32_t[coordinateSize];
            indexBegSet->read(data, indexBegSet->getDataType());
            for (int i = 0; i < coordinateSize; i++)
                indexBeg[i] = data[i];
            delete[] data;
        }

        countSet->read(count, countSet->getDataType());

        // add (start, length) pairs in the local coordinate reference
        for (std::map<long, long>::iterator it = localIndexes->segments.begin();
             it != localIndexes->segments.end();
             it++)
        {
            indexes->addSegment(it->first, it->second);
        }

        // add (start, length) pairs in the leads IndexSelection
        long start, length;
        for (std::map<long, long>::iterator it = leadsIndexes->segments.begin();
             it != leadsIndexes->segments.end();
             it++)
        {
            start = it->first;
            length = it->second;
            for (int i = start; i < length + start; i++)
            {
                indexes->addSegment(indexBeg[i] - 1, count[i]);
            }
        }
        indexesProcessed = true;

        return indexes;
    }

  private:
    IndexSelection *leadsIndexes;
    IndexSelection *localIndexes;
    H5::Group leadsGroup;
    std::string shortname;
};
#endif
