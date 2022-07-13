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

        // For each segment group, write to the index begin output/subset dataset.
        for (map<long, long>::iterator it = indexes->segments.begin(); it != indexes->segments.end(); it++)
        {
            long segmentStart = it->first;
            long segmentLength = it->second;

            // For each segment, construct the index begin subset dataset by subtracting the first index begin value
            // in each segment from each consecutive value in the source/input index begin dataset.
            // One is added to each index begin subset/output value to account for the index begin datasets starting
            // at one and not zero.
            for (int i = 0; i < segmentLength; i++)
            {
                indexBeginOut[i] = indexBeginIn[segmentStart + i] - indexBeginIn[segmentStart] + 1;
            }
        }

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
