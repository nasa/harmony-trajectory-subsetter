#ifndef FORWARDREFERENCECOORDINATES_H
#define FORWARDREFERENCECOORDINATES_H

#include "Configuration.h"
#include "Coordinate.h"
#include "LogLevel.h"
#include "SuperGroupCoordinate.h"

#include "H5Cpp.h"
#include <boost/algorithm/string/find.hpp>

/*
 * Subclass of Coordinate for forward-reference segment-begin + count
 * coordinates These coordinates are associated with a target dataset (segmented
 * trajectory), but refer to segment-control datasets (begin, count) typically
 * in a separate segment group. This class Includes getIndexSelection and
 * Segmented Trajectory Subset operations.
 *
 * The count dataset is not used in the forward-reference calculation,
 * due to the existence of data collections whose count dataset is not
 * representative of the non-fill value size of an index begin segment (e.g.,
 * GEDI). So, a more generalized index scanning approach is used here.
 */
class ForwardReferenceCoordinates : public Coordinate
{
  public:
    IndexSelection *segIndexes = nullptr;
    // Selected Segments - computed in SegmentedTrajectorySubset method

    ForwardReferenceCoordinates // main constructor for class
        (std::string groupname,
         std::vector<geobox> *geoboxes,
         Temporal *temporal,
         GeoPolygon *geoPolygon,
         Configuration *config)
        : Coordinate(groupname, geoboxes, temporal, geoPolygon, config)
    {
    }

    ~ForwardReferenceCoordinates() // destructor
    {
        delete segIndexes;
    }

    /*
     * Get/populate the coordinate and IndexSelection object for ATL03
     * photon level subsetting based on segment group and for ATL10
     * leads group based on freeboard swath segment.
     * ex. /gt1l/geolocation
     * ex. /freeboard_swath_segment
     * @param Group root: root group
     * @param Group ingroup: input group
     * @param string shortName: product short name
     * @param SubsetDataLayers subsetDataLayers: dataset names to include in the
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
        LOG_DEBUG(
            "ForwardReferenceCoordinates::getCoordinate(): ENTER groupname: "
            << groupname);

        if (Coordinate::lookUp(groupname))
        {
            LOG_DEBUG(
                "ForwardReferenceCoordinates::getCoordinate(): groupname: "
                << " already exists in lookUpMap(ForwardReferenceCoordinate)");
            return lookUpMap[groupname];
        }

        ForwardReferenceCoordinates *forCoor = new ForwardReferenceCoordinates(
            groupname, geoboxes, temporal, geoPolygon, config);

        forCoor->coordinateSize = 0;
        forCoor->shortname = shortName;

        // get the size of the target (segmented trajectory) dataset for this
        // group
        if (config->isPhotonDataset(shortName, groupname))
        {
            H5::DataSet data = root.openDataSet(groupname);
            H5::DataSpace inspace = data.getSpace();
            int dim = inspace.getSimpleExtentNdims();
            hsize_t olddims[dim];
            inspace.getSimpleExtentDims(olddims);
            forCoor->coordinateSize = olddims[0];
        }
        else
        {
            forCoor->setCoordinateSize(ingroup);
        }

        // get the segment control group name
        std::string segGroupname =
            config->getReferencedGroupname(shortName, groupname);

        // if segment control group does not exist, return everything
        if (H5Lexists(root.getLocId(), segGroupname.c_str(), H5P_DEFAULT) == 0)
        {
            forCoor->indexesProcessed = true;
            return forCoor;
        }

        // open segment control group and retrieve the segment control group
        // coordinate object
        forCoor->segGroup = root.openGroup(segGroupname);
        Coordinate *coor;
        if (config->subsetBySuperGroup(shortName, segGroupname))
        {
            coor = SuperGroupCoordinate::getCoordinate(root,
                                                       forCoor->segGroup,
                                                       shortName,
                                                       subsetDataLayers,
                                                       segGroupname,
                                                       geoboxes,
                                                       temporal,
                                                       geoPolygon,
                                                       config);
        }
        else
        {
            coor = Coordinate::getCoordinate(root,
                                             forCoor->segGroup,
                                             segGroupname,
                                             shortName,
                                             subsetDataLayers,
                                             geoboxes,
                                             temporal,
                                             geoPolygon,
                                             config);
        }

        // if the IndexSelection object for the segment group has
        // already been processed, use it. Otherwise, process it
        if (coor->indexesProcessed)
        {
            forCoor->segIndexes = coor->indexes;
        }
        else
        {
            forCoor->segIndexes = coor->getIndexSelection();
        }

        // insert new coordinate object (forCoor) in lookup map
        // (coordinates have been established, and can be reused)
        Coordinate::lookUpMap.insert(std::make_pair(groupname, forCoor));

        return forCoor;
    }

    /*
     * get/create IndexSelection object for forward reference target
     * (photon/leads) level subsetting (i.e. /gt1l/heights group, /gt1l/leads)
     */
    virtual IndexSelection *getIndexSelection()
    {
        LOG_DEBUG(" ForwardReferenceCoordinates::getIndexSelection(): ENTER");

        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL)
        {
            return NULL;
        }

        H5::DataSet *indexBegSet = nullptr;

        indexes = new IndexSelection(coordinateSize);
        // self.coordinateSize = target segmented trajectory size

        std::string indexBegName, countName;
        // The count name is required for accessing dataset names.

        // Get dataset names for the datasets that provides the
        // starting index. (indexBeg) in the target (segmented
        // trajectory, photon) group and the number of elements
        // (photons, segmentPnCnt) in the segment that follow in
        // sequence from this start index.
        config->getDatasetNames(shortname, groupname, indexBegName, countName);

        if (!indexBegName.empty() &&
            H5Lexists(segGroup.getLocId(), indexBegName.c_str(), H5P_DEFAULT) >
                0)
        {
            indexBegSet = new H5::DataSet(segGroup.openDataSet(indexBegName));
        }
        else
        {
            // if fbswath_lead_ndx_gt<1l,1r....> doesn't exist,
            // try fbswath_lead_ndx_gt<1...6> for ATL10
            config->groundTrackRename(groupname, indexBegName, countName);
            indexBegSet = new H5::DataSet(segGroup.openDataSet(indexBegName));
        }

        // Compute the index-selection object for this coordinates - do the
        // subset
        segmentedTrajectorySubset(indexBegSet);

        indexesProcessed = true;

        return indexes;
    }

    /**
     * Scan forwards, from segStart to segEnd in indexBeg array
     * for a non-fill value.
     *
     * @param segStartIdx     The start index of the input segment.
     * @param segEndIdx       The end index of the input segment.
     * @param firstTrajIndex  The trajectory index with the first non-fill
     *                        indexBeg value.
     * @param firstNonFillIdx The first non-fill indexBeg index.
     * @param indexBegDatset  The input indexBeg dataset array.
     */
    void scanFwdNonFill(long segStartIdx,
                        long segEndIdx,
                        long &firstTrajIndex,
                        long &firstNonFillIdx,
                        int64_t indexBegDataset[])
    {
        // skip over segment-begin (start) fill values
        for (long i = segStartIdx; i <= segEndIdx; i++)
        {
            if (indexBegDataset[i] > 0)
            {
                firstNonFillIdx = i;
                firstTrajIndex = indexBegDataset[firstNonFillIdx];
                break;
            }
        }
    }

    /** Scan backwards, from segEnd to segStart in indexBeg array
     * for a non-fill value.
     *
     * @param segEndIdx       The end index of input segment.
     * @param segStartIdx     The start index of the input segment.
     * @param lastTrajIndex   The trajectory index with the last non-fill
     *                        indexBeg value.
     * @param lastNonFillIdx  The last non-fill indexBeg index.
     * @param indexBegDatset  The input indexBeg dataset array.
     */
    void scanBackNonFill(long segEndIdx,
                         long segStartIdx,
                         long &lastTrajIndex,
                         long &lastNonFillIdx,
                         int64_t indexBegDataset[])
    {
        // skip over segment-begin (start) fill values
        for (long i = segEndIdx; i >= segStartIdx; i--)
        {
            if (indexBegDataset[i] > 0)
            {
                lastNonFillIdx = i;
                lastTrajIndex = indexBegDataset[lastNonFillIdx];
                break;
            }
        }
    }

    /**
     * @brief Return the segment's first trajectory index and its length.
     *
     * Note: This removes the count dataset dependency to calculate
     *       trajectory segment lengths. This was done because there exist
     *       collections (such as ATL03) where the forward references
     *       (e.g. index begin) contain fill values (zero). The count dataset
     *       only considers non-fill index begin values Forward reference
     *       datasets point to trajectory dataset indexes, where the trajectory
     *       and count datasets only take into account non-fill index values.
     *
     *       The output index begin dataset will contain no fill values.
     *
     * @param selectedStartIdx The start index of the selected segment in the
     *                         index begin dataset.
     * @param selectedCount    The total sum of counts for each index begin
     *                         segment from the selected start index begin
     *                         to the last non-fill value index begin value.
     * @param firstTrajIndex   The trajectory index associated with the first
     *                         non-fill indexBeg value.
     * @param trajSegLength    The length of the trajectory segment.
     * @param maxIndexBegIdx   The final index of the entire indexBeg
     *                         dataset.
     * @param maxTrajIndex     The final index of the entire trajectory
     *                         dataset.
     * @param indexBegDataset  The input indexBeg dataset array.
     */
    void defineOneSegment(long selectedStartIdx,
                          long selectedCount,
                          long &firstTrajIndex,
                          long &trajSegLength,
                          long maxIndexBegIdx,
                          long maxTrajIndex,
                          int64_t indexBegDataset[])
    {
        LOG_DEBUG(" ForwardReferenceCoordinates::defineOneSegment(): ENTER");

        long firstIdxNonFill = 0; // first non-fill indexBeg value in
                                  // selected segment

        // The end index of input segment.
        long lastSelectedIdx = selectedStartIdx + selectedCount - 1;

        // Step forwards to find first non-fill index begin segment.
        scanFwdNonFill(selectedStartIdx,
                       lastSelectedIdx,
                       firstTrajIndex,
                       firstIdxNonFill,
                       indexBegDataset);

        // Step backwards to find last non-fill index begin segment.
        long lastTrajIndex = 0; // trajectory index with the last non-fill
                                // indexBeg value.
        long lastBegIdx = 0;    // last non-fill indexBeg index.
        scanBackNonFill(lastSelectedIdx,
                        firstIdxNonFill,
                        lastTrajIndex,
                        lastBegIdx,
                        indexBegDataset);

        // if not found - skip this selected segment group
        if (lastTrajIndex <= 0)
        {
            firstTrajIndex = 0;
            trajSegLength = 0;
            return;
        }

        // If this is the last segment of the input trajectory, then the
        // length of this trajectory segment is just the number of trajectory
        // values between the first value of this segment and the last
        // trajectory value.
        if (lastSelectedIdx + 1 == maxIndexBegIdx)
        {
            trajSegLength = maxTrajIndex - indexBegDataset[firstIdxNonFill] + 1;
            return;
        }

        // Need to find end of this segment. Unfortunately, in GEDI data, the
        // count dataset does not define the end of the full segment. It does
        // not include the padding that is present in the waveform (segmented
        // trajectory) data.

        // Step forwards to find the next non-fill segment begin, using segment
        // group max index as last segment begin reference to look at.
        long nextBegIdx = 0;    // non-fill begin index after the last
                                // selected index begin segment
        long nextTrajIndex = 0; // trajectory index associated with the
                                // index begin segment after the last
                                // selected segment.

        scanFwdNonFill(lastBegIdx + 1,
                       maxIndexBegIdx,
                       nextTrajIndex,
                       nextBegIdx,
                       indexBegDataset);

        // We need to calculate the length of the last segment in the selection
        // since we can't use the count dataset.
        // We don't subtract 1 from either count calculation because
        // neither include the greater value.
        long allExceptLastCount =
            lastTrajIndex - firstTrajIndex; // Doesn't include last index
        long lastCount =
            nextTrajIndex - lastTrajIndex; // Doesn't include next index
        trajSegLength = allExceptLastCount + lastCount;
    }

  private:
    H5::Group segGroup;
    std::string shortname;

    /*
     * Subset the segmented trajectory dataset - define the index
     * selection sets, limiting the index range by indexBeg starting
     * value and last index of last selected segment.
     *
     * We use (next non-fill indexBegin) to compute segment sizes.
     *
     * @param indexBegSet: index begin dataset
     */
    void segmentedTrajectorySubset(H5::DataSet *indexBegSet)
    {
        LOG_DEBUG(
            "ForwardReferenceCoordinates::segmentedTrajectorySubset(): ENTER");

        size_t idxBegSize = indexBegSet->getSpace().getSimpleExtentNpoints();

        // Index begin datasets for ATL03 and ATL08 are 64-bit and 32-bit for
        // ATL10.
        hid_t indexBeg_native_type = H5Tget_native_type(
            H5Dget_type(indexBegSet->getId()), H5T_DIR_ASCEND);

        int64_t *indexBeg = new int64_t[idxBegSize];

        // Load index begin dataset.
        // Reading in the source data is data-type size specific:
        if (H5Tequal(indexBeg_native_type, H5T_NATIVE_LLONG)) // 64-bit int
        {
            indexBegSet->read(indexBeg, indexBegSet->getDataType());
        }
        else if (H5Tequal(indexBeg_native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            int32_t *data = new int32_t[idxBegSize];
            indexBegSet->read(data, indexBegSet->getDataType());
            for (int i = 0; i < idxBegSize; i++)
            {
                indexBeg[i] = data[i];
            }
            delete[] data;
        }
        else if (H5Tequal(indexBeg_native_type,
                          H5T_NATIVE_ULLONG)) // unsigned 64-bit int
        {
            uint64_t *data = new uint64_t[idxBegSize];
            indexBegSet->read(data, indexBegSet->getDataType());
            for (int i = 0; i < idxBegSize; i++)
            {
                indexBeg[i] = data[i];
            }
            delete[] data;
        }
        // Create Segment reference - start index and length
        // ** avoiding selected segment references that are fill values **
        //
        // start = first selected non-fill indexBeg - 1
        //    (indexBegin values are 1 based indexing,
        //     selection start is 0 based indexing)
        //
        // length = last selected non-fill indexBeg - start
        //             - 1 + size-last-selected-segment;
        for (std::map<long, long>::iterator it = segIndexes->segments.begin();
             it != segIndexes->segments.end();
             it++)
        {
            long selectedStart = it->first;
            long selectedCount = it->second;
            long start = 0, length = 0;

            defineOneSegment(selectedStart,
                             selectedCount,
                             start,
                             length,
                             idxBegSize,
                             coordinateSize,
                             indexBeg);

            // Note: index-selection start is true to datasets,
            // zero based indexing, whereas start index pulled from
            // indexBegin datasets is one based indexing one based
            // indexing.
            if (start > 0)
            {
                indexes->addSegment(start - 1, length);
            }
        }

        // If no spatial subsetting, include all segments.
        if (segIndexes->segments.empty())
        {
            long start = 0, length = 0;

            long selectedStart = segIndexes->minIndexStart;
            long selectedCount = segIndexes->maxIndexEnd - selectedStart - 1;

            defineOneSegment(selectedStart,
                             selectedCount,
                             start,
                             length,
                             idxBegSize,
                             coordinateSize,
                             indexBeg);

            // Note: index-selection start is true to datasets, zero based
            // indexing, whereas start index pulled from indexBegin datasets is
            // one based indexing
            if (start > 0)
            {
                indexes->addSegment(start - 1, length);
            }
        }

        // No data found matched the spatial/temporal constraints, return no
        // data.
        if (indexes->segments.empty())
        {
            indexes->addRestriction(0, 0);
            LOG_DEBUG(
                "ForwardReferenceCoordinates::segmentedTrajectorySubset(): "
                << "No data found in trajectory subset that matched the "
                   "spatial/temporal constraints.");
        }

        delete[] indexBeg;
    }
};

#endif
