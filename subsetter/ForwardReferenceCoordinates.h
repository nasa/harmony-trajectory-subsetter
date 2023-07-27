#ifndef FORWARDREFERENCECOORDINATES_H
#define	FORWARDREFERENCECOORDINATES_H

#include "Coordinate.h"
#include "Configuration.h"
#include "SuperGroupCoordinate.h"

#include <boost/algorithm/string/find.hpp>

/*
 * Subclass of Coordinate for forward-reference segment-begin + count coordinates
 * These coordinates are associated with a target dataset (segmented trajectory),
 * but refer to segment-control datasets (begin, count) typically in a
 * separate segment group. This class Includes getIndexSelection and
 * Segmented Trajectory Subset operations.
 * 
 * The count dataset is not used in the forward-reference calculation, 
 * due to the existence of data collections whose count dataset is not
 * representative of the non-fill value size of an index begin segment (e.g., GEDI).
 * So, a more generalized index scanning approach is used here. 
 */
class ForwardReferenceCoordinates: public Coordinate
{
public:
    IndexSelection* segIndexes;
    // Selected Segments - computed in SegmentedTrajectorySubset method
    
    ForwardReferenceCoordinates   // main constructor for class
        (std::string groupname, std::vector<geobox>* geoboxes, 
         Temporal* temporal,    GeoPolygon* geoPolygon)
    : Coordinate(groupname, geoboxes, temporal, geoPolygon)
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
     * @param SubsetDataLayers subsetDataLayers: dataset names to include in the output
     */
    static Coordinate* getCoordinate
        ( H5::Group&           root,     
          H5::Group&           ingroup,
          const std::string&   shortName, 
          SubsetDataLayers*    subsetDataLayers, 
          const std::string&   groupname, 
          std::vector<geobox>* geoboxes,
          Temporal*            temporal, 
          GeoPolygon*          geoPolygon )
    {
        std::cout << "groupname: " << groupname << std::endl;
        if (Coordinate::lookUp(groupname))
        {
            std::cout << groupname 
                << " already exists in lookUpMap(ForwardReferenceCoordinate)" 
                << std::endl;
            return lookUpMap[groupname];
        }
        
        ForwardReferenceCoordinates* forCoor 
            = new ForwardReferenceCoordinates
                    (groupname, geoboxes, temporal, geoPolygon);
        std::cout << "getSegIndexSelection " << groupname << std::endl;

        forCoor->coordinateSize = 0;
        forCoor->shortname = shortName;

        // get the size of the target (segmented trajectory) dataset for this group
        if (Configuration::getInstance()->isPhotonDataset(shortName, groupname))
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
        std::string segGroupname 
            = Configuration::getInstance()
                -> getReferencedGroupname(shortName, groupname);
        
        // if segment control group does not exist, return everything
        if (H5Lexists(root.getLocId(), segGroupname.c_str(), H5P_DEFAULT) == 0)
        {
            forCoor->indexesProcessed = true;
            return forCoor;
        }
        
        // open segment control group and retrieve the segment control group
        // coordinate object
        forCoor->segGroup = root.openGroup(segGroupname);
        Coordinate* coor;
        if (Configuration::getInstance()
                -> subsetBySuperGroup(shortName, segGroupname))
        {
            coor = SuperGroupCoordinate::getCoordinate
                    ( root, forCoor->segGroup, shortName, 
                      subsetDataLayers, segGroupname, geoboxes, 
                      temporal, geoPolygon );
        }
        else
        {
            coor = Coordinate::getCoordinate
                    ( root, forCoor->segGroup, segGroupname, 
                      shortName, subsetDataLayers, geoboxes, 
                      temporal, geoPolygon );
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
     * get/create IndexSelection object for forward reference target (photon/leads)
     * level subsetting (i.e. /gt1l/heights group, /gt1l/leads)
     */
    virtual IndexSelection* getIndexSelection()
    {        
        std::cout << " ForwardReferenceCoordinates getIndexSelection" << std::endl;
        
        // if both temporal and spatial constraints don't exist,
        // return null to include all in the output
        if (geoboxes == NULL && temporal == NULL && geoPolygon == NULL) { return NULL;}
        
        H5::DataSet *indexBegSet = NULL;
                
        indexes = new IndexSelection(coordinateSize);
            // self.coordinateSize = target segmented trajectory size
        
        std::string indexBegName, countName;  
            // The count name is required for accessing dataset names.
        
        // Get dataset names for the datasets that provides the 
        // starting index. (indexBeg) in the target (segmented
        // trajectory, photon) group and the number of elements
        // (photons, segmentPnCnt) in the segment that follow in
        // sequence from this start index.
        Configuration::getInstance()
            -> getDatasetNames(shortname, groupname, indexBegName, countName);

        if (!indexBegName.empty() 
              && H5Lexists(segGroup.getLocId(), indexBegName.c_str(), H5P_DEFAULT) > 0)
        {
            indexBegSet = new H5::DataSet(segGroup.openDataSet(indexBegName));
        }
        else
        {
            // if fbswath_lead_ndx_gt<1l,1r....> doesn't exist, 
            // try fbswath_lead_ndx_gt<1...6> for ATL10
            Configuration::getInstance()
                -> groundTrackRename(groupname, indexBegName, countName);
            indexBegSet = new H5::DataSet(segGroup.openDataSet(indexBegName));
        }

        // Compute the index-selection object for this coordinates - do the subset
        segmentedTrajectorySubset(indexBegSet);
                
        indexesProcessed = true;
        
        return indexes;
    }
    
private:
    
    H5::Group segGroup;
    std::string shortname;
    
   /**
    * Scan forwards, from segStart to segEnd in indexBeg array
    * for a non-fill value. 
    * 
    * @param segStartIdx    The start index of the input segment.
    * @param segEndIdx      The end index of the input segment.
    * @param found          The trajectory value with the first non-fill
    *                       indexBeg value.
    * @param foundIdx       The first non-fill indexBeg index.
    * @param indexBegDatset The input indexBeg dataset array.
    */ 
    void scanFwdNonFill( long segStartIdx, long segEndIdx,
                         long &found,      long &foundIdx, 
                         int64_t indexBegDataset[] )
        // __attribute__ ((optnone)) // MacOS specific for debug
    {
        // skip over segment-begin (start) fill values
        for (long i = segStartIdx; i <= segEndIdx; i++)
        {
            if (indexBegDataset[i] > 0 )
            {
                foundIdx = i;
                found    = indexBegDataset[foundIdx];
                break;
            }
        }
    }

    /** Scan backwards, from segEnd to segStart in indexBeg array
    * for a non-fill value.
    *  
    * @param segStartIdx    The start index of the input segment.
    * @param segEndIdx      The end index of input segment.
    * @param found          The trajectory value with the first non-fill
    *                       indexBeg value.
    * @param foundIdx       The first non-fill indexBeg index.
    * @param indexBegDatset The input indexBeg dataset array.
    */ 
    void scanBackNonFill( long segEndIdx, long segStartIdx, 
                         long &found,    long &foundIdx, 
                         int64_t indexBegDataset[] ) 
        // __attribute__ ((optnone)) // MacOS specific for debug
    {
        // skip over segment-begin (start) fill values
        for (long i = segEndIdx; i >= segStartIdx; i--)
        {
            if (indexBegDataset[i] > 0 )
            {
                foundIdx = i;
                found    = indexBegDataset[foundIdx];
                break;
            }
        } 
    }

    /**
     * Find the begin index reference for the selected segment data 
     * (defined by selected segment's starting index and count). 
     * Note that the last segment in the selected set has a segment-begin
     * reference, but to find the length of that last segment we need to 
     * use the next segment-begin reference and subtract 1.
     *
     * @param selectedStartIdx The start index of the selected segment in the
     *                         index begin dataset.
     * @param selectedCount    The length of the selected segment in the
     *                         index begin dataset.
     * @param firstTrajSeg     The trajectory value associated with the first
     *                         non-fill selectedStartIdx value.
     *                         non-fill indexBeg value.
     * @param trajSegLength    The length of the trajectory segment.
     * @param maxIndexBegIdx   The final index of the entire indexBeg
     *                         dataset.
     * @param maxTrajSegRef    The final index of the entire trajectory
     *                         dataset.
     * @param indexBegDataset  The input indexBeg dataset array.
     */
    void defineOneSegment( long selectedStartIdx, long selectedCount, 
                           long &firstTrajSeg,    long &trajSegLength, 
                           long maxIndexBegIdx,   long maxTrajSegRef,
                           int64_t indexBegDataset[] )
    {


        long startIdxNonFill = 0;  // first non-fill indexBeg value in
                                   // selected segment

        // The end index of input segment.
        long lastSelectedIdx = selectedStartIdx + selectedCount - 1;
        
        // Step forwards to find first non-fill index begin segment.
        scanFwdNonFill( selectedStartIdx, lastSelectedIdx,
                        firstTrajSeg, startIdxNonFill, indexBegDataset );

        // Step backwards to find last non-fill index begin segment.
        long lastTrajSeg = 0;   // trajectory value with the first non-fill
                                // indexBeg value.
        long lastBegIdx = 0;   // last non-fill indexBeg index.
        scanBackNonFill( lastSelectedIdx, startIdxNonFill, 
                        lastTrajSeg, lastBegIdx, indexBegDataset );

        // if not found - skip this selected segment group
        if (lastTrajSeg <= 0) 
        {
            firstTrajSeg = 0;
            trajSegLength = 0;
            return;
        }

        // Need to find end of this segment. Unfortunately, in GEDI data, the count
        // dataset does not define the end of the full segment. It does not include
        // the padding that is present in the waveform (segmented trajectory) data.

        // Step forwards to find the next non-fill segment begin, using segment
        // group max index as last segment begin reference to look at.
        long nextBegIdx = 0;     // non-fill begin index after the last
                                 // selected index begin segment
        long nextTrajRef = 0;    // trajectory value associated with the 
                                 // index begin segment after the last
                                 // selected segment.
        scanFwdNonFill( lastBegIdx+1, maxIndexBegIdx, 
                        nextTrajRef, nextBegIdx, indexBegDataset );

        // If no segment is found after the last index begin segment:
        if (nextTrajRef <= 0)
        {
            nextTrajRef = maxTrajSegRef; // Segmented trajectory group/dataset size
        }

        long length_to_here = lastTrajSeg - firstTrajSeg - 1;
        long lastTrajSize = nextTrajRef - lastTrajSeg; 
        trajSegLength = length_to_here + lastTrajSize;
    }
    
    /*
     * Subset the segmented trajectory dataset - define the index 
     * selection sets, limiting the index range by indexBeg starting
     * value and last index of last selected segment.
     * 
     * We use (next non-fill indexBegin) to compute segment sizes.
     * 
     * @param indexBegSet: index begin dataset
     */
    void segmentedTrajectorySubset( H5::DataSet* indexBegSet ) 
    {
        std::cout << "SegmentedTrajectorySubset" << std::endl;
                
        size_t idxBegSize = indexBegSet->getSpace().getSimpleExtentNpoints();
        
        // Index begin datasets for ATL03 and ATL08 are 64-bit and 32-bit for ATL10.
        hid_t indexBeg_native_type 
                = H5Tget_native_type(H5Dget_type(indexBegSet->getId()), H5T_DIR_ASCEND);

        int64_t* indexBeg = new int64_t[idxBegSize];

        // Load index begin dataset.
        // Reading in the source data is data-type size specific:
        if (H5Tequal(indexBeg_native_type, H5T_NATIVE_LLONG)) // 64-bit int
        {
            indexBegSet->read(indexBeg, indexBegSet->getDataType());
        }
        else if(H5Tequal(indexBeg_native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            int32_t* data = new int32_t[idxBegSize];
            indexBegSet->read(data, indexBegSet->getDataType());
            for (int i = 0; i < idxBegSize; i++)
            {
                indexBeg[i] = data[i];
            }
            delete[] data;
        }
        else if(H5Tequal(indexBeg_native_type, H5T_NATIVE_ULLONG)) // unsigned 64-bit int
        {
            uint64_t* data = new uint64_t[idxBegSize];
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

                defineOneSegment(selectedStart, selectedCount, 
                                start, length, idxBegSize,
                                coordinateSize, indexBeg);

                // Note: index-selection start is true to datasets,
                // zero based indexing, whereas start index pulled from
                // indexBegin datasets is one based indexing one based
                // indexing.
                if (start > 0)
                {
                    indexes->addSegment(start-1, length);
                }
            }
            
            // If no spatial subsetting, include all segments.
            if (segIndexes->segments.empty())
            {
                long start = 0, length = 0;

                long selectedStart = segIndexes->minIndexStart;
                long selectedCount = segIndexes->maxIndexEnd - selectedStart - 1;

                defineOneSegment( selectedStart, selectedCount,
                                start, length, idxBegSize,
                                coordinateSize, indexBeg );

               // Note: index-selection start is true to datasets, zero based indexing, whereas
                // start index pulled from indexBegin datasets is one based indexing
                if (start > 0) 
                {
                    indexes->addSegment(start-1, length);
                }
            }
            
            // No data found matched the spatial/temporal constraints, return no data.
            if (indexes->segments.empty())
            {
                indexes->addRestriction(0, 0);
                std::cout << "No data found in trajectory subset that matched the spatial/temporal constraints.\n";
            } 
            
            delete [] indexBeg;
    }
};

 #endif

