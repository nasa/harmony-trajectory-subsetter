#ifndef SUPERGROUPCOORDINATE_H
#define SUPERGROUPCOORDINATE_H

#include <stdlib.h>

using namespace std;

/**
 * class to write index begin datasets for ATL03 and ATL08
 */
class SuperGroupCoordinate: public Coordinate
{
public:
    SuperGroupCoordinate(string groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    : Coordinate(groupname, geoboxes, temporal, geoPolygon)
    {
    }
    
    /*
     * subset dataset the "super group"
     * when multiple sets of lat/lon coordinates references exist within one group and these coordinate references
     * all referencing one delta_time datasets, the datasets under that group will be subsetted using the "super group" rule.
     *  - ex.) lat_1, lon_1, lat_2, lon_2, lat_3, and lon_3 exist within one group, and all have reference to delta_time
     *         subsetting will be applied by <lat_1,lon_1> or <lat_2,lon_2> or <lat_3,lon_3> and delta_time(if temporal 
     *         constraint exists)
     * @param Group root: root group
     * @param Group ingroup: input group
     * @param string shortName: product short name
     * @param SubsetDataLayers subsetDataLayers: dataset name to include in the output
     * @param String groupname: group name
     * @param Vector<geobox> geoboxes: bounding boxes(spatial constraints)
     * @param Temporal temporal: temporal constraint
     */
    static Coordinate* getCoordinate(Group& root, Group& ingroup, const string& shortname, SubsetDataLayers* subsetDataLayers,
            const string& groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    {
        SuperGroupCoordinate* sgCoor = new SuperGroupCoordinate(groupname, geoboxes, temporal, geoPolygon);
        sgCoor->coordinateSize = 0;
        
        cout << "superGroup getCoordinate" << endl;
        map<string, vector<string>> datasets;
        string superGroupname, latitudeName, longitudeName, timeName, otherName;
        bool inconsistentCoorDatasets = false;
        DataSet* data = NULL;
        vector<string> superGroupnames;
        
        datasets = Configuration::getInstance()->getSuperCoordinates(shortname);
        Configuration::getInstance()->getSuperCoordinateGroupname(shortname, groupname, superGroupnames);
        for(vector<string>::iterator it = superGroupnames.begin(); it != superGroupnames.end(); it++)
        {
            Group group = root.openGroup(*it);
            sgCoor->superGroups.push_back(group);
        }
        superGroupname = superGroupnames[0].substr(0, superGroupnames[0].find_first_of("/\\", 1)+1);
        // if the Coordinate group already exists, return it, else, continue processing
        if (sgCoor->lookUp(superGroupname))
        {
            cout << superGroupname << " already exists in lookUpMap" << endl;
            return lookUpMap[superGroupname];
        }
        
        // distinguish between latitude, longitude and delta_time datasets
        //while (!datasets.empty())
        for (map<string, vector<string>>::iterator it = datasets.begin(); it != datasets.end(); it++)
        {
            superGroupnames.push_back(it->first);
            while (!it->second.empty())
            {
                Configuration::getInstance()->getMatchingCoordinateDatasetNames(shortname, it->second, timeName, latitudeName, longitudeName, otherName);
                if (!latitudeName.empty())
                {
                    sgCoor->latitudes.push_back(latitudeName);
                    it->second.erase(std::remove(it->second.begin(), it->second.end(), latitudeName), it->second.end());
                }
                if (!longitudeName.empty())
                {
                    sgCoor->longitudes.push_back(longitudeName);
                    it->second.erase(std::remove(it->second.begin(), it->second.end(), longitudeName), it->second.end());
                }
                if (!timeName.empty())
                {
                    it->second.erase(std::remove(it->second.begin(), it->second.end(), timeName), it->second.end());
                    for (vector<Group>::iterator it = sgCoor->superGroups.begin(); it != sgCoor->superGroups.end(); it++)
                    {
                        if (!timeName.empty() && H5Lexists(it->getLocId(), timeName.c_str(), H5P_DEFAULT) > 0)
                        {
                            data = new DataSet(it->openDataSet(timeName));
                            sgCoor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
                            sgCoor->coorDatasets.insert(std::pair<string, DataSet*>("time",data));
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
            for (vector<Group>::iterator it = sgCoor->superGroups.begin(); it != sgCoor->superGroups.end(); it++)
            {
                if (H5Lexists(it->getLocId(), sgCoor->latitudes[i].c_str(), H5P_DEFAULT) > 0)
                {
                    data = new DataSet(it->openDataSet(sgCoor->latitudes[i]));
                    sgCoor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
                    sgCoor->coorDatasets.insert(std::pair<string, DataSet*>(sgCoor->latitudes[i],data));
                }
            }
            for (vector<Group>::iterator it = sgCoor->superGroups.begin(); it != sgCoor->superGroups.end(); it++)
            {
                if (H5Lexists(it->getLocId(), sgCoor->longitudes[i].c_str(), H5P_DEFAULT) > 0)
                {
                    data = new DataSet(it->openDataSet(sgCoor->longitudes[i]));
                    sgCoor->checkCoorDatasetSize(data, inconsistentCoorDatasets);
                    sgCoor->coorDatasets.insert(std::pair<string, DataSet*>(sgCoor->longitudes[i],data));
                }
            }
        }        
                        
        // if the number of latitude datasets and the number of longitude datasets don't agree, or their
        // dataset sizes are not the same, return Index Selection with retstriction, start=0, length=0
        if ((sgCoor->latitudes.size() != sgCoor->longitudes.size()) || inconsistentCoorDatasets)
        {
            sgCoor->indexes = new IndexSelection(sgCoor->coordinateSize);
            sgCoor->indexes->addRestriction(0,0);
            sgCoor->indexesProcessed = true;
            return sgCoor;
        }
        
        lookUpMap.insert(std::make_pair(superGroupname, sgCoor));
        
        return sgCoor;
    }
    
    virtual IndexSelection* getIndexSelection()
    {
        cout << "superGroup getIndexSelection" << endl;
        DataSet* timeSet = NULL;
        double* time = new double[coordinateSize];
        
        indexes = new IndexSelection(coordinateSize);
        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL) return NULL;
        
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
                
        //cout << "coorDatasets.size(): " << this->coorDatasets.size() << endl;
        
        // read lat/lon datasets if spatial(bbox/polygon) constraints exist
        if ((geoboxes != NULL || geoPolygon != NULL) && this->coorDatasets.size() != 0)
        {
            for (int i = 0; i < this->latitudes.size(); i++)
            {
                double* lat = new double[coordinateSize];
                double* lon = new double[coordinateSize];
                readLatLonDatasets(this->coorDatasets.find(this->latitudes[i])->second, this->coorDatasets.find(this->longitudes[i])->second, lat,lon);
                this->coors.insert(std::pair<string, double*>(this->latitudes[i],lat));
                this->coors.insert(std::pair<string, double*>(this->longitudes[i],lon));
            }
        }
        
        // limit the index by spatial constraint
        if (geoboxes != NULL && this->coorDatasets.size() != 0) spatialBboxSubset(); 
        else cout << "spatial constraint or lat/lon coordinates not found" << endl;
        // limit the index by polygon
        if (geoPolygon != NULL && this->coorDatasets.size() != 0) spatialPolygonSubset();
        else cout << "polygon or lat/lon coordinates not found" << endl;
        
        indexesProcessed = true;
        
        return indexes;
    }
private:
    
    vector<Group> superGroups;
    vector<string> latitudes;
    vector<string> longitudes;
    map<string, DataSet*> coorDatasets;
    map<string, double*> coors;
    
    // limit the index range by spatial constraints
    void spatialBboxSubset()
    {
        cout << "superGroup spatialBboxSubset" << endl;
        bool contains = false;
        
        long indexBegin = indexes->minIndexStart, indexEnd = indexes->maxIndexEnd - 1;
        long start = 0, length = 0;
        for (int i=indexBegin; i<=indexEnd; i++)
        {
            vector<geobox>::iterator geobox_it = geoboxes->begin();
            for (; geobox_it != geoboxes->end(); geobox_it++)
            {
                for (int j = 0; j < latitudes.size(); j++)
                {
                    //cout << latitudes[j] << "," << longitudes[j] << " -- " << i << endl;
                    //cout << coors[latitudes[j]][i] << ", " << coors[longitudes[j]][i] << endl;
                    if (geobox_it->contains(coors[latitudes[j]][i], coors[longitudes[j]][i]))
                    {
                        contains = true;
                        //cout << "found " << coors[latitudes[j]][i] << ", " << coors[longitudes[j]][i] << " inside polygon" << endl;
                        break;
                    }
                    else contains = false;
                }
                if (contains)
                {
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
        
        // if new index range found, but does not have an end point, add it
        if (length != 0) indexes->addSegment(start, length);
        // if no index range found, return no data
        if (indexes->segments.empty()) indexes->addRestriction(0, 0);
    }
    
    // limit the index range by polygon
    void spatialPolygonSubset()
    {
        cout << "superGroup spatialPolygonSubset" << endl;
        IndexSelection* newIndexes = new IndexSelection(coordinateSize);
        
        geobox g = geoPolygon->getBbox();
        if (geoboxes == NULL) geoboxes = new vector<geobox>();
        geoboxes->push_back(g);
        
        cout << "geoboxes.size(): " << geoboxes->size() << endl;
        
        spatialBboxSubset();
        cout << "indexes->size(): " << indexes->size() << endl;
        
        if (indexes->size() == 0) return;
        //if (indexes->size() == 0 || geoPolygon->isBbox()) return;
        
        long start = 0, length = 0;
        bool contains = false;
        
        for (map<long, long>::iterator it = indexes->segments.begin(); it != indexes->segments.end(); it++)
        {
            for (int i = it->first; i != it->second+it->first; i++)
            {                
                // count in the points with fill values
                for (int j = 0; j < latitudes.size(); j++)
                {
                    if (geoPolygon->contains(coors[latitudes[j]][i], coors[longitudes[j]][i]))
                    {
                        contains = true;
                        break;
                        //cout << "point: (" << lat[i] << "," << lon[i] << ") is within the polygon" << endl;
                    }
                    else contains = false;
                }
                if (contains)
                {
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
        
        // if new index range found, but does not have an end point, add it
        if (length != 0) indexes->addSegment(start, length);
        // if no index range found, return no data
        if (indexes->segments.empty()) indexes->addRestriction(0, 0);
    }
};
#endif
