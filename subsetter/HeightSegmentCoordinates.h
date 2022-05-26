#ifndef HEIGHTSEGMENTCOORDINATES_H
#define	HEIGHTSEGMENTCOORDINATES_H

#include "Coordinate.h"
#include "Configuration.h"

#include <boost/algorithm/string/find.hpp>

using namespace std;

class HeightSegmentCoordinates: public Coordinate
{
public:
        
    HeightSegmentCoordinates(string groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    : Coordinate(groupname, geoboxes, temporal, geoPolygon)
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
     * @param SubsetDataLayers subsetDataLayers: dataset name to include in the output
     */
     static Coordinate* getCoordinate(Group& root, Group& ingroup, const string& shortName, SubsetDataLayers* subsetDataLayers,
            const string& groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    {
        cout << "HeightSegmentCoordinates" << endl;

        if (Coordinate::lookUp(groupname))
        {
            cout << groupname << " already exists in lookUpMap(HeightSegmentCoordinate)" << endl;
            return lookUpMap["HeightSegmentRate"];
        }
        
        HeightSegmentCoordinates* hgtSegCoor = new HeightSegmentCoordinates(groupname, geoboxes, temporal, geoPolygon);
        
        hgtSegCoor->coordinateSize = 0;
        hgtSegCoor->setCoordinateSize(ingroup);
     	hgtSegCoor->shortname = shortName;
        
        // get coordinate reference from in group's coordinates attribute(latitude/longitude and/or delta_time)
        Coordinate* coor = Coordinate::getCoordinate(root, ingroup, groupname, shortName, 
                            subsetDataLayers, geoboxes, temporal, geoPolygon);
        if (coor->indexesProcessed) hgtSegCoor->localIndexes = coor->indexes;
        else hgtSegCoor->localIndexes = coor->getIndexSelection();
        
        // get forward coordinate reference from leads group
        string leadsGroupname = Configuration::getInstance()->getLeadsGroup(shortName, groupname);
        //cout << "leadsGroup: " << leadsGroupname << endl;
        Coordinate* leadsCoor;
        hgtSegCoor->leadsGroup = root.openGroup(leadsGroupname);
        if (Coordinate::lookUp(leadsGroupname)) leadsCoor = lookUpMap[leadsGroupname];
        else leadsCoor = ForwardReferenceCoordinates::getCoordinate(root, hgtSegCoor->leadsGroup, shortName, subsetDataLayers, leadsGroupname, geoboxes, temporal, geoPolygon);
        //leadsCoor->getCoordinate(root, leadsGroup, shortName, subsetDataLayers);
        if (hgtSegCoor->indexesProcessed) hgtSegCoor->leadsIndexes = leadsCoor->indexes;
        else hgtSegCoor->leadsIndexes = leadsCoor->getIndexSelection();
        Coordinate::lookUpMap.insert(std::make_pair("HeightSegmentRate", hgtSegCoor));
        
        return hgtSegCoor;
    }
    
    /*
     * create IndexSelection for height segment rates group(i.e. /gt1l/freeboard_beam_segment/beam_freeboard)
     *      - merge IndexSelection objects: local latitude/longitude/delta_time and leads group
     */
    virtual IndexSelection* getIndexSelection()
    {
        indexes = new IndexSelection(coordinateSize);

        DataSet *indexBegSet = NULL, *countSet = NULL;

        string indexBegName, countName;

	Configuration::getInstance()->getDatasetNames(shortname, groupname, indexBegName, countName);
        //cout << "indexBegin: " << indexBegName << endl;
        //cout << "countName: " << countName << endl;
        if (!indexBegName.empty() && H5Lexists(leadsGroup.getLocId(), indexBegName.c_str(), H5P_DEFAULT) > 0)
            indexBegSet = new DataSet(leadsGroup.openDataSet(indexBegName));
        if (!countName.empty() && H5Lexists(leadsGroup.getLocId(), countName.c_str(), H5P_DEFAULT) > 0)
            countSet = new DataSet(leadsGroup.openDataSet(countName));

	DataSet* nonNullDataset = (indexBegSet != NULL)? indexBegSet : countSet;
        size_t coordinateSize = nonNullDataset->getSpace().getSimpleExtentNpoints();
	hid_t native_type = H5Tget_native_type(H5Dget_type(indexBegSet->getId()), H5T_DIR_ASCEND);

        // index minIndexStart datasets for ATL03 and ATL08 are 64-bit and 32-bit for ATL10
        int64_t* indexBeg = new int64_t[coordinateSize];
        int32_t* count = new int32_t[coordinateSize];

        if (H5Tequal(native_type, H5T_NATIVE_LLONG)) // 64-bit int
        {
            indexBegSet->read(indexBeg, indexBegSet->getDataType());
        }
        else if(H5Tequal(native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            int32_t* data = new int32_t[coordinateSize];
            indexBegSet->read(data, indexBegSet->getDataType());
            for (int i = 0; i < coordinateSize; i++) indexBeg[i] = data[i];
            delete[] data;
        }

        countSet->read(count, countSet->getDataType());

        //cout << "localIndexes" << endl; 
        // add (start, length) pairs in the local coordinate reference
        for (map<long, long>::iterator it = localIndexes->segments.begin(); it != localIndexes->segments.end(); it++)
        {
            indexes->addSegment(it->first, it->second);
        }

        //cout << "leadsIndexes" << endl;
        // add (start, length) pairs in the leads IndexSelection
        long start, length;
        for (map<long, long>::iterator it = leadsIndexes->segments.begin(); it != leadsIndexes->segments.end(); it++)
        {
            start = it->first;
            length = it->second;
            for (int i = start; i < length+start; i++)
            {
		//cout << i << " - " << "indexBegin: " << indexBeg[i] << "; count: " << count[i] << endl;
		indexes->addSegment(indexBeg[i] - 1, count[i]);
            }
        }
        indexesProcessed = true;

        return indexes;
    }
private:
    IndexSelection* leadsIndexes;
    //ForwardReferenceCoordinates* leadsCoor;
    IndexSelection* localIndexes;
    Group leadsGroup;
    string shortname;
};
#endif
