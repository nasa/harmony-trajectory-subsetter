#ifndef COORDINATE_H
#define COORDINATE_H

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/unordered_map.hpp>

#include "Configuration.h"
#include "GeoPolygon.h"
#include "H5Cpp.h"
#include "IndexSelection.h"
#include "LogLevel.h"
#include "SubsetDataLayers.h"
#include "Temporal.h"
#include "geobox.h"

// This class captures a reference from one dataset to a set of other coordinate
// datasets.
// Summary of different behaviors for different cases:
//  - test cases when index selection returns everything (at the group level)
//    - no spatial/temporal constraints specified
//    - no coordinate references found
//  - returns everything (at the dataset level)
//    - datasets that have dataset size that is different than the dataset size
//    of  the coordinate datasets(lat/lon/time)
//  - returns nothing(reject the group)
//    - no matching data found within the temporal/spatial bounds specified
//    - coordinate datasets have different datasets size
//    - coordinate reference does not exist
class Coordinate
{
  public:
    // hash map that stores Coordinate instance references for later look up, so
    // that IndexSelection will not be computed multiple times key: coordinate
    // datasets group path; value: Coordinate instance reference
    static boost::unordered_map<std::string, Coordinate *> lookUpMap;

    // IndexSelection instance created based on the coordinate datasets and
    // temporal and/or spatial constraints specified
    IndexSelection *indexes;
    bool indexesProcessed;
    std::string groupname;

    Coordinate(std::string groupname,
               std::vector<geobox> *geoboxes,
               Temporal *temporal,
               GeoPolygon *geoPolygon,
               Configuration *config)
        : groupname(groupname), geoboxes(geoboxes), temporal(temporal),
          geoPolygon(geoPolygon), indexes(NULL), indexesProcessed(false),
          config(config) {};

    ~Coordinate() {}

    // returns the Coordinate instance that is associated with the coordinate
    // attribute value if it does not exist, create it
    static Coordinate *getCoordinate(H5::Group &root,
                                     H5::Group &ingroup,
                                     const std::string &groupname,
                                     const std::string &shortname,
                                     SubsetDataLayers *subsetDataLayers,
                                     std::vector<geobox> *geoboxes,
                                     Temporal *temporal,
                                     GeoPolygon *geoPolygon,
                                     Configuration *config)
    {
        LOG_DEBUG(
            "Coordinate::getCoordinate(): ENTER groupname: " << groupname);

        std::string typeName, objName;
        int numOfObjs, inGroupCount = 0;
        hsize_t arraySize = 0;
        std::string latitudeName, longitudeName, timeName, ignoreName,
            coorGroupname;
        bool inconsistentCoorDatasets = false;
        std::vector<std::string> allDatasets;
        std::string timeNameInGroup, latitudeNameInGroup, longitudeNameInGroup,
            ignoreNameInGroup;

        numOfObjs = ingroup.getNumObjs();
        int i = 0;

        Coordinate *coor =
            new Coordinate(groupname, geoboxes, temporal, geoPolygon, config);
        coor->coordinateSize = 0;

        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL)
        {
            coor->indexesProcessed = true;
            return coor;
        }

        // loop through all datasets in a group
        // to get coordinate datasets from coordinate attributes
        for (i = 0; i < numOfObjs; i++)
        {
            ingroup.getObjTypeByIdx(i, typeName);
            objName = ingroup.getObjnameByIdx(i);
            bool inSubsetDataLayers =
                subsetDataLayers->is_included(groupname + objName + "/");

            if (typeName == "dataset")
            {
                H5::DataSet data = ingroup.openDataSet(objName);
                H5::DataSpace inspace = data.getSpace();
                int dim = inspace.getSimpleExtentNdims();
                hsize_t olddims[dim];
                inspace.getSimpleExtentDims(olddims);
                arraySize = olddims[0];
                allDatasets.push_back(objName);
                coor->getCoordinateDatasetNames(data,
                                                shortname,
                                                latitudeName,
                                                longitudeName,
                                                timeName,
                                                ignoreName,
                                                coorGroupname);
            }
        }
        // match datasets in the group with the coordinate dataset names in the
        // configuration file
        config->getMatchingCoordinateDatasetNames(shortname,
                                                  allDatasets,
                                                  timeNameInGroup,
                                                  latitudeNameInGroup,
                                                  longitudeNameInGroup,
                                                  ignoreNameInGroup);

        // use groupname as coorGroupname(key for lookup map) if the coordinate
        // dataset names are retrieved by walking through the datasets in the
        // group
        if (coorGroupname.empty())
            coorGroupname = groupname;

        // if the Coordinate group already exists, return it, else, continue
        // processing
        if (coor->lookUp(coorGroupname))
        {
            LOG_DEBUG("Coordinate::getCoordinate():"
                      << coorGroupname << " already exists in lookUpMap");
            return lookUpMap[coorGroupname];
        }

        // if lat/lon/time datasets were found in the group and not in
        // coordinates attribute, use lat/lon/time datasets found in the group
        if (timeName.empty() && !timeNameInGroup.empty())
        {
            timeName = timeNameInGroup;
            inGroupCount++;
        }
        if (latitudeName.empty() && !latitudeNameInGroup.empty())
        {
            latitudeName = latitudeNameInGroup;
            inGroupCount++;
        }
        if (longitudeName.empty() && !longitudeNameInGroup.empty())
        {
            longitudeName = longitudeNameInGroup;
            inGroupCount++;
        }

        // if no coordinate references found in the group,
        // return null IndexSelection
        if (latitudeName.empty() && longitudeName.empty() && timeName.empty())
        {
            coor->indexesProcessed = true;
            return coor;
        }

        H5::Group coorGroup = root.openGroup(coorGroupname);

        H5::DataSet *data = NULL;
        if (!latitudeName.empty() &&
            H5Lexists(coorGroup.getLocId(), latitudeName.c_str(), H5P_DEFAULT) >
                0)
        {
            data = new H5::DataSet(coorGroup.openDataSet(latitudeName));
            coor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
            coor->coorDatasets.insert(
                std::pair<std::string, H5::DataSet *>("latitude", data));
        }
        if (!longitudeName.empty() && H5Lexists(coorGroup.getLocId(),
                                                longitudeName.c_str(),
                                                H5P_DEFAULT) > 0)
        {
            data = new H5::DataSet(coorGroup.openDataSet(longitudeName));
            coor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
            coor->coorDatasets.insert(
                std::pair<std::string, H5::DataSet *>("longitude", data));
        }
        if (!timeName.empty() &&
            H5Lexists(coorGroup.getLocId(), timeName.c_str(), H5P_DEFAULT) > 0)
        {
            data = new H5::DataSet(coorGroup.openDataSet(timeName));
            coor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
            coor->coorDatasets.insert(
                std::pair<std::string, H5::DataSet *>("time", data));
        }

        // This is significant when there is a spatial-only subset, but no
        // spatial coordinates exist.
        if (!timeName.empty() && latitudeName.empty() && longitudeName.empty())
        {
            LOG_DEBUG("Group " << groupname << " has temporal coordinates ");
            LOG_DEBUG("and no spatial coordinates.");
            coor->temporalOnlyCoordinates = true;
        }

        // if lat/lon/time datasets do not exist in the group,
        // return IndexSelection with restriction, start=0, length=0
        //    (arraySize used to instantiate IndexSelection is from the datasets
        //    in the group,
        //     since the coordinate datasets do not exist)
        if ((!latitudeName.empty() && H5Lexists(coorGroup.getLocId(),
                                                latitudeName.c_str(),
                                                H5P_DEFAULT) <= 0) ||
            (!longitudeName.empty() && H5Lexists(coorGroup.getLocId(),
                                                 longitudeName.c_str(),
                                                 H5P_DEFAULT) <= 0) ||
            (!timeName.empty() &&
             H5Lexists(coorGroup.getLocId(), timeName.c_str(), H5P_DEFAULT) <=
                 0))
        {
            LOG_DEBUG("Coordinate::getCoordinate(): coordinate dataset(s) does "
                      "not exist");
            coor->indexes = new IndexSelection(arraySize);
            coor->indexes->addRestriction(0, 0);
            coor->indexesProcessed = true;
            return coor;
        }

        // if the coordinate datasets(lat/lon/time) have different dataset size,
        // return Index Selection with retstriction, start=0, length=0
        if (inconsistentCoorDatasets)
        {
            LOG_DEBUG("Coordinate::getCoordinate(): coordinate "
                      "datasets(lat/lon/time) have different dataset size");
            coor->indexes = new IndexSelection(coor->coordinateSize);
            coor->indexes->addRestriction(0, 0);
            coor->indexesProcessed = true;
            return coor;
        }

        // if the coordinate dataset names do not match with the configuration,
        // return IndexSelection with restriction, start=0, length=0
        int count = 0;
        if (!timeName.empty())
            count++;
        if (!longitudeName.empty())
            count++;
        if (!latitudeName.empty())
            count++;
        if (!ignoreName.empty())
            count++;

        if (count != (coor->datasetNames.size() + inGroupCount))
        {
            LOG_DEBUG(
                "Coordinate::getCoordinate(): Invalid coordinate reference");
            coor->indexes = new IndexSelection(coor->coordinateSize);
            coor->indexes->addRestriction(0, 0);
            coor->indexesProcessed = true;
            return coor;
        }

        lookUpMap.insert(std::make_pair(coorGroupname, coor));

        return coor;
    }

    // check if the Coordinate object already exists
    static bool lookUp(std::string coorGroupname)
    {
        if (lookUpMap.find(coorGroupname) == lookUpMap.end())
            return false;
        else
            return true;
    }

    /**
     * read lat/lon values from DataSet object to array
     * @param latSet latitude DataSet
     * @param lonSet longitude DataSet
     * @param lat latitude array
     * @param lon longitude array
     */
    void readLatLonDatasets(H5::DataSet *latSet,
                            H5::DataSet *lonSet,
                            double *lat,
                            double *lon)
    {
        LOG_DEBUG("Coordinate::readLatLonDatasets(): ENTER");

        if (latSet->getDataType().getSize() == 8) // double
        {
            latSet->read(lat, latSet->getDataType());
        }
        else
        {
            float *data = new float[coordinateSize];
            latSet->read(data, latSet->getDataType());
            for (int i = 0; i < coordinateSize; i++)
                lat[i] = data[i];
            delete[] data;
        }

        if (lonSet->getDataType().getSize() == 8) // double
        {
            lonSet->read(lon, lonSet->getDataType());
        }
        else
        {
            float *data = new float[coordinateSize];
            lonSet->read(data, lonSet->getDataType());
            for (int i = 0; i < coordinateSize; i++)
                lon[i] = data[i];
            delete[] data;
        }
    }

    // return IndexSelection instance, if it exists
    // if it does not exist, create one
    virtual IndexSelection *getIndexSelection()
    {
        LOG_DEBUG("Coordinate::getIndexSelection(): ENTER");

        H5::DataSet *latSet = NULL, *lonSet = NULL, *timeSet = NULL;
        double *lat = new double[coordinateSize];
        double *lon = new double[coordinateSize];
        double *time = new double[coordinateSize];

        indexes = new IndexSelection(coordinateSize);

        // if both spatial and temporal constraints don't exist
        // or no coordinate datasets, return null
        if ((geoboxes == NULL && temporal == NULL && geoPolygon == NULL) ||
            (this->coorDatasets.empty()))
        {
            indexesProcessed = true;
            return NULL;
        }

        if (this->coorDatasets.find("latitude") != this->coorDatasets.end())
            latSet = this->coorDatasets.find("latitude")->second;
        if (this->coorDatasets.find("longitude") != this->coorDatasets.end())
            lonSet = this->coorDatasets.find("longitude")->second;
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
            LOG_DEBUG("Coordinate::getIndexSelection(): temporal constraint or "
                      "temporal coordinate not found");

        // read lat/lon datasets if spatial(bbox/polygon) constraints exist
        if ((geoboxes != NULL || geoPolygon != NULL) &&
            (latSet != NULL && lonSet != NULL))
            readLatLonDatasets(latSet, lonSet, lat, lon);

        // limit the index by spatial constraint
        if (geoboxes != NULL && latSet != NULL && lonSet != NULL)
            spatialBboxSubset(lat, lon);
        else
            LOG_DEBUG("Coordinate::getIndexSelection(): spatial constraint or "
                      "lat/lon coordinates not found");
        // limit the index by polygon
        if (geoPolygon != NULL && latSet != NULL && lonSet != NULL)
            spatialPolygonSubset(lat, lon);
        else
            LOG_DEBUG("Coordinate::getIndexSelection(): polygon or lat/lon "
                      "coordinates not found");

        indexesProcessed = true;

        delete[] lat;
        delete[] lon;

        return indexes;
    }

    bool hasTemporalOnlyCoordinates() { return temporalOnlyCoordinates; }
    void setTemporalOnlyCoordinates(const bool temporalOnly)
    {
        temporalOnlyCoordinates = temporalOnly;
    }

  protected:
    // temporal and spatial constraints
    std::vector<geobox> *geoboxes;
    Temporal *temporal;
    GeoPolygon *geoPolygon;

    Configuration *config;

    std::vector<std::string> datasetNames;

    hsize_t coordinateSize;

    virtual std::vector<std::string> *getCoordinateDatasetNames()
    {
        return NULL;
    }

    virtual void getCoordinateByGroup(H5::Group ingroup)
    {
        LOG_DEBUG("Coordinate::getIndexSelection(): ENTER");

        std::vector<std::string> *coordinateSets = getCoordinateDatasetNames();
        H5::DataSet *latitudeSet = NULL, *longitudeSet = NULL, *timeSet = NULL;
        if (coordinateSets == NULL || coordinateSets->empty())
            return;
        H5::Group group = ingroup.openGroup(groupname);
        if (!coordinateSets->at(0).empty() &&
            H5Lexists(group.getLocId(),
                      coordinateSets->at(0).c_str(),
                      H5P_DEFAULT) > 0)
        {
            latitudeSet =
                new H5::DataSet(group.openDataSet(coordinateSets->at(0)));
        }
        if (!coordinateSets->at(1).empty() &&
            H5Lexists(group.getLocId(),
                      coordinateSets->at(1).c_str(),
                      H5P_DEFAULT) > 0)
        {
            longitudeSet =
                new H5::DataSet(group.openDataSet(coordinateSets->at(1)));
        }
        if (!coordinateSets->at(2).empty() &&
            H5Lexists(group.getLocId(),
                      coordinateSets->at(2).c_str(),
                      H5P_DEFAULT) > 0)
        {
            timeSet = new H5::DataSet(group.openDataSet(coordinateSets->at(2)));
        }
        group.close();
        delete coordinateSets;
    }

    // sets the maximum coordinate size for the output for photon groups
    void setCoordinateSize(H5::Group &group)
    {
        LOG_DEBUG("Coordinate::setCoordinateSize(): ENTER");

        std::string objName, typeName;
        hsize_t arraySize = 0;

        for (int i = 0; i < group.getNumObjs(); i++)
        {
            objName = group.getObjnameByIdx(i);
            group.getObjTypeByIdx(i, typeName);

            if (typeName == "dataset")
            {
                H5::DataSet data = group.openDataSet(objName);
                H5::DataSpace inspace = data.getSpace();
                int dim = inspace.getSimpleExtentNdims();
                hsize_t olddims[dim];
                inspace.getSimpleExtentDims(olddims);
                arraySize = olddims[0];
                if (coordinateSize == 0)
                    coordinateSize = arraySize;
            }
        }
    }

    void temporalSubset(double *time)
    {
        LOG_DEBUG("Coordinate::temporalSubset(): ENTER");

        long start = 0, end = 0, length = 0;

        // if data is within the temporal constraint range
        if ((time[0] <= temporal->getEnd()) &&
            (time[coordinateSize - 1] >= temporal->getStart()))
        {
            bool matchFound = false;
            // find the starting point
            for (int i = 0; i < coordinateSize; i++)
            {
                if (temporal->contains(time[i]))
                {
                    start = i;
                    matchFound = true;
                    break;
                }
            }

            // find the ending point
            for (int i = coordinateSize - 1; i >= 0; i--)
            {
                if (temporal->contains(time[i]))
                {
                    end = i;
                    break;
                }
            }
            if (matchFound)
                length = end - start + 1;
        }
        indexes->addRestriction(start, length);
        delete[] time;
    }

    void updateEpochTime(H5::DataSet *time)
    {
        LOG_DEBUG("Coordinate::updateEpochTime(): ENTER");

        H5::Attribute attr;
        std::string attrName, attrValue;
        std::string timeStr = "T00:00:00";
        std::string epoch;

        for (int i = 0; i < time->getNumAttrs(); i++)
        {
            attr = time->openAttribute(i);
            attrName = attr.getName();

            // get "units" attribute
            // parse the "units" attribute value to get the epoch
            // ex.) units = seconds since 2018-01-01, parse out "2018-01-01"
            // add "T00:00:00" to the date
            if (attrName == "units")
            {
                attr.read(attr.getDataType(), attrValue);
                boost::smatch match;
                // parse out date from the units attribute if exists
                if (regex_search(attrValue,
                                 match,
                                 boost::regex("\\d{4}[-]\\d{2}[-]\\d{2}")))
                {
                    epoch = match[0] + timeStr;
                    // if the epoch is different from the product epoch in
                    // configuration file or default epoch, update it
                    if (temporal->needToUpdateEpoch(epoch))
                    {
                        temporal->updateReferenceTime(epoch);
                    }
                }
            }
        }
    }

    // check if all the coordinate datasets(lat/lon/time) have same coordinate
    // size
    void checkCoorDatasetSize(H5::DataSet *data, bool &inconsistentCoorDatasets)
    {
        H5::DataSpace inspace = data->getSpace();
        int dim = inspace.getSimpleExtentNdims();
        hsize_t olddims[dim];
        inspace.getSimpleExtentDims(olddims);
        if (coordinateSize == 0)
            coordinateSize = olddims[0];
        if ((coordinateSize != olddims[0]) && (!inconsistentCoorDatasets))
            inconsistentCoorDatasets = true;
    }

  private:
    // limit the index range by spatial constraints
    // lat/lon datasets for SMAP are 32-bit floating-point and 64-bit for ICESat
    void spatialBboxSubset(double *lat, double *lon)
    {
        LOG_DEBUG("Coordinate::spatialBboxSubset(): ENTER");

        long indexBegin = indexes->minIndexStart,
             indexEnd = indexes->maxIndexEnd - 1;
        long start = 0, length = 0;
        for (int i = indexBegin; i <= indexEnd; i++)
        {
            std::vector<geobox>::iterator geobox_it = geoboxes->begin();

            // count in the points with fill values
            if (lat[i] > 90 || lat[i] < -90 || lon[i] > 180 || lon[i] < -180)
            {

                if (length != 0)
                    length++;
            }
            // check if every point is within the spatial constraint
            else
            {
                for (; geobox_it != geoboxes->end(); geobox_it++)
                {
                    if (geobox_it->contains(lat[i], lon[i]))
                    {
                        // new index range found
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
        }

        // if new index range found, but does not have an end point, add it
        if (length != 0)
            indexes->addSegment(start, length);
        // if no index range found, return no data
        if (indexes->segments.empty())
            indexes->addRestriction(0, 0);
    }

    // limit the index range by polygon
    void spatialPolygonSubset(double *lat, double *lon)
    {
        LOG_DEBUG("Coordinate::spatialPolygonSubset(): ENTER");

        IndexSelection *newIndexes = new IndexSelection(coordinateSize);

        geobox g = geoPolygon->getBbox();
        if (geoboxes == NULL)
            geoboxes = new std::vector<geobox>();
        geoboxes->push_back(g);

        spatialBboxSubset(lat, lon);
        LOG_DEBUG("Coordinate::spatialPolygonSubset(): indexes->size(): "
                  << indexes->size());

        if (indexes->size() == 0)
            return;

        long start = 0, length = 0;

        for (std::map<long, long>::iterator it = indexes->segments.begin();
             it != indexes->segments.end();
             it++)
        {
            for (int i = it->first; i != it->second + it->first; i++)
            {
                // count in the points with fill values
                if (lat[i] > 90 || lat[i] < -90 || lon[i] > 180 ||
                    lon[i] < -180)
                {

                    if (length != 0)
                        length++;
                }
                // check if every point is within the polygon
                else
                {
                    if (geoPolygon->contains(lat[i], lon[i]))
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
    }

    // get coordinate dataset names from the "coordinates" attribute
    // and return false if the datasets are not consistent with any of the
    // previously defined datasets
    void getCoordinateDatasetNames(H5::DataSet dataset,
                                   std::string shortname,
                                   std::string &latitudeName,
                                   std::string &longitudeName,
                                   std::string &timeName,
                                   std::string &ignoreName,
                                   std::string &coorGroupname)
    {
        H5::Attribute attr;
        std::string attrName, attrValue, attrDataset, absPath;
        boost::char_separator<char> delim(" ,");

        // loop through all attributes for a dataset
        for (int i = 0; i < dataset.getNumAttrs(); i++)
        {
            attr = dataset.openAttribute(i);
            attrName = attr.getName();
            // when "coordinates" dataset is found, parse the attribute value,
            // and put it into the vector
            if (attrName == "coordinates")
            {
                attr.read(attr.getDataType(), attrValue);
                boost::tokenizer<boost::char_separator<char>> datasets(
                    attrValue, delim);
                std::vector<std::string>::iterator it = datasetNames.begin();
                BOOST_FOREACH (attrDataset, datasets)
                {
                    std::string dots = "..";
                    // if it starts with '/', it is the absolute path
                    if (attrDataset.find_first_of("/\\") == 0)
                    {
                        absPath = attrDataset;
                    }
                    // if the coordinate path does not have "..", the coordinate
                    // references is in the same group
                    else if (attrDataset.find(dots) == std::string::npos)
                    {
                        absPath = groupname + attrDataset;
                    }
                    // else, if it does contain "..", need to convert it to the
                    // correct path
                    else
                    {
                        absPath = groupname + attrDataset;
                        std::string pathDelim("/");
                        std::vector<std::string> path;
                        split(path, absPath, boost::is_any_of(pathDelim));
                        std::vector<std::string>::iterator it =
                            find(path.begin(), path.end(), dots);
                        while (it != path.end())
                        {
                            path.erase(it - 1, it + 1);
                            it = find(path.begin(), path.end(), dots);
                        }
                        absPath = boost::join(path, pathDelim);
                    }
                    attrDataset =
                        attrDataset.substr(attrDataset.find_last_of("/\\") + 1);
                    coorGroupname =
                        absPath.substr(0, absPath.find_last_of("/\\") + 1);
                    if (std::find(datasetNames.begin(),
                                  datasetNames.end(),
                                  attrDataset) == datasetNames.end())
                    {
                        LOG_DEBUG(
                            "Coordinate::getCoordinateDatasetNames(): adding "
                            << attrDataset);
                        datasetNames.push_back(attrDataset);

                        // get matching coordinate dataset names with the
                        // configuration file
                        config->getMatchingCoordinateDatasetNames(shortname,
                                                                  datasetNames,
                                                                  timeName,
                                                                  latitudeName,
                                                                  longitudeName,
                                                                  ignoreName);
                    }
                }
            }
        }
    }

    // map that captures coordinate datasets, keys are coordinate dataset
    // names(string) values are acutal coordinate datasets e.g.
    // key:/profile_1/delta_time, value: /profile_1/delta_time dataset object
    std::map<std::string, H5::DataSet *> coorDatasets;

    bool temporalOnlyCoordinates = false;
};
boost::unordered_map<std::string, Coordinate *> Coordinate::lookUpMap;
#endif
