#ifndef COORDINATE_H
#define COORDINATE_H

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>

#include "H5Cpp.h"
#include "IndexSelection.h"
#include "geobox.h"
#include "Temporal.h"
#include "SubsetDataLayers.h"
#include "GeoPolygon.h"

using namespace std;
using namespace H5;
using namespace boost;

// This class captures a reference from one dataset to a set of other coordinate
// datasets.
// Summary of different behavoirs for different cases:
//  - test cases when index selection returns everything (at the group level)
//    - no spatial/temporal constraints specified
//    - no coordinate references found
//  - returns everything (at the dataset level)
//    - datasets that have dataset size that is different than the dataset size of  the coordinate datasets(lat/lon/time)
//  - returns nothing(reject the group)
//    - no matching data found within the temporal/spatial bounds specified
//    - coordinate datasets have different datasets size
//    - coordinate reference does not exist
class Coordinate
{
public:
    
    // hash map that stores Coordinate instance references for later look up, so that
    // IndexSelection will not be computed multiple times
    // key: coordinate datasets group path; value: Coordinate instance reference
    static boost::unordered_map<string, Coordinate*> lookUpMap;
    
    // IndexSelection instance created based on the coordinate datasets and temporal
    // and/or spatial constraints specified
    IndexSelection* indexes;
    
    bool indexesProcessed;
    
    string groupname;
        
    Coordinate(string groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    : groupname(groupname),geoboxes(geoboxes),temporal(temporal),geoPolygon(geoPolygon),indexes(NULL), indexesProcessed(false)
    {
    };
    
    ~Coordinate()
    {
    }
    
    // returns the Coordinate instance that is associated with the coordinate attribute value
    // if it does not exist, create it
    static Coordinate* getCoordinate(Group& root, Group& ingroup, const string& groupname, const string& shortname, 
        SubsetDataLayers* subsetDataLayers, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    {
        string typeName, objName;
        int numOfObjs, inGroupCount=0;
        hsize_t arraySize = 0;
        string latitudeName, longitudeName, timeName, ignoreName, coorGroupname;
        bool inconsistentCoorDatasets = false;
        vector<string> allDatasets;
        string timeNameInGroup, latitudeNameInGroup, longitudeNameInGroup, ignoreNameInGroup;
                        
        numOfObjs = ingroup.getNumObjs();
        int i = 0;
        //coordinateSize = 0;
        
        Coordinate* coor = new Coordinate(groupname, geoboxes, temporal, geoPolygon);
        coor->coordinateSize = 0;
        
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL) 
        {
            coor->indexesProcessed = true;
            return coor;
        }
        
        //cout << "groupname: " << groupname << endl;
        
        // loop through all datasets in a group
        // to get coordinate datasets from coordinate attributes
        for (i = 0; i < numOfObjs; i++)
        {
            ingroup.getObjTypeByIdx(i, typeName);
            objName = ingroup.getObjnameByIdx(i);
            bool inSubsetDataLayers = subsetDataLayers->is_included(groupname+objName+"/");
            
            if (typeName == "dataset")
            {
                //cout << objName << endl;
                DataSet data = ingroup.openDataSet(objName);
                DataSpace inspace = data.getSpace();
                int dim = inspace.getSimpleExtentNdims();
                hsize_t olddims[dim];
                inspace.getSimpleExtentDims(olddims);
                arraySize = olddims[0];
                allDatasets.push_back(objName);
                coor->getCoordinateDatasetNames(data, shortname, latitudeName, longitudeName, timeName, ignoreName, coorGroupname);
            }
        }
        // match datasets in the group with the coordinate dataset names in the configuration file
        Configuration::getInstance()->getMatchingCoordinateDatasetNames(shortname, allDatasets, timeNameInGroup, latitudeNameInGroup, longitudeNameInGroup,
                                                                        ignoreNameInGroup);
        // use groupname as coorGroupname(key for lookup map) if the coordinate dataset names are 
        // retrieved by walking through the datasets in the group
        if (coorGroupname.empty()) coorGroupname = groupname;
        
        // if the Coordinate group already exists, return it, else, continue processing
        if (coor->lookUp(coorGroupname))
        {
            cout << coorGroupname << " already exists in lookUpMap" << endl;
            return lookUpMap[coorGroupname];
        }
        
        // if lat/lon/time datasets were found in the group and not in coordinates attribute,
        // use lat/lon/time datasets found in the group
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
        
        Group coorGroup = root.openGroup(coorGroupname);
        
        DataSet *data = NULL;
        if (!latitudeName.empty() && H5Lexists(coorGroup.getLocId(), latitudeName.c_str(), H5P_DEFAULT) > 0)
        {
            //cout << "latitudeName: " << latitudeName << endl;
            data = new DataSet(coorGroup.openDataSet(latitudeName));
            coor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
            coor->coorDatasets.insert(std::pair<string, DataSet*>("latitude", data));
        }
        if (!longitudeName.empty() && H5Lexists(coorGroup.getLocId(), longitudeName.c_str(), H5P_DEFAULT) > 0)
        {
            //cout << "longitudeName: " << longitudeName << endl;
            data = new DataSet(coorGroup.openDataSet(longitudeName));
            coor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
            coor->coorDatasets.insert(std::pair<string, DataSet*>("longitude", data));
        }
        if (!timeName.empty() && H5Lexists(coorGroup.getLocId(), timeName.c_str(), H5P_DEFAULT) > 0)
        {
            //cout << "timeName: " << timeName << endl;
            data = new DataSet(coorGroup.openDataSet(timeName));
            coor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
            coor->coorDatasets.insert(std::pair<string, DataSet*>("time", data));
        }
        
        // if lat/lon/time datasets do not exist in the group, 
        // return IndexSelection with restriction, start=0, length=0
        //    (arraySize used to instantiate IndexSelection is from the datasets in the group,
        //     since the coordinate datasets do not exist)
        if ((!latitudeName.empty() && H5Lexists(coorGroup.getLocId(), latitudeName.c_str(), H5P_DEFAULT) <= 0) ||
            (!longitudeName.empty() && H5Lexists(coorGroup.getLocId(), longitudeName.c_str(), H5P_DEFAULT) <= 0) ||
            (!timeName.empty() && H5Lexists(coorGroup.getLocId(), timeName.c_str(), H5P_DEFAULT) <= 0))
        {
            cout << "coordinate dataset(s) does not exist" << endl;
            coor->indexes = new IndexSelection(arraySize);
            coor->indexes->addRestriction(0,0);
            coor->indexesProcessed = true;
            return coor;
        }
        
        // if the coordinate datasets(lat/lon/time) have different dataset size,
        // return Index Selection with retstriction, start=0, length=0
        if (inconsistentCoorDatasets)
        {
            coor->indexes = new IndexSelection(coor->coordinateSize);
            coor->indexes->addRestriction(0,0);
            coor->indexesProcessed = true;
            return coor;
        }
        
        // if the coordinate dataset names do not match with the configuration,
        // return IndexSelection with restriction, start=0, length=0
        int count = 0;
        if (!timeName.empty()) count++;
        if (!longitudeName.empty()) count++;
        if (!latitudeName.empty()) count++;
        if (!ignoreName.empty()) count++;
        
        if (count != (coor->datasetNames.size() + inGroupCount))
        {
            cout << "invalid coordinate reference" << endl;
            coor->indexes = new IndexSelection(coor->coordinateSize);
            coor->indexes->addRestriction(0,0);
            coor->indexesProcessed = true;
            return coor;
        }
        
        lookUpMap.insert(std::make_pair(coorGroupname, coor));

        return coor;
    }
    
    // check if the Coordinate object already exists
    static bool lookUp(string coorGroupname)
    {
        if (lookUpMap.find(coorGroupname) == lookUpMap.end()) return false;
        else return true;
    }
    
    /**
     * read lat/lon values from DataSet object to array
     * @param latSet latitude DataSet
     * @param lonSet longitude DataSet
     * @param lat latitude array
     * @param lon longitude array
     */
    void readLatLonDatasets(DataSet* latSet, DataSet* lonSet, double* lat, double* lon)
    {
        if (latSet->getDataType().getSize() == 8) // double
        {
            latSet->read(lat, latSet->getDataType());
        }
        else
        {
            float* data = new float[coordinateSize];
            latSet->read(data, latSet->getDataType());
            for (int i = 0; i < coordinateSize; i++) lat[i] = data[i];
            delete [] data;
        }
        
        if (lonSet->getDataType().getSize() == 8) //double
        {
            lonSet->read(lon, lonSet->getDataType());
        }
        else
        {
            float* data = new float[coordinateSize];
            lonSet->read(data, lonSet->getDataType());
            for (int i = 0; i < coordinateSize; i++) lon[i] = data[i];
            delete [] data;
        }
    }
    
    // return IndexSelection instance, if it exists
    // if it does not exist, create one
    virtual IndexSelection* getIndexSelection()
    {
        cout << "getIndexSelection" << endl;
        DataSet *latSet = NULL, *lonSet=NULL, *timeSet=NULL;
        double* lat = new double[coordinateSize];
        double* lon = new double[coordinateSize];
        double* time = new double[coordinateSize];
        
        indexes = new IndexSelection(coordinateSize);
        
        // if both spatial and temporal constraints don't exist 
        // or no coordinate datasets, return null
        if ((geoboxes == NULL && temporal == NULL && geoPolygon == NULL) || (this->coorDatasets.empty()))
        {
            indexesProcessed = true;
            return NULL;
        }
        
        if (this->coorDatasets.find("latitude") != this->coorDatasets.end()) latSet = this->coorDatasets.find("latitude")->second;
        if (this->coorDatasets.find("longitude") != this->coorDatasets.end()) lonSet = this->coorDatasets.find("longitude")->second;
        if (this->coorDatasets.find("time") != this->coorDatasets.end()) timeSet = this->coorDatasets.find("time")->second;
                
        //find index ranges
        // limit the index by temporal constraint
        if (temporal != NULL && timeSet != NULL)
        {
            updateEpochTime(timeSet);
            timeSet->read(time, timeSet->getDataType());
            temporalSubset(time);
        }
        else cout << "temporal constraint or temporal coordinate not found" << endl;
        
        // read lat/lon datasets if spatial(bbox/polygon) constraints exist
        if ((geoboxes != NULL || geoPolygon != NULL) && (latSet != NULL && lonSet !=NULL)) readLatLonDatasets(latSet, lonSet, lat, lon);
        
        // limit the index by spatial constraint
        if (geoboxes != NULL && latSet != NULL && lonSet !=NULL) spatialBboxSubset(lat, lon); 
        else cout << "spatial constraint or lat/lon coordinates not found" << endl;
        // limit the index by polygon
        if (geoPolygon != NULL && latSet != NULL && lonSet != NULL) spatialPolygonSubset(lat, lon);
        else cout << "polygon or lat/lon coordinates not found" << endl;
        
        indexesProcessed = true;
        
        delete lat;
        delete lon;
        
        return indexes;
    }
    
protected:
    
    // temporal and spatial constraints 
    vector<geobox>* geoboxes;
    Temporal* temporal; 
    GeoPolygon* geoPolygon;
    
    //string groupname;
    vector<string> datasetNames;
        
    hsize_t coordinateSize;
    
    virtual vector<string>* getCoordinateDatasetNames(){return NULL;}
    
    virtual void getCoordinateByGroup(Group ingroup)
    {
        cout << "retrieveCoordinateDatasets " << groupname << endl;
        vector<string>* coordinateSets = getCoordinateDatasetNames();
        DataSet *latitudeSet = NULL, *longitudeSet = NULL, *timeSet = NULL;
        if (coordinateSets == NULL || coordinateSets->empty())
            return;
        Group group = ingroup.openGroup(groupname);
        if (!coordinateSets->at(0).empty() && H5Lexists(group.getLocId(), coordinateSets->at(0).c_str(), H5P_DEFAULT) > 0)
        {
            latitudeSet = new DataSet(group.openDataSet(coordinateSets->at(0)));
        }
        if (!coordinateSets->at(1).empty() && H5Lexists(group.getLocId(), coordinateSets->at(1).c_str(), H5P_DEFAULT) > 0)
        {
            longitudeSet = new DataSet(group.openDataSet(coordinateSets->at(1)));
        }
        if (!coordinateSets->at(2).empty() && H5Lexists(group.getLocId(), coordinateSets->at(2).c_str(), H5P_DEFAULT) > 0)
        {
            timeSet = new DataSet(group.openDataSet(coordinateSets->at(2)));                
        }        
        group.close();        
        delete coordinateSets;
    }
    
    // sets the maximum coordinate size for the output for photon groups
    void setCoordinateSize(Group& group)
    {
        string objName, typeName;
        hsize_t arraySize = 0;
        
        for (int i = 0; i < group.getNumObjs(); i++)
        {
            objName = group.getObjnameByIdx(i);
            group.getObjTypeByIdx(i, typeName);
            
            if (typeName == "dataset")
            {
                DataSet data = group.openDataSet(objName);
                DataSpace inspace = data.getSpace();
                int dim = inspace.getSimpleExtentNdims();
                hsize_t olddims[dim];
                inspace.getSimpleExtentDims(olddims);
                arraySize  = olddims[0];
                if (coordinateSize == 0) coordinateSize = arraySize;
            }
        }
    }
    
    void temporalSubset(double* time)
    {
        cout << "temporalSubset" << endl;
        long start = 0, end = 0, length = 0;
                
        // if data is within the temporal constraint range
        if ((time[0] <= temporal->getEnd()) && (time[coordinateSize-1] >= temporal->getStart()))
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
            for (int i = coordinateSize-1; i >=0; i--)
            {
                if (temporal->contains(time[i]))
                {
                    end = i;
                    break;
                }
            }
            if (matchFound) length = end - start + 1;
        }
        indexes->addRestriction(start, length);
        delete [] time;
    }
    
    void updateEpochTime(DataSet* time)
    {
        //cout << "updateEpochTime" << endl;
        
        Attribute attr;
        string attrName, attrValue;
        string timeStr = "T00:00:00";
        string epoch;
        
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
                if (regex_search(attrValue, match, regex("\\d{4}[-]\\d{2}[-]\\d{2}")))
                {
                    epoch = match[0] + timeStr;
                    // if the epoch is different from the product epoch in configuration file or default epoch,
                    // update it
                    if (temporal->needToUpdateEpoch(epoch))
                    {
                        temporal->updateReferenceTime(epoch);
                    }
                }
            }
        }
        
    }
    
    // check if all the coordinate datasets(lat/lon/time) have same coordinate size
    void checkCoorDatasetSize(DataSet* data, bool& inconsistentCoorDatasets)
    {
        DataSpace inspace = data->getSpace();
        int dim = inspace.getSimpleExtentNdims();
        hsize_t olddims[dim];
        inspace.getSimpleExtentDims(olddims);
        if (coordinateSize == 0) coordinateSize = olddims[0];
        if ((coordinateSize != olddims[0]) && (!inconsistentCoorDatasets)) inconsistentCoorDatasets = true;                
    }
private:
    
    // limit the index range by spatial constraints
    // lat/lon datasets for SMAP are 32-bit floating-point and 64-bit for ICESat
    void spatialBboxSubset(double* lat, double* lon)
    {        
        cout << "spatialBboxSubset" << endl;
        //double* lat = new double[coordinateSize];
        //double* lon = new double[coordinateSize];
        
        //readLatLonDatasets(latSet, lonSet, lat, lon);
        
        long indexBegin = indexes->minIndexStart, indexEnd = indexes->maxIndexEnd - 1;
        long start = 0, length = 0;
        for (int i=indexBegin; i<=indexEnd; i++)
        {
            vector<geobox>::iterator geobox_it = geoboxes->begin();
            
            // count in the points with fill values
            if (lat[i] > 90 || lat[i] < -90 || lon[i] > 180 || lon[i] < -180)
            {
                //cout << "***** found fill value in coordinate datasets lat lon: "
                 //        << i << " " << lat[i] << " " << lon[i] << endl;
                if (length != 0) length++;
            }
            // check if every point is within the spatial constraint
            else
            {
                for (; geobox_it != geoboxes->end(); geobox_it++)
                {
                    if (geobox_it->contains(lat[i], lon[i]))
                    {
                        //cout << "found " << lat[i] << ", " << lon[i] << " inside polygon" << endl;
                        // new index range found
                        if (length == 0) start = i; 
                        length++;
                        break;
                    }
                }
                // not covered by the bbox
                if (geobox_it == geoboxes->end())
                {
                    //cout << "latitude " << lat[i] << " and longitude " << lon[i] << " was not in the polygon" << endl;
                    if (length != 0)
                    {
                        indexes->addSegment(start, length);
                        length = 0;
                    }
                }
            }
        }
        
        // if new index range found, but does not have an end point, add it
        if (length != 0) indexes->addSegment(start, length);
        // if no index range found, return no data
        if (indexes->segments.empty()) indexes->addRestriction(0, 0);
    }
    
    // limit the index range by polygon
    void spatialPolygonSubset(double* lat, double* lon)
    {
        cout << "spatialPolygonSubset" << endl;
        IndexSelection* newIndexes = new IndexSelection(coordinateSize);
        
        geobox g = geoPolygon->getBbox();
        if (geoboxes == NULL) geoboxes = new vector<geobox>();
        geoboxes->push_back(g);
        
        spatialBboxSubset(lat, lon);
        cout << "indexes->size(): " << indexes->size() << endl;
        
        if (indexes->size() == 0) return;
        //if (indexes->size() == 0 || geoPolygon->isBbox()) return;
        
        long start = 0, length = 0;
        
        //if (geoPolygon->crossedEast) cout << "crossedEast" << endl;
        //else if (geoPolygon->crossedWest) cout << "crossedWest" << endl;
        
        for (map<long, long>::iterator it = indexes->segments.begin(); it != indexes->segments.end(); it++)
        {
            for (int i = it->first; i != it->second+it->first; i++)
            {                
                // count in the points with fill values
                if (lat[i] > 90 || lat[i] < -90 || lon[i] > 180 || lon[i] < -180)
                {
                    //cout << "***** found fill value in coordinate datasets lat lon: "
                    //         << i << " " << lat[i] << " " << lon[i]<< endl;
                    if (length != 0) length++;
                }
                // check if every point is within the polygon
                else
                {
                    if (geoPolygon->contains(lat[i], lon[i]))
                    {
                        //cout << "point: (" << lat[i] << "," << lon[i] << ") is within the polygon" << endl;
                        if(length == 0) start = i;
                        length++;
                    }
                    else
                    {
                        //cout << "point: (" << lat[i] << "," << lon[i] << ") is not within the polygon" << endl;
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
        if (length != 0) newIndexes->addSegment(start, length);
        // if no index range found, return no data
        if (newIndexes->segments.empty()) newIndexes->addRestriction(0, 0);
        
        indexes = newIndexes;
    }

    // get coordinate dataset names from the "coordinates" attribute
    // and return false if the datasets are not consistent with any of the previously
    // defined datasets
    void getCoordinateDatasetNames(DataSet dataset, string shortname, string &latitudeName, string &longitudeName, string &timeName, 
                                   string& ignoreName, string &coorGroupname)
    {
        Attribute attr;
        string attrName, attrValue, attrDataset, absPath;
        char_separator<char> delim(" ,");
                
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
                tokenizer<char_separator<char> > datasets(attrValue, delim);
                vector<string>::iterator it = datasetNames.begin();
                BOOST_FOREACH(attrDataset, datasets)
                {
                    string dots = "..";
                    // if it starts with '/', it is the absolute path
                    if (attrDataset.find_first_of("/\\") == 0)
                    {
                        absPath = attrDataset;
                    }
                    // if the coordinate path does not have "..", the coordinate references
                    // is in the same group
                    else if (attrDataset.find(dots) == string::npos)
                    {
                        absPath = groupname + attrDataset;
                    }
                    // else, if it does contain "..", need to convert it to the correct path
                    else
                    {
                        absPath = groupname+attrDataset;
                        string pathDelim("/");
                        vector<string> path;
                        split(path, absPath, is_any_of(pathDelim));
                        vector<string>::iterator it = find(path.begin(), path.end(), dots);
                        while (it != path.end())
                        {
                            path.erase(it-1, it+1);
                            it = find(path.begin(), path.end(), dots);
                        }
                        absPath = join(path, pathDelim);
                    }
                    attrDataset = attrDataset.substr(attrDataset.find_last_of("/\\")+1);
                    coorGroupname = absPath.substr(0, absPath.find_last_of("/\\")+1);
                    if (std::find(datasetNames.begin(),datasetNames.end(), attrDataset) == datasetNames.end()) 
                    {
                        cout << "adding " << attrDataset << endl;
                        datasetNames.push_back(attrDataset);

                        // get matching coordinate dataset names with the configuration file
                        Configuration::getInstance()->getMatchingCoordinateDatasetNames(shortname, datasetNames, timeName, latitudeName, longitudeName,
                                                                                        ignoreName);
                    }
                }
            }
        }
    }
    
    // check if all the coordinate datasets(lat/lon/time) have same coordinate size
    /*void checkCoorDatasetSize(DataSet* data, bool& inconsistentCoorDatasets)
    {
        DataSpace inspace = data->getSpace();
        int dim = inspace.getSimpleExtentNdims();
        hsize_t olddims[dim];
        inspace.getSimpleExtentDims(olddims);
        if (coordinateSize == 0) coordinateSize = olddims[0];
        if ((coordinateSize != olddims[0]) && (!inconsistentCoorDatasets)) inconsistentCoorDatasets = true;                
    }*/

    // update epoch time if different with existing product epoch
    /*void updateEpochTime(DataSet* time)
    {
        //cout << "updateEpochTime" << endl;
        
        Attribute attr;
        string attrName, attrValue;
        string timeStr = "T00:00:00";
        string epoch;
        
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
                string pathDelim(" ");
                vector<string> units;
                split(units, attrValue, is_any_of(pathDelim));
                epoch = units.at(units.size()-1) + timeStr;
                // if the epoch is different from the product epoch in configuration file or default epoch,
                // update it
                if (temporal->needToUpdateEpoch(epoch)) temporal->updateReferenceTime(epoch);
            }
        }
        
    }*/
    
    // map that captures coordinate datasets, keys are coordinate dataset names(string)
    // values are acutal coordinate datasets
    // e.g. key:/profile_1/delta_time, value: /profile_1/delta_time dataset object
    map<string, DataSet*> coorDatasets;

};
boost::unordered_map<string, Coordinate*> Coordinate::lookUpMap;
#endif
