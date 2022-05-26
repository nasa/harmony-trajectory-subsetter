#ifndef REVERSEREFERENCECOORDINATES_H
#define	REVERSEREFERENCECOORDINATES_H

#include "Configuration.h"


using namespace std;

class ReverseReferenceCoordinates: public Coordinate
{
public:

    ReverseReferenceCoordinates(string groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    : Coordinate(groupname, geoboxes, temporal, geoPolygon)
    {
    }
    
    ~ReverseReferenceCoordinates()
    {
        delete referencedIndexes;
    }
    
    /*
     * subset dataset by its reverse reference
     * @param Group root: root group
     * @param Group ingroup: input group
     * @param string shortName: product short name
     * @param SubsetDataLayers subsetDataLayers: dataset name to include in the output
     * @param String groupname: group name
     * @param Vector<geobox> geoboxes: bounding boxes(spatial constraints)
     * @param Temporal temporal: temporal constraint
     */
     static Coordinate* getCoordinate(Group& root, Group& ingroup, const string& shortName, SubsetDataLayers* subsetDataLayers,
            const string& groupname, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    {
        cout << "ReverseReferenceCoordinates" << endl;
         
        ReverseReferenceCoordinates* reverseCoor = new ReverseReferenceCoordinates(groupname, geoboxes, temporal, geoPolygon);
        reverseCoor->coordinateSize = 0;
        reverseCoor->setCoordinateSize(ingroup);
        reverseCoor->shortname = shortName;
        reverseCoor->ingroup = ingroup;
        
        // get referenced group name
        string referencedGroupname = Configuration::getInstance()->getReferencedGroupname(reverseCoor->shortname, groupname);
        //cout << "referencedGroupname: " << referencedGroupname << endl;
        Group referencedGroup = root.openGroup(referencedGroupname);
        Coordinate* referencedCoor;
        
        // if the referenced group Coordinate has been created, get it from the lookUpMap
        // else create it
        if (Coordinate::lookUp(referencedGroupname)) referencedCoor = Coordinate::lookUpMap[referencedGroupname];
        else if (Configuration::getInstance()->isFreeboardSwathSegmentGroup(reverseCoor->shortname, referencedGroupname))
            referencedCoor = Coordinate::getCoordinate(root, referencedGroup, referencedGroupname, reverseCoor->shortname, 
                    subsetDataLayers, geoboxes, temporal, geoPolygon);
        else if (Configuration::getInstance()->isFreeboardBeamSegmentGroup(reverseCoor->shortname, referencedGroupname))
            referencedCoor = ReverseReferenceCoordinates::getCoordinate(root, referencedGroup, reverseCoor->shortname, 
                    subsetDataLayers, referencedGroupname, geoboxes, temporal, geoPolygon);
        
        // if the index selection object has been created, get it
        // else create it
        if (referencedCoor->indexesProcessed) reverseCoor->referencedIndexes = referencedCoor->indexes;
        else reverseCoor->referencedIndexes = referencedCoor->getIndexSelection();
        
        return reverseCoor;
        
    }
     
    /*
     * get/create IndexSelection object for reversed referenced groups(i.e., /freeboard_swath_group/gt1l/swath_freeboard)
     */
    virtual IndexSelection* getIndexSelection()
    {
        cout << "ReverseReferenceCoordinates.getIndexSelection()" << endl;
        
        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL) return NULL;
        
        string indexBegName = Configuration::getInstance()->getIndexBeginDatasetName(shortname, groupname);
        
        indexes = new IndexSelection(coordinateSize);
        
        DataSet* indexBegSet = NULL;

        if (!indexBegName.empty())
        {
            //
            // look for a bar as a string delimiter for potential two values of index Selection
            //
            string token;
            string delimiter = "|";
            bool found = false;
            int pos;
            while (((pos = indexBegName.find(delimiter)) != string::npos) and (!found))
            {
               //
               // Isolate the first token found before the bar
               //
               token = indexBegName.substr(0, pos);  
               cout << "getIndexSelection Token is: " << token << endl;
               if (H5Lexists(ingroup.getLocId(), token.c_str(), H5P_DEFAULT) > 0)
               {
                  indexBegSet = new DataSet(ingroup.openDataSet(token));
                  // It existed so set our boolean
                  found = true;
               }    
               //
               // Keep searching through the string
               //
               indexBegName.erase(0, pos + delimiter.length());
            }
            
            if ((!found) and (H5Lexists(ingroup.getLocId(), indexBegName.c_str(), H5P_DEFAULT) > 0))
            {
               indexBegSet = new DataSet(ingroup.openDataSet(indexBegName));
            }
        }
        
        reverseSubset(indexBegSet);
        
        indexesProcessed = true;
        
        return indexes;
    }
private:
    
    IndexSelection* referencedIndexes;
    string shortname;
    Group ingroup;
    
    /*
     * limit the index range by referenced group indexSelection 
     * @param DataSet indexBegSet: index minIndexStart dataset
     */
    void reverseSubset(DataSet* indexBegSet)
    {
        cout << "reverseSubset" << endl;
        
        int32_t* indexBegin = new int32_t[coordinateSize];
        cout << "coordinateSize: " << coordinateSize << endl;
        indexBegSet->read(indexBegin, indexBegSet->getDataType());
        long start = 0, length = 0, end = 0, count = 0, newStart = 0, newLength = 0;
        
        // iterate through index selection of the referenced group (i.e., for leads group ,iterate through freeboard swath group)
        // if the value in the index minIndexStart dataset  matches the indices in the index selection,
        // calculate the index range for that value, add it the index selection
        for (map<long, long>::iterator it = referencedIndexes->segments.begin(); it != referencedIndexes->segments.end(); it++)
        {
            start = it->first + 1;
            length = it->second;
            end = start + length;

            //cout << "start: " << start << ", length: " << length << ", end: " << endl;
            //cout << "last index minIndexStart: " << indexBegin[coordinateSize-1] << endl;
            
            // if start is greater than the last value in the index minIndexStart dataset
            // skip this referenced index selection pair
            if (start > indexBegin[coordinateSize-1])
            {
                continue;
            }
            
            // if end is less than the first value in the index minIndexStart dataset
            // skip this referenced index selection pair
            if (end < indexBegin[0])
            {
                continue;
            }
            
            for (int i = 0; i < coordinateSize; i++)
            {
                if (indexBegin[i] >= start)
                {
                    //cout << "indexBegin[" << i << "]: " << indexBegin[i] << endl;
                    newStart = i;
                    break;
                }
                   
            }
            
            for (int i = coordinateSize-1; i >= 0; i--)
            {
                if (indexBegin[i] < end)
                {
                    newLength = i + 1 - newStart;
                    indexes->addSegment(newStart, newLength);
                    break;
                }
            }
            //cout << "newStart: " << newStart << endl;
            //cout << "newLength: " << newLength << endl;
        }
        
        // if no spatial subsetting
        if (referencedIndexes->segments.empty())
        {
            //cout << "no spatial subsetting" << endl;
            // index selection end is excluded
            start = referencedIndexes->minIndexStart;
            end = referencedIndexes->maxIndexEnd;
            
            //cout << "start: " << start << ", end: " << end << endl;
            
            for (int i = 0; i < coordinateSize; i++)
            {
                //cout << "indexBegin[" << i << "]: " << indexBegin[i] << endl;
                if (indexBegin[i] > start)
                {
                    newStart = i;
                    //cout << "newStart: " << newStart << endl;
                    break;
                }
            }
            for (int i = coordinateSize-1; i >= 0; i--)
            {
                if (indexBegin[i] <= end)
                {
                    newLength = i + 1 - newStart;
                    indexes->addSegment(newStart, newLength);
                    break;
                }
            }
            //cout << "newStart: " << newStart << endl;
            //cout << "newLength: " << newLength << endl;
        }
                
        // if no matching data found, return no data
        if (indexes->segments.empty()) indexes->addRestriction(0, 0);
        
        delete [] indexBegin;
    }
    
};
#endif
