#ifndef SUPERGROUPCOORDINATE_H
#define SUPERGROUPCOORDINATE_H

#include "Configuration.h"
#include "LogLevel.h"
#include <stdlib.h>

#include "H5Cpp.h"

/**
 * class to write index begin datasets for ATL03 and ATL08
 */
class SuperGroupCoordinate : public Coordinate
{
  public:
    SuperGroupCoordinate(std::string groupname,
                         std::vector<geobox> *geoboxes,
                         Temporal *temporal,
                         GeoPolygon *geoPolygon,
                         Configuration *config)
        : Coordinate(groupname, geoboxes, temporal, geoPolygon, config)
    {
    }

    /*
     * subset dataset the "super group"
     * when multiple sets of lat/lon coordinates references exist within one
     * group and these coordinate references all referencing one delta_time
     * datasets, the datasets under that group will be subsetted using the
     * "super group" rule.
     *  - ex.) lat_1, lon_1, lat_2, lon_2, lat_3, and lon_3 exist within one
     * group, and all have reference to delta_time subsetting will be applied by
     * <lat_1,lon_1> or <lat_2,lon_2> or <lat_3,lon_3> and delta_time(if
     * temporal constraint exists)
     * @param Group root: root group
     * @param Group ingroup: input group
     * @param string shortName: product short name
     * @param SubsetDataLayers subsetDataLayers: dataset name to include in the
     * output
     * @param String groupname: group name
     * @param Vector<geobox> geoboxes: bounding boxes(spatial constraints)
     * @param Temporal temporal: temporal constraint
     */
    static Coordinate *getCoordinate(H5::Group &root,
                                     H5::Group &ingroup,
                                     const std::string &shortname,
                                     SubsetDataLayers *subsetDataLayers,
                                     const std::string &groupname,
                                     std::vector<geobox> *geoboxes,
                                     Temporal *temporal,
                                     GeoPolygon *geoPolygon,
                                     Configuration *config)
    {
        LOG_DEBUG("SuperGroupCoordinate::getCoordinate(): ENTER groupname: "
                  << groupname);

        SuperGroupCoordinate *sgCoor = new SuperGroupCoordinate(
            groupname, geoboxes, temporal, geoPolygon, config);
        sgCoor->coordinateSize = 0;

        std::map<std::string, std::vector<std::string>> datasets;
        std::string superGroupname, latitudeName, longitudeName, timeName,
            otherName;
        bool inconsistentCoorDatasets = false;
        H5::DataSet *data = NULL;
        std::vector<std::string> superGroupnames;

        datasets = config->getSuperCoordinates(shortname);
        config->getSuperCoordinateGroupname(
            shortname, groupname, superGroupnames);
        for (std::vector<std::string>::iterator it = superGroupnames.begin();
             it != superGroupnames.end();
             it++)
        {
            H5::Group group = root.openGroup(*it);
            sgCoor->superGroups.push_back(group);
        }
        superGroupname = superGroupnames[0].substr(
            0, superGroupnames[0].find_first_of("/\\", 1) + 1);
        // if the Coordinate group already exists, return it, else, continue
        // processing
        if (sgCoor->lookUp(superGroupname))
        {
            LOG_DEBUG("SuperGroupCoordinate::getCoordinate(): superGroupname: "
                      << superGroupname << " already exists in lookUpMap");
            return lookUpMap[superGroupname];
        }

        // distinguish between latitude, longitude and delta_time datasets
        // while (!datasets.empty())
        for (std::map<std::string, std::vector<std::string>>::iterator it =
                 datasets.begin();
             it != datasets.end();
             it++)
        {
            superGroupnames.push_back(it->first);
            while (!it->second.empty())
            {
                config->getMatchingCoordinateDatasetNames(shortname,
                                                          it->second,
                                                          timeName,
                                                          latitudeName,
                                                          longitudeName,
                                                          otherName);
                if (!latitudeName.empty())
                {
                    sgCoor->latitudes.push_back(latitudeName);
                    it->second.erase(std::remove(it->second.begin(),
                                                 it->second.end(),
                                                 latitudeName),
                                     it->second.end());
                }
                if (!longitudeName.empty())
                {
                    sgCoor->longitudes.push_back(longitudeName);
                    it->second.erase(std::remove(it->second.begin(),
                                                 it->second.end(),
                                                 longitudeName),
                                     it->second.end());
                }
                if (!timeName.empty())
                {
                    it->second.erase(std::remove(it->second.begin(),
                                                 it->second.end(),
                                                 timeName),
                                     it->second.end());
                    for (std::vector<H5::Group>::iterator it =
                             sgCoor->superGroups.begin();
                         it != sgCoor->superGroups.end();
                         it++)
                    {
                        if (!timeName.empty() && H5Lexists(it->getLocId(),
                                                           timeName.c_str(),
                                                           H5P_DEFAULT) > 0)
                        {
                            data = new H5::DataSet(it->openDataSet(timeName));
                            sgCoor->checkCoorDatasetSize(
                                data, inconsistentCoorDatasets);
                            sgCoor->coorDatasets.insert(
                                std::pair<std::string, H5::DataSet *>("time",
                                                                      data));
                        }
                    }
                }
            }
        }

        sort(sgCoor->latitudes.begin(), sgCoor->latitudes.end());
        sort(sgCoor->longitudes.begin(), sgCoor->longitudes.end());

        // get coordinate datasets from input file if they exist
        for (int i = 0; i < sgCoor->latitudes.size(); i++)
        {
            for (std::vector<H5::Group>::iterator it =
                     sgCoor->superGroups.begin();
                 it != sgCoor->superGroups.end();
                 it++)
            {
                if (H5Lexists(it->getLocId(),
                              sgCoor->latitudes[i].c_str(),
                              H5P_DEFAULT) > 0)
                {
                    data =
                        new H5::DataSet(it->openDataSet(sgCoor->latitudes[i]));
                    sgCoor->checkCoorDatasetSize(data,
                                                 inconsistentCoorDatasets);
                    sgCoor->coorDatasets.insert(
                        std::pair<std::string, H5::DataSet *>(
                            sgCoor->latitudes[i], data));
                }
            }
            for (std::vector<H5::Group>::iterator it =
                     sgCoor->superGroups.begin();
                 it != sgCoor->superGroups.end();
                 it++)
            {
                if (H5Lexists(it->getLocId(),
                              sgCoor->longitudes[i].c_str(),
                              H5P_DEFAULT) > 0)
                {
                    data =
                        new H5::DataSet(it->openDataSet(sgCoor->longitudes[i]));
                    sgCoor->checkCoorDatasetSize(data,
                                                 inconsistentCoorDatasets);
                    sgCoor->coorDatasets.insert(
                        std::pair<std::string, H5::DataSet *>(
                            sgCoor->longitudes[i], data));
                }
            }
        }

        // if the number of latitude datasets and the number of longitude
        // datasets don't agree, or their dataset sizes are not the same, return
        // Index Selection with retstriction, start=0, length=0
        if ((sgCoor->latitudes.size() != sgCoor->longitudes.size()) ||
            inconsistentCoorDatasets)
        {
            sgCoor->indexes = new IndexSelection(sgCoor->coordinateSize);
            sgCoor->indexes->addRestriction(0, 0);
            sgCoor->indexesProcessed = true;
            return sgCoor;
        }

        lookUpMap.insert(std::make_pair(superGroupname, sgCoor));

        return sgCoor;
    }

    virtual IndexSelection *getIndexSelection()
    {
        LOG_DEBUG("SuperGroupCoordinate::getIndexSelection(): ENTER");

        H5::DataSet *timeSet = NULL;
        double *time = new double[coordinateSize];

        indexes = new IndexSelection(coordinateSize);
        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL)
            return NULL;

        if (this->coorDatasets.find("time") != this->coorDatasets.end())
            timeSet = this->coorDatasets.find("time")->second;

        // find index ranges
        //  limit the index by temporal constraint
        if (temporal != NULL && timeSet != NULL)
        {
            updateEpochTime(timeSet);
            timeSet->read(time, timeSet->getDataType());
            temporalSubset(time);
        }
        else
            LOG_DEBUG(
                "SuperGroupCoordinate::getIndexSelection(): "
                << "temporal constraint or temporal coordinate not found");

        // read lat/lon datasets if spatial(bbox/polygon) constraints exist
        if ((geoboxes != NULL || geoPolygon != NULL) &&
            this->coorDatasets.size() != 0)
        {
            for (int i = 0; i < this->latitudes.size(); i++)
            {
                double *lat = new double[coordinateSize];
                double *lon = new double[coordinateSize];
                readLatLonDatasets(
                    this->coorDatasets.find(this->latitudes[i])->second,
                    this->coorDatasets.find(this->longitudes[i])->second,
                    lat,
                    lon);
                this->coors.insert(
                    std::pair<std::string, double *>(this->latitudes[i], lat));
                this->coors.insert(
                    std::pair<std::string, double *>(this->longitudes[i], lon));
            }
        }

        // limit the index by spatial constraint
        if (geoboxes != NULL && this->coorDatasets.size() != 0)
            spatialBboxSubset();
        else
            LOG_DEBUG(
                "SuperGroupCoordinate::getIndexSelection(): spatial constraint "
                "or lat/lon coordinates not found");
        // limit the index by polygon
        if (geoPolygon != NULL && this->coorDatasets.size() != 0)
            spatialPolygonSubset();
        else
            LOG_DEBUG(
                "SuperGroupCoordinate::getIndexSelection(): polygon or lat/lon "
                "coordinates not found");

        indexesProcessed = true;

        return indexes;
    }

  private:
    std::vector<H5::Group> superGroups;
    std::vector<std::string> latitudes;
    std::vector<std::string> longitudes;
    std::map<std::string, H5::DataSet *> coorDatasets;
    std::map<std::string, double *> coors;

    // limit the index range by spatial constraints
    void spatialBboxSubset()
    {
        LOG_DEBUG("SuperGroupCoordinate::spatialBboxSubset(): ENTER");

        bool contains = false;

        long indexBegin = indexes->minIndexStart,
             indexEnd = indexes->maxIndexEnd - 1;
        long start = 0, length = 0;
        for (int i = indexBegin; i <= indexEnd; i++)
        {
            std::vector<geobox>::iterator geobox_it = geoboxes->begin();
            for (; geobox_it != geoboxes->end(); geobox_it++)
            {
                for (int j = 0; j < latitudes.size(); j++)
                {
                    if (geobox_it->contains(coors[latitudes[j]][i],
                                            coors[longitudes[j]][i]))
                    {
                        contains = true;
                        break;
                    }
                    else
                        contains = false;
                }
                if (contains)
                {
                    if (length == 0)
                        start = i;
                    length++;
                    break;
                }
            }
            // not covered by the bbox
            if (geobox_it == geoboxes->end())
            {
                if (length != 0)
                {
                    indexes->addSegment(start, length);
                    length = 0;
                }
            }
        }

        // if new index range found, but does not have an end point, add it
        if (length != 0)
            indexes->addSegment(start, length);
        // if no index range found, return no data
        if (indexes->segments.empty())
            indexes->addRestriction(0, 0);
    }

    // limit the index range by polygon
    void spatialPolygonSubset()
    {
        LOG_DEBUG("SuperGroupCoordinate::spatialPolygonSubset(): ENTER");

        IndexSelection *newIndexes = new IndexSelection(coordinateSize);

        geobox g = geoPolygon->getBbox();
        if (geoboxes == NULL)
            geoboxes = new std::vector<geobox>();
        geoboxes->push_back(g);

        LOG_DEBUG(
            "SuperGroupCoordinate::spatialPolygonSubset(): geoboxes.size(): "
            << geoboxes->size());

        spatialBboxSubset();
        LOG_DEBUG(
            "SuperGroupCoordinate::spatialPolygonSubset(): indexes->size(): "
            << indexes->size());

        if (indexes->size() == 0)
            return;

        long start = 0, length = 0;
        bool contains = false;

        for (std::map<long, long>::iterator it = indexes->segments.begin();
             it != indexes->segments.end();
             it++)
        {
            for (int i = it->first; i != it->second + it->first; i++)
            {
                // count in the points with fill values
                for (int j = 0; j < latitudes.size(); j++)
                {
                    if (geoPolygon->contains(coors[latitudes[j]][i],
                                             coors[longitudes[j]][i]))
                    {
                        contains = true;
                        break;
                    }
                    else
                        contains = false;
                }
                if (contains)
                {
                    if (length == 0)
                        start = i;
                    length++;
                }
                else
                {
                    if (length != 0)
                    {
                        newIndexes->addSegment(start, length);
                        length = 0;
                    }
                }
            }
            if (length != 0)
            {
                newIndexes->addSegment(start, length);
                length = 0;
            }
        }

        // if new index range found, but does not have an end point, add it
        if (length != 0)
            newIndexes->addSegment(start, length);
        // if no index range found, return no data
        if (newIndexes->segments.empty())
            newIndexes->addRestriction(0, 0);

        indexes = newIndexes;

        // if new index range found, but does not have an end point, add it
        if (length != 0)
            indexes->addSegment(start, length);
        // if no index range found, return no data
        if (indexes->segments.empty())
            indexes->addRestriction(0, 0);
    }
};
#endif
