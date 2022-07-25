#ifndef FWDREFBEGINDATASET_H
#define FWDREFBEGINDATASET_H

#include <stdlib.h>

using namespace std;

/**
 * class to write index begin datasets for ATL03 and ATL08
 */
class FwdRefBeginDataset
{
public:

    string shortname;
    string datasetName;

    FwdRefBeginDataset(string shortname, string objname)
    : shortname(shortname), datasetName(objname)
    {
    }
        
    /**
     * write photon index begin dataset
     * @param indataset DataSet the input dataset
     * @param outgroup Group the output group
     * @param groupname string group name
     * @param indexes IndexSelection index selection object  
     * @param subsetDataLayers SubsetDataLayers list of included dataset names   
     */
    void writeDataset(Group& outgroup, const string& groupname, const DataSet& indataset,IndexSelection* indexes, SubsetDataLayers* subsetDataLayers)
    {
        cout << "ForwardReferenceDatasets.writeDataset" << endl;

        // Get output coordinate data rate from output count dataset.
        string countName = Configuration::getInstance()->getCountDatasetName(shortname, groupname, datasetName);
        DataSet countOutDs = outgroup.openDataSet(countName);
        size_t outputCoordinateSize = countOutDs.getSpace().getSimpleExtentNpoints();

        // Read in the source and declare the output index begin dataset.
        hid_t indexBegin_native_type = H5Tget_native_type(H5Dget_type(indataset.getId()), H5T_DIR_ASCEND);
        size_t inputCoordinateSize = indataset.getSpace().getSimpleExtentNpoints();
        int64_t* indexBeginIn = new int64_t[inputCoordinateSize];
        int64_t* indexBeginOut = new int64_t[outputCoordinateSize];

        if (H5Tequal(indexBegin_native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            indataset.read(indexBeginIn, indataset.getDataType());
        }
        else if(H5Tequal(indexBegin_native_type, H5T_NATIVE_USHORT)) // unsigned 16-bit int
        {
            uint16_t* data = new uint16_t[inputCoordinateSize];
            indataset.read(data, indataset.getDataType());
            for (int i = 0; i < inputCoordinateSize; i++) indexBeginIn[i] = data[i];
            delete[] data;
        }
        else
        {
            int64_t* data = new int64_t[inputCoordinateSize];
            indataset.read(data, indataset.getDataType());
            for (int i = 0; i < inputCoordinateSize; i++) indexBeginIn[i] = data[i];
            delete[] data;
        }

        // A few things to take into account when updating the ph_index_beg:
        // (1) count datasets have _FillValue = 0
        // (2) index begin dataset have _FillValue = 0 for ATL03 and ATL08,
        //     and _FillValue = -1 for ATL10
        // when there are no corresponding photons
        // (3) index starts at 1

        /////// DAVID REVISION ///////

        int64_t previousPartialOffset = -1;
        // The offset between the source datasets index-begin values and the output index-begin
        // values is composed of an accumulation of previous offsets plus the current
        // source index-begin value, and starting at -1. (See offset variable following)

        int frSegmentStart_Out = 0;
        // Forward Reference Segment Start index to output dataset (starts at position 0)

        // For each segment group, write to the index begin output/subset dataset.
        for (map<long, long>::iterator it
                = indexes->segments.begin(); it != indexes->segments.end(); it++)
        {
            long frSegmentStart_In = it->first;
            long segmentsCountIn = it->second;

            int64_t offset
                    = previousPartialOffset + indexBeginIn[frSegmentStart_In];
            // indexBeginIn here defines the end of the skipped values from the previous iteration.
            // Accumulating offset = offset + recently-skipped-value-count
            // recently-skipped-value-count = skipped-end-point - skipped-start
            // previousPartialOffsest = offset - skipped-start

            // Write segment to output index begin dataset.
            for (int i = 0; i < segmentsCountIn; i++)
            {
                indexBeginOut[frSegmentStart_Out + i] = indexBeginIn[frSegmentStart_In + i] - offset;
            }

            // Recalculate offset.
            frSegmentStart_Out = frSegmentStart_Out + segmentsCountIn;
            // Update the output starting index for the next segment, where to start writing to.

            int64_t nextIndexBeginIn
                    = indexBeginIn[frSegmentStart_In + segmentsCountIn];

            previousPartialOffset = offset - nextIndexBeginIn;
            // nextIndexBeginIn here defines the start of the skipped values in the target
            // (segmented) dataset.
        }

        //////////////////////////////

        /////// JOSIE REVISION ///////
        /*
        int64_t offset = 1;                 // The first segment will begin at the zeroth position in the output index begin dataset.
                                            // However, the index begin dataset starts at 1, not zero, so the initial offset is 1.
        int frSegmentStartIdx_Out = 0;      // Forward Reference Segment Start Index for index begin output dataset.

        // For each segment group, write to the index begin output/subset dataset.
        for (map<long, long>::iterator it = indexes->segments.begin(); it != indexes->segments.end(); it++)
        {
            long frSegmentStartIdx_In = it->first;
            long segmentLengthIn = it->second;

            // Write segment to output index begin dataset.
            for (int i = 0; i < segmentLengthIn; i++)
            {
                indexBeginOut[frSegmentStartIdx_Out + i] = indexBeginIn[frSegmentStartIdx_In + i] - indexBeginIn[frSegmentStartIdx_In] - offset;
            }

            // If there are multiple segments, we'll need this data to calculate the index begin output values,
            // including the offset, for the following segment.
            if (indexes->segments.size() > 1)
            {
                // Re-calculate the offset for following segment, which is offset by the value associated with the last
                // index in the last segment. To find this offset, we must find this value's length.
                // *** Note: We cannot just use the count dataset to do this since the count dataset only considers
                // non-padded data, whereas some data collections (e.g., GEDI) include zero padding in each segment.
                // Instead, we can just calculate the length of that last index begin value by considering it's
                // difference from what the next index begin value is in the source dataset.
                // Then we add that difference to the last written index begin output value.
                frSegmentStartIdx_Out = segmentLengthIn;   // Update the current index begin index so that the next segment knows where to start writing to.
                int64_t nextIndexBeginIn = indexBeginIn[frSegmentStartIdx_In + segmentLengthIn];
                int64_t previousIndexBeginIn = indexBeginIn[frSegmentStartIdx_In + segmentLengthIn - 1];
                int64_t previousIndexBeginOut = indexBeginOut[segmentLengthIn - 1];
                offset = offset + nextIndexBeginIn; // - previousIndexBeginIn + previousIndexBeginOut;
            }
        }
        */
        //////////////////////////////

        // Write the index begin dataset.
        int dimnum = indataset.getSpace().getSimpleExtentNdims();
        hsize_t olddims[dimnum], newdims[dimnum], maxdims[dimnum];
        indataset.getSpace().getSimpleExtentDims(olddims, maxdims);
        newdims[0] = outputCoordinateSize;
        for (int d = 1; d < dimnum; d++) newdims[d] = olddims[d];

        DataSpace outspace(dimnum, newdims, maxdims);
        DataType datatype(indataset.getDataType());
        DataSet outdataset(outgroup.createDataSet(datasetName, datatype, outspace, indataset.getCreatePlist()));
        outdataset.write(indexBeginOut, datatype, DataSpace::ALL, outspace);
        delete[] indexBeginOut;

        // Unlink the count dataset if user doesn't ask for it.
        if (!subsetDataLayers->is_dataset_included(groupname + countName))
        {
	        outgroup.unlink(countName);
	    }
    }
  
};

#endif
