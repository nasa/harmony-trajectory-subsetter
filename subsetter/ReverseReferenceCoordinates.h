#ifndef REVERSEREFERENCECOORDINATES_H
#define REVERSEREFERENCECOORDINATES_H

#include "Configuration.h"
#include "Coordinate.h"
#include "GeoPolygon.h"
#include "IndexSelection.h"
#include "LogLevel.h"
#include "Temporal.h"
#include "geobox.h"

#include "H5Cpp.h"

class ReverseReferenceCoordinates : public Coordinate
{
  public:
    ReverseReferenceCoordinates(std::string groupname,
                                std::vector<geobox> *geoboxes,
                                Temporal *temporal,
                                GeoPolygon *geoPolygon,
                                Configuration *config)
        : Coordinate(groupname, geoboxes, temporal, geoPolygon, config)
    {
    }

    ~ReverseReferenceCoordinates() { delete referencedIndexes; }

    /*
     * subset dataset by its reverse reference
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
                                     const std::string &shortName,
                                     SubsetDataLayers *subsetDataLayers,
                                     const std::string &groupname,
                                     std::vector<geobox> *geoboxes,
                                     Temporal *temporal,
                                     GeoPolygon *geoPolygon,
                                     Configuration *config)
    {
        LOG_DEBUG(
            "ReverseReferenceCoordinates::getCoordinate(): ENTER groupname:"
            << groupname);

        ReverseReferenceCoordinates *reverseCoor =
            new ReverseReferenceCoordinates(
                groupname, geoboxes, temporal, geoPolygon, config);
        reverseCoor->coordinateSize = 0;
        reverseCoor->setCoordinateSize(ingroup);
        reverseCoor->shortname = shortName;
        reverseCoor->ingroup = ingroup;

        // get referenced group name
        std::string referencedGroupname =
            config->getReferencedGroupname(reverseCoor->shortname, groupname);
        H5::Group referencedGroup = root.openGroup(referencedGroupname);
        Coordinate *referencedCoor;

        // if the referenced group Coordinate has been created, get it from the
        // lookUpMap else create it
        if (Coordinate::lookUp(referencedGroupname))
            referencedCoor = Coordinate::lookUpMap[referencedGroupname];
        else if (config->isFreeboardSwathSegmentGroup(reverseCoor->shortname,
                                                      referencedGroupname))
            referencedCoor = Coordinate::getCoordinate(root,
                                                       referencedGroup,
                                                       referencedGroupname,
                                                       reverseCoor->shortname,
                                                       subsetDataLayers,
                                                       geoboxes,
                                                       temporal,
                                                       geoPolygon,
                                                       config);
        else if (config->isFreeboardBeamSegmentGroup(reverseCoor->shortname,
                                                     referencedGroupname))
            referencedCoor = ReverseReferenceCoordinates::getCoordinate(
                root,
                referencedGroup,
                reverseCoor->shortname,
                subsetDataLayers,
                referencedGroupname,
                geoboxes,
                temporal,
                geoPolygon,
                config);
        else if (config->isReferenceSurfaceSectionGroup(reverseCoor->shortname,
                                                        referencedGroupname))
            referencedCoor = Coordinate::getCoordinate(root,
                                                       referencedGroup,
                                                       referencedGroupname,
                                                       reverseCoor->shortname,
                                                       subsetDataLayers,
                                                       geoboxes,
                                                       temporal,
                                                       geoPolygon,
                                                       config);

        // if the index selection object has been created, get it
        // else create it
        if (referencedCoor->indexesProcessed)
            reverseCoor->referencedIndexes = referencedCoor->indexes;
        else
            reverseCoor->referencedIndexes =
                referencedCoor->getIndexSelection();

        return reverseCoor;
    }

    /*
     * get/create IndexSelection object for reversed referenced groups(i.e.,
     * /freeboard_swath_group/gt1l/swath_freeboard)
     */
    virtual IndexSelection *getIndexSelection()
    {
        LOG_DEBUG("ReverseReferenceCoordinates::getIndexSelection(): ENTER");

        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL)
            return NULL;

        std::string indexBegName =
            config->getIndexBeginDatasetName(shortname, groupname);

        indexes = new IndexSelection(coordinateSize);

        std::shared_ptr<H5::DataSet> indexBegSet;

        if (!indexBegName.empty() and
            (H5Lexists(ingroup.getLocId(), indexBegName.c_str(), H5P_DEFAULT) >
             0))
        {
            indexBegSet = std::make_shared<H5::DataSet>(
                ingroup.openDataSet(indexBegName));
        }

        reverseSubset(indexBegSet.get());

        indexesProcessed = true;

        return indexes;
    }

  private:
    IndexSelection *referencedIndexes;
    std::string shortname;
    H5::Group ingroup;

    /*
     * limit the index range by referenced group indexSelection
     * @param DataSet indexBegSet: index begin dataset
     */
    void reverseSubset(H5::DataSet *indexBegSet)
    {
        LOG_DEBUG("ReverseReferenceCoordinates::reverseSubset(): ENTER");

        int32_t *indexBegin = new int32_t[coordinateSize];
        LOG_DEBUG(
            "ReverseReferenceCoordinates::reverseSubset(): coordinateSize:"
            << coordinateSize);
        indexBegSet->read(indexBegin, indexBegSet->getDataType());
        long start = 0, length = 0, end = 0, count = 0, newStart = 0,
             newLength = 0;

        // iterate through index selection of the referenced group (i.e., for
        // leads group ,iterate through freeboard swath group) if the value in
        // the index begin dataset  matches the indices in the index selection,
        // calculate the index range for that value, add it the index selection
        for (std::map<long, long>::iterator it =
                 referencedIndexes->segments.begin();
             it != referencedIndexes->segments.end();
             it++)
        {
            start = it->first + 1;
            length = it->second;
            end = start + length;

            // if start is greater than the last value in the index begin
            // dataset skip this referenced index selection pair
            if (start > indexBegin[coordinateSize - 1])
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

            for (int i = coordinateSize - 1; i >= 0; i--)
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
            for (int i = coordinateSize - 1; i >= 0; i--)
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
        if (indexes->segments.empty())
            indexes->addRestriction(0, 0);

        delete[] indexBegin;
    }
};
#endif
