#ifndef PHOTONREFERENCEDATASETS_H
#define PHOTONREFERENCEDATASETS_H

#include <stdlib.h>

using namespace std;

/**
 * class to write index begin datasets for ATL03 and ATL08
 */
class PhotonReferenceDatasets
{
public:

    string shortname;
    string datasetName;

    PhotonReferenceDatasets(string shortname, string objname)
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
    void writeIndexBeginDataset(Group& outgroup, const string& groupname, const DataSet& indataset,IndexSelection* indexes, SubsetDataLayers* subsetDataLayers)
    {
        cout << "ForwardReferenceDatasets.writeIndexBeginDataset" << endl;
        string countName = Configuration::getInstance()->getCountDatasetName(shortname, groupname,datasetName);
        //cout << "groupname+countName: " << groupname << " + " << countName << endl;

     	// read the count in the output
     	DataSet countDs = outgroup.openDataSet(countName);
        size_t coordinateSize = countDs.getSpace().getSimpleExtentNpoints();
        hid_t count_native_type = H5Tget_native_type(H5Dget_type(countDs.getId()), H5T_DIR_ASCEND);
        int32_t* count = new int32_t[coordinateSize];
        int64_t* indexBegin = new int64_t[coordinateSize];
        
        if (H5Tequal(count_native_type, H5T_NATIVE_INT)) // 32-bit int
        {
            countDs.read(count, countDs.getDataType());
        }
        else if(H5Tequal(count_native_type, H5T_NATIVE_USHORT)) // unsigend 16-bit int
        {
            uint16_t* data = new uint16_t[coordinateSize];
            countDs.read(data, countDs.getDataType());
            for (int i = 0; i < coordinateSize; i++) count[i] = data[i];
            delete[] data;
        }
        else if(H5Tequal(count_native_type, H5T_NATIVE_LONG)) // 64-bit int
        {
            int64_t* data = new int64_t[coordinateSize];
            countDs.read(data, countDs.getDataType());
            for (int i = 0; i < coordinateSize; i++) count[i] = data[i];
            delete[] data;
        }

        // A few things to take into account when updating the ph_index_beg:
        // (1) count datasets have _FillValue = 0
        // (2) index begin dataset have _FillValue = 0 for ATL03 and ATL08,
        //     and _FillValue = -1 for ATL10
        // when there are no corresponding photons
        // (3) index starts at 1

        //keep track of current photon location, start with 1       
        int64_t location = 1;
        for (int i = 0; i < coordinateSize; i++)
        {
            indexBegin[i] = (count[i] == 0) ? 0 : location;
            location += count[i];
            //cout << indexBegin[i] << " " << count[i] << " " << location << endl;
        }

        // write the index begin dataset
        int dimnum = indataset.getSpace().getSimpleExtentNdims();
        hsize_t olddims[dimnum], newdims[dimnum], maxdims[dimnum];
        indataset.getSpace().getSimpleExtentDims(olddims, maxdims);
        newdims[0] = coordinateSize;
        for (int d = 1; d < dimnum; d++) newdims[d] = olddims[d];

        DataSpace outspace(dimnum, newdims, maxdims);
        DataType datatype(indataset.getDataType());
        DataSet outdataset(outgroup.createDataSet(datasetName, datatype, outspace, indataset.getCreatePlist()));
        outdataset.write(indexBegin, datatype, DataSpace::ALL, outspace);
        delete[] count;
        delete[] indexBegin;

        // unlink the count dataset if user doesn't ask for it
        if (!subsetDataLayers->is_dataset_included(groupname + countName))
        {
	    outgroup.unlink(countName);
	}
    }
  
};

#endif
