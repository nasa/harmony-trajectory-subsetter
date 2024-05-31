#ifndef REVERSEREFERENCECOORDINATES_H
#define	REVERSEREFERENCECOORDINATES_H

#include "Configuration.h"


class ReverseReferenceCoordinates: public Coordinate
{
public:

    ReverseReferenceCoordinates(std::string groupname, std::vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
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
     static Coordinate* getCoordinate(H5::Group& root, H5::Group& ingroup, const std::string& shortName, SubsetDataLayers* subsetDataLayers,
            const std::string& groupname, std::vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    {
        std::cout << "ReverseReferenceCoordinates" << std::endl;

        ReverseReferenceCoordinates* reverseCoor = new ReverseReferenceCoordinates(groupname, geoboxes, temporal, geoPolygon);
        reverseCoor->coordinateSize = 0;
        reverseCoor->setCoordinateSize(ingroup);
        reverseCoor->shortname = shortName;
        reverseCoor->ingroup = ingroup;

        // get referenced group name
        std::string referencedGroupname = Configuration::getInstance()->getReferencedGroupname(reverseCoor->shortname, groupname);
        H5::Group referencedGroup = root.openGroup(referencedGroupname);
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
        std::cout << "ReverseReferenceCoordinates.getIndexSelection()" << std::endl;

        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL) return NULL;

        std::string indexBegName = Configuration::getInstance()->getIndexBeginDatasetName(shortname, groupname);

        indexes = new IndexSelection(coordinateSize);

        H5::DataSet* indexBegSet = NULL;

        if (!indexBegName.empty())
        {
            //
            // look for a bar as a string delimiter for potential two values of index Selection
            //
            std::string token;
            std::string delimiter = "|";
            bool found = false;
            int pos;
            while (((pos = indexBegName.find(delimiter)) != std::string::npos) and (!found))
            {
               //
               // Isolate the first token found before the bar
               //
               token = indexBegName.substr(0, pos);
               std::cout << "getIndexSelection Token is: " << token << std::endl;
               if (H5Lexists(ingroup.getLocId(), token.c_str(), H5P_DEFAULT) > 0)
               {
                  indexBegSet = new H5::DataSet(ingroup.openDataSet(token));
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
               indexBegSet = new H5::DataSet(ingroup.openDataSet(indexBegName));
            }
        }

        reverseSubset(indexBegSet);

        indexesProcessed = true;

        return indexes;
    }
private:

    IndexSelection* referencedIndexes;
    std::string shortname;
    H5::Group ingroup;

    /*
     * limit the index range by referenced group indexSelection
     * @param DataSet indexBegSet: index begin dataset
     */
    void reverseSubset(H5::DataSet* indexBegSet)
    {
        std::cout << "reverseSubset" << std::endl;

        int32_t* indexBegin = new int32_t[coordinateSize];
        std::cout << "coordinateSize: " << coordinateSize << std::endl;
        indexBegSet->read(indexBegin, indexBegSet->getDataType());
        long start = 0, length = 0, end = 0, count = 0, newStart = 0, newLength = 0;

        // iterate through index selection of the referenced group (i.e., for leads group ,iterate through freeboard swath group)
        // if the value in the index begin dataset  matches the indices in the index selection,
        // calculate the index range for that value, add it the index selection
        for (std::map<long, long>::iterator it = referencedIndexes->segments.begin(); it != referencedIndexes->segments.end(); it++)
        {
            start = it->first + 1;
            length = it->second;
            end = start + length;


            // if start is greater than the last value in the index begin dataset
            // skip this referenced index selection pair
            if (start > indexBegin[coordinateSize-1])
            {
                continue;
            }

            // if end is less than the first value in the index begin dataset
            // skip this referenced index selection pair
            if (end < indexBegin[0])
            {
                continue;
            }

            for (int i = 0; i < coordinateSize; i++)
            {
                if (indexBegin[i] >= start)
                {
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
        }

        // if no spatial subsetting
        if (referencedIndexes->segments.empty())
        {
            // index selection end is excluded
            start = referencedIndexes->minIndexStart;
            end = referencedIndexes->maxIndexEnd;


            for (int i = 0; i < coordinateSize; i++)
            {
                if (indexBegin[i] > start)
                {
                    newStart = i;
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
        }

        // if no matching data found, return no data
        if (indexes->segments.empty()) indexes->addRestriction(0, 0);

        delete [] indexBegin;
    }

};
#endif
