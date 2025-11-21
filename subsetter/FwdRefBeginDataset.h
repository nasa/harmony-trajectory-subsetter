#ifndef FWDREFBEGINDATASET_H
#define FWDREFBEGINDATASET_H

#include "Configuration.h"
#include "H5Cpp.h"
#include "IndexSelection.h"
#include "LogLevel.h"
#include "SubsetDataLayers.h"
#include <stdlib.h>

/**
 * class to write forward reference segment-begin datasets for ATL03 and ATL08
 * and recalculate after subsetting the segments of the underlying trajectory
 */
class FwdRefBeginDataset
{
  public:
    std::string shortname;
    std::string datasetName;
    Configuration *config;

    // Initialize with collection shortname and forward-reference begin dataset
    // name
    FwdRefBeginDataset(std::string shortname,
                       std::string objname,
                       Configuration *config)
        : shortname(shortname), datasetName(objname), config(config)
    {
    }

    /**
     * Write segment begin dataset ()
     * @param outgroup         Group   - the output group
     * @param groupname        String  - group name
     * @param indataset        DataSet - the input dataset
     * @param selectedElements SubsetList - Index Selection object
     * @param subsetDataLayers SubsetDataLayers - list of included dataset names
     */
    void writeDataset(H5::Group &outgroup,
                      const std::string &groupname,
                      const H5::DataSet &indataset,
                      IndexSelection *selectedElements,
                      SubsetDataLayers *subsetDataLayers)
    {
        LOG_DEBUG("ForwardReferenceDatasets::writeDataset(): ENTER groupname: "
                  << groupname);

        // Get output dataset data size from output count dataset.
        // Dataset size for fwd-ref-begin dataset should match the fwd-ref-count
        // dataset
        std::string countName =
            config->getCountDatasetName(shortname, groupname, datasetName);
        H5::DataSet countOutDS = outgroup.openDataSet(countName);
        size_t outputCoordinateSize =
            countOutDS.getSpace().getSimpleExtentNpoints();

        // Read in the source and declare the output Segment Begin dataset.
        hid_t segmentBegin_native_type =
            H5Tget_native_type(H5Dget_type(indataset.getId()), H5T_DIR_ASCEND);
        size_t inputCoordinateSize =
            indataset.getSpace().getSimpleExtentNpoints();
        int64_t *subsetBeginIn = new int64_t[inputCoordinateSize];
        int64_t *subsetBeginOut = new int64_t[outputCoordinateSize];

        // Reading in the source data is data-type size specific:
        if (H5Tequal(segmentBegin_native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            indataset.read(subsetBeginIn, indataset.getDataType());
        }
        else if (H5Tequal(segmentBegin_native_type,
                          H5T_NATIVE_USHORT)) // unsigned 16-bit int
        {
            uint16_t *data = new uint16_t[inputCoordinateSize];
            indataset.read(data, indataset.getDataType());
            for (int i = 0; i < inputCoordinateSize; i++)
                subsetBeginIn[i] = data[i];
            delete[] data;
        }
        else // 64-bit int
        {
            int64_t *data = new int64_t[inputCoordinateSize];
            indataset.read(data, indataset.getDataType());
            for (int i = 0; i < inputCoordinateSize; i++)
                subsetBeginIn[i] = data[i];
            delete[] data;
        }

        // A few things to take into account when updating the Segment Begin
        // values: (1) Segment Count (size) datasets have _FillValue = 0 (2)
        // Segment Begin datasets have _FillValue = 0 for ATL03 and ATL08,
        //     and _FillValue = -1 for ATL10
        //     when there are no corresponding photons
        // (3) Segment Begin values start at 1 (while most array indexes start
        // at 0)

        // Writing the segment begin datasets requires copying the selected
        // values to the output, minus an offset.  The offset reflects the
        // extent of previous values in the target segmented datasets that have
        // not been written, thus shifting the forward reference begin index to
        // an earlier value.  E.g., we know the first segment will begin at the
        // first position in the output segment begin datatset
        // - there are no preceding data items in the subsetted target dataset.
        // The offset in this case is the source forward-reference-begin value
        // for this first selected segment. Subtracting the offset yields zero
        // for the output value, + 1 since index values start at one.

        int64_t offset = 0;

        // The start of the skipped values in the target (segmented) dataset.
        int64_t nextSegmentBeginIn = 1;

        // The end of the skipped values in the target (segmented) dataset.
        int64_t skippedFillValueEnd = 1;
        int64_t lstSegBegIndex = 0;

        // Forward Reference Segment Begin index to output dataset (starts at
        // position 0).
        int64_t segmentBeginOut = 0;

        // For each selected element group, write to the segment begin
        // output/subset dataset.
        std::map<long, long> allSegments = selectedElements->getSegments();
        for (std::map<long, long>::iterator it = allSegments.begin();
             it != allSegments.end();
             it++)
        {

            long segmentBeginIn = it->first;
            long segmentCountIn = it->second;

            // Prepare for offset update, stepping over fill values.
            while (nextSegmentBeginIn <= 0 and
                   lstSegBegIndex < segmentBeginIn + segmentCountIn)
            {
                lstSegBegIndex++;
                nextSegmentBeginIn = subsetBeginIn[lstSegBegIndex];
            }

            // Find skipped segments end, stepping over fill values.
            int skippedFillValueCnt = 0;
            skippedFillValueEnd = subsetBeginIn[segmentBeginIn];
            while (skippedFillValueEnd <= 0 and
                   skippedFillValueCnt < segmentCountIn - 1)
            {
                subsetBeginOut[segmentBeginOut] = skippedFillValueEnd;
                // 0 for ATL3, ATL8, -1 for ATL10
                skippedFillValueCnt++;
                segmentBeginIn++;
                segmentBeginOut++;
                skippedFillValueEnd = subsetBeginIn[segmentBeginIn];
            }

            // skippedFillValueEnd here defines the end of the skipped values
            // from the previous iteration. Accumulating offset = offset +
            // skipped-values-count skipped-values-count = skipped-end-point -
            // skipped-start skipped-start      = (previous) nextSegmentBeginIn
            if (nextSegmentBeginIn > 0 and skippedFillValueEnd > 0)
            {
                offset = offset + skippedFillValueEnd - nextSegmentBeginIn;
            }

            // Calculate segment references for output segment begin dataset.
            for (int i = skippedFillValueCnt; i < segmentCountIn; i++)
            {
                if (subsetBeginIn[segmentBeginIn] <= 0)
                {
                    subsetBeginOut[segmentBeginOut] =
                        subsetBeginIn[segmentBeginIn];
                    // 0 for ATL3, ATL8, -1 for ATL10
                }
                else
                {
                    subsetBeginOut[segmentBeginOut] =
                        subsetBeginIn[segmentBeginIn] - offset;

                    // Prep to recalculate offset, in case this is the last
                    // non-zero case in selected group.
                    nextSegmentBeginIn = subsetBeginIn[segmentBeginIn + 1];
                    lstSegBegIndex = segmentBeginIn;
                }
                segmentBeginIn++;
                segmentBeginOut++;
            }
        }

        // Write the index begin dataset.
        int dimnum = indataset.getSpace().getSimpleExtentNdims();
        hsize_t olddims[dimnum], newdims[dimnum], maxdims[dimnum];
        indataset.getSpace().getSimpleExtentDims(olddims, maxdims);
        newdims[0] = outputCoordinateSize;
        for (int d = 1; d < dimnum; d++)
            newdims[d] = olddims[d];

        H5::DataSpace outspace(dimnum, newdims, maxdims);
        H5::DataType datatype(indataset.getDataType());
        H5::DataSet outdataset(outgroup.createDataSet(
            datasetName, datatype, outspace, indataset.getCreatePlist()));
        outdataset.write(
            subsetBeginOut, datatype, H5::DataSpace::ALL, outspace);
        delete[] subsetBeginIn;
        delete[] subsetBeginOut;

        // Unlink the count dataset if user doesn't ask for it.
        if (!subsetDataLayers->is_dataset_included(groupname + countName))
        {
            outgroup.unlink(countName);
        }
    }
};

#endif
