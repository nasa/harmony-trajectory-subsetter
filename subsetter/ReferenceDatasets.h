#ifndef REFERENCEDATASETS_H
#define REFERENCEDATASETS_H

#include <stdlib.h>

using namespace std;


/**
 * class to write index begin datasets for ATL10
 */
class ReferenceDatasets
{
public:
    
    string shortname;
    string datasetName;

    ReferenceDatasets(string shortname, string objname)
    : shortname(shortname), datasetName(objname)
    {
    }
    
    /**
     * compute new index begin values and write it
     * @param objname string dataset object name
     * @param indataset DataSet the input dataset
     * @param outgroup Group the output group
     * @param groupname string group name
     * @param indexes IndexSelection index selection object for the current group
     * @param targetIndexes IndexSelection index selection for the referenced group
     *          (i.e., for /freeboard_swath_segment/fbswath_lead_ndx_gt1, 
     *           targetIndexes will be the IndexSelection from /gt1l/leads group)
     */
    void indexBeginMapping(Group& outgroup, const string&groupname, const DataSet& indataset, IndexSelection* indexes, IndexSelection* targetIndexes, SubsetDataLayers* subsetDataLayers)
    {
        //cout << "indexBeginMapping" << endl;

        size_t inDatasetSize = indataset.getSpace().getSimpleExtentNpoints(); // input dataset size
        size_t subsettedSize = indexes->size(); // subsetted dataset size
        int32_t* inData = new int32_t[inDatasetSize]; // array to store input dataset
        int32_t* indexBegin = new int32_t[subsettedSize]; // array to store subsetted index minIndexStart dataset
        int32_t* newIndexBegin = new int32_t[subsettedSize]; // array to store repaired index minIndexStart dataset
        bool diffIndex = false;
        
        indataset.read(inData, indataset.getDataType());
        int index = 0;
        // copy index minIndexStart from the input file according to indexes
        if (indexes->segments.empty())
        {
            for (int i = indexes->minIndexStart; i < indexes->maxIndexEnd; i++)
            {
                indexBegin[index] = inData[i];
                index++;
            }
        }
        else
        {
            for (map<long, long>::iterator it = indexes->segments.begin(); it != indexes->segments.end(); it++)
            {
                for (int i = it->first; i < (it->first+it->second); i++)
                {
                    indexBegin[index] = inData[i];
                    index++;
                }
            }
        }
        
        // location - new index minIndexStart value
        // offset - difference between new index minIndexStart and previous new index minIndexStart
        // prevLocation - previous new index minIndexStart
        // count - keeps track of how many indices in the (start, length) have been accounted for
        // prevOldLocation - previous index minIndexStart
        int32_t location = 1, offset = 0, prevLocation = 1, count = 0, prevOldLocation = 0;
        
        long start = 0, length = 0, end = 0, prevLength = 0;
       
        // if only temporal constraint is specified
        if (targetIndexes->segments.empty())
        {
            start = (targetIndexes->minIndexStart) + 1;
            end = targetIndexes->maxIndexEnd;
            length = end - start;
        }
        
        map<long, long>::iterator startIter = targetIndexes->segments.begin();
        
        // walk through the subsetted index minIndexStart and targetIndexes
        for (int i = 0; i < subsettedSize; i++)
        {
            // if indexBegin is 0 or -1, copy over
            if (indexBegin[i] == 0 || indexBegin[i] == -1) 
            {
                newIndexBegin[i] = indexBegin[i];
                continue;
            }

            // if spatial contraint is specified
            if (!targetIndexes->segments.empty())
            {
                for (map<long, long>::iterator it = startIter; it != targetIndexes->segments.end(); it++)
                {
                    if (indexBegin[i] > it->first && indexBegin[i] <= (it->first + it->second+1))
                    {
                        if (start != (it->first)+1)
                        {
                            startIter = it;
                            start = (it->first)+1;
                            length = it->second;
                            end = start + length;
                            it--;
                            prevLength = it->second;
                            diffIndex = true;
                        }
                        else diffIndex = false;
                        break;
                    }
                }
            }

            // first index minIndexStart
            // calcualte offset if first index minIndexStart does not equal to start of the (start, length) pair
            // in targetIndexes
            if (prevOldLocation == 0 && startIter == targetIndexes->segments.begin())
            {
                location = indexBegin[i] - (start-1);
                if (indexBegin[i] != start) offset = indexBegin[i] - start;
            }
            // anything other than first index minIndexStart
            // offset = index minIndexStart - previous index minIndexStart
            // new index minIndexStart = previous new index minIndexStart + offset
            else
            {
                offset = indexBegin[i] - prevOldLocation;
                location = prevLocation + offset;
                // if the index minIndexStart is in a different (start, length) pair than the previous index minIndexStart
                // offset = index minIndexStart - start of the (start, length)
                // new index minIndexStart = previous new index minIndexStart + (previous length of the (start, length) - count) + offset
                // reset count 
                if (diffIndex) 
                {
                    offset = indexBegin[i] - start;
                    location = prevLocation + (prevLength - count) + offset;
                    count = 0;
                }
            }
            newIndexBegin[i] = location;
            count += offset; 

            prevLocation = location;
            prevOldLocation = indexBegin[i];
        }
        
         // write the index minIndexStart dataset
        int dimnum = indataset.getSpace().getSimpleExtentNdims();
        hsize_t olddims[dimnum], newdims[dimnum], maxdims[dimnum];
        indataset.getSpace().getSimpleExtentDims(olddims, maxdims);
        newdims[0] = subsettedSize;
        for (int d = 1; d < dimnum; d++) newdims[d] = olddims[d];

        DataSpace outspace(dimnum, newdims, maxdims);
        DataType datatype(indataset.getDataType());
        DataSet outdataset(outgroup.createDataSet(datasetName, datatype, outspace, indataset.getCreatePlist()));
        outdataset.write(newIndexBegin, datatype, DataSpace::ALL, outspace);
        
        delete [] indexBegin;
        delete [] inData;
        delete [] newIndexBegin;
        
    }
};

#endif
