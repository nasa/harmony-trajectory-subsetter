#ifndef FORWARDREFERENCECOORDINATES_H
#define	FORWARDREFERENCECOORDINATES_H

#include "Coordinate.h"
#include "Configuration.h"
#include "SuperGroupCoordinate.h"

#include <boost/algorithm/string/find.hpp>

using namespace std;

class ForwardReferenceCoordinates: public Coordinate
{
public:
    IndexSelection* segIndexes;
    
    ForwardReferenceCoordinates(string groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    : Coordinate(groupname, geoboxes, temporal, geoPolygon)
    {
    }
    ~ForwardReferenceCoordinates()
    {
        delete segIndexes;
    }
    
    /*
     * get the IndexSelection object for ATL03 photon level subsetting based on segment group
     * get the IndexSelection object for ATL10 leads group based on freeboard swath segment
     * ex. /gt1l/geolocation
     * ex. /freeboard_swath_segment
     * @param Group root: root group
     * @param Group ingroup: input group
     * @param string shortName: product short name
     * @param SubsetDataLayers subsetDataLayers: dataset name to include in the output
     */
    static Coordinate* getCoordinate(Group& root, Group& ingroup, const string& shortName, SubsetDataLayers* subsetDataLayers,
                    const string& groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    {
        cout << "groupname: " << groupname << endl;
        if (Coordinate::lookUp(groupname))
        {
            cout << groupname << " already exists in lookUpMap(ForwardReferenceCoordinate)" << endl;
            return lookUpMap[groupname];
        }
        
        ForwardReferenceCoordinates* forCoor = new ForwardReferenceCoordinates(groupname, geoboxes, temporal, geoPolygon);
        cout << "getSegIndexSelection " << groupname << endl;
        forCoor->coordinateSize = 0;
        if (Configuration::getInstance()->isPhotonDataset(shortName,groupname))
        {
            DataSet data = root.openDataSet(groupname);
            DataSpace inspace = data.getSpace();
            int dim = inspace.getSimpleExtentNdims();
            hsize_t olddims[dim];
            inspace.getSimpleExtentDims(olddims);
            forCoor->coordinateSize = olddims[0];
        }
        else
        {
            forCoor->setCoordinateSize(ingroup);
        }
        forCoor->shortname = shortName;
        //cout << "shotName: " << shortName << endl;
      
        // get the segment group name 
        string segGroupname = Configuration::getInstance()->getReferencedGroupname(shortName, groupname);
        
        // if segment group does not exist, return everything
        if (H5Lexists(root.getLocId(), segGroupname.c_str(), H5P_DEFAULT) == 0)
        {
            forCoor->indexesProcessed = true;
            return forCoor;
        }
        
        forCoor->segGroup = root.openGroup(segGroupname);
        Coordinate* coor;
        if (Configuration::getInstance()->subsetBySuperGroup(shortName, segGroupname))
        {
            coor = SuperGroupCoordinate::getCoordinate(root, forCoor->segGroup, shortName, subsetDataLayers, segGroupname, geoboxes, temporal, geoPolygon);
        }
        else
        {
            coor = Coordinate::getCoordinate(root, forCoor->segGroup, segGroupname, shortName, subsetDataLayers, geoboxes, temporal, geoPolygon);
        }
        
        // if the IndexSelection object for the segment group has already been processed, use it
        // otherwise, process it
        if (coor->indexesProcessed) forCoor->segIndexes = coor->indexes;
        else forCoor->segIndexes = coor->getIndexSelection();
        
        Coordinate::lookUpMap.insert(std::make_pair(groupname, forCoor));
        
        return forCoor;
        
    }
    
    /*
     * get/create IndexSelection object for photon/leads level subsetting (i.e. /gt1l/heights group, /gt1l/leads)
     */
    virtual IndexSelection* getIndexSelection()
    {        
        cout << " ForwardReferenceCoordinates getIndexSelection" << endl;
        
        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL) return NULL;
        
        DataSet *indexBegSet = NULL, *countSet = NULL;
                
        indexes = new IndexSelection(coordinateSize);
        
        string indexBegName, countName;
        
        // get dataset names for the datasets that provides the the starting index in the photon group (indexBeg)
        // and number of photons in the segment follow in sequence from this index value (segmentPnCnt)
        Configuration::getInstance()->getDatasetNames(shortname, groupname, indexBegName, countName);
        if (!indexBegName.empty() && H5Lexists(segGroup.getLocId(), indexBegName.c_str(), H5P_DEFAULT) > 0)
        {
            indexBegSet = new DataSet(segGroup.openDataSet(indexBegName));
        }
        else
        {
            // if fbswath_lead_ndx_gt<1l,1r....> doesn't exist, try fbswath_lead_ndx_gt<1...6> for ATL10
            Configuration::getInstance()->groundTrackRename(groupname, indexBegName, countName);
            indexBegSet = new DataSet(segGroup.openDataSet(indexBegName));
        }
        if (!countName.empty() && H5Lexists(segGroup.getLocId(), countName.c_str(), H5P_DEFAULT) > 0)
            countSet = new DataSet(segGroup.openDataSet(countName));

        photonSubset(indexBegSet, countSet);
                
        indexesProcessed = true;
        
        return indexes;
    }
    
private:
    
    Group segGroup;
    string shortname;
    
    /*
     * limit the index range by indexBeg and count datasets
     * @param DataSet indexBegSet: index begin dataset
     * @param DataSet countSet: index count dataset
     */
    void photonSubset(DataSet* indexBegSet, DataSet* countSet)
    {
        cout << "photonSubset" << endl;
                
        long start = 0, length = 0, segStart = 0, segLength = 0;
        
        DataSet* nonNullDataset = (indexBegSet != NULL)? indexBegSet : countSet;
        size_t coordinateSize = nonNullDataset->getSpace().getSimpleExtentNpoints();
        //cout << "coordinateSize: " << coordinateSize << endl;
        
        // index begin datasets for ATL03 and ATL08 are 64-bit and 32-bit for ATL10
        hid_t indexBeg_native_type = H5Tget_native_type(H5Dget_type(indexBegSet->getId()), H5T_DIR_ASCEND);
        hid_t count_native_type = H5Tget_native_type(H5Dget_type(countSet->getId()), H5T_DIR_ASCEND);

        int64_t* indexBeg = new int64_t[coordinateSize];
        int32_t* count = new int32_t[coordinateSize];

        if (H5Tequal(indexBeg_native_type, H5T_NATIVE_LLONG)) // 64-bit int
        {
            indexBegSet->read(indexBeg, indexBegSet->getDataType());
        }
        else if(H5Tequal(indexBeg_native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            int32_t* data = new int32_t[coordinateSize];
            indexBegSet->read(data, indexBegSet->getDataType());
            for (int i = 0; i < coordinateSize; i++) indexBeg[i] = data[i];
            delete[] data;
        }
        else if(H5Tequal(indexBeg_native_type, H5T_NATIVE_ULLONG)) // unsigned 64-bit int
        {
            uint64_t* data = new uint64_t[coordinateSize];
            indexBegSet->read(data, indexBegSet->getDataType());
            for (int i = 0; i < coordinateSize; i++) indexBeg[i] = data[i];
            delete[] data;
        }

        if (H5Tequal(count_native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            countSet->read(count, countSet->getDataType());
        }
        else if(H5Tequal(count_native_type, H5T_NATIVE_USHORT)) // unsigend 16-bit int
        {
            uint16_t* data = new uint16_t[coordinateSize];
            countSet->read(data, countSet->getDataType());
            for (int i = 0; i < coordinateSize; i++) count[i] = data[i];
            delete[] data;
        }
        else if(H5Tequal(count_native_type, H5T_NATIVE_LLONG)) // 64-bit int
        {
            int64_t* data = new int64_t[coordinateSize];
            countSet->read(data, countSet->getDataType());
            for (int i = 0; i < coordinateSize; i++) count[i] = data[i];
            delete[] data;
        }
                
        // set start and length for the photon level subsetting
        // start = first non-zero indexBeg - 1
        // length = last non-zero indexBeg - 1 + last non-zero count - start;
        for (map<long, long>::iterator it = segIndexes->segments.begin(); it != segIndexes->segments.end(); it++)
        {
            start = 0, length = 0;
            segStart = it->first;
            segLength = it->second;

            for (int i = 0; i < segLength; i++)
            {
                if (indexBeg[segStart+i] > 0 )
                {
                    start = indexBeg[segStart+i] - 1;
                    break;
                }
            }

            for (int i = segLength-1; i >= 0; i--)
            {
                if (indexBeg[segStart+i] > 0)
                {
                    long countFull = indexBeg[segStart+i+1] - indexBeg[segStart+i]; // This includes data padding, if it exists.
                    length = indexBeg[segStart+i] - 1 + countFull - start;
                    indexes->addSegment(start, length);
                    break;
                }
            }
        }
        
        // if no spatial subsetting
        if (segIndexes->segments.empty())
        {
            // index selection end is excluded
            for (int i = segIndexes->minIndexStart; i < segIndexes->maxIndexEnd; i++)
            {
                if (indexBeg[i] > 0)
                {
                    start = indexBeg[i] - 1;
                    break;
                }
            }
            for (int i = segIndexes->maxIndexEnd - 1; i >= segIndexes->minIndexStart; i--)
            {
                if (indexBeg[i] > 0)
                {
                    length = indexBeg[i] - 1 + count[i] - start;
                    indexes->addSegment(start, length);
                    break;
                }
            }
        }
        
        // no data found matched the spatial/temporal constraints, return no data
        if (indexes->segments.empty()) indexes->addRestriction(0, 0);
        
        delete [] indexBeg;
        delete [] count;
    }   
};
#endif

