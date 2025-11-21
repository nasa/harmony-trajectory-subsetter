#ifndef RVSREFDATASETS_H
#define RVSREFDATASETS_H

#include "IndexSelection.h"
#include "LogLevel.h"
#include "SubsetDataLayers.h"
#include <stdlib.h>

#include "H5Cpp.h"

/**
 * class to write index datasets for ATL10
 */
class RvsRefDatasets
{
  public:
    std::string shortname;
    std::string datasetName;

    RvsRefDatasets(std::string shortname, std::string objname)
        : shortname(shortname), datasetName(objname)
    {
    }

    /**
     * compute new index values and write it
     * @param objname string dataset object name
     * @param indataset DataSet the input dataset
     * @param outgroup Group the output group
     * @param groupname string group name
     * @param indexes IndexSelection index selection object for the current
     * group
     * @param targetIndexes IndexSelection index selection for the referenced
     * group (i.e., for /freeboard_swath_segment/fbswath_lead_ndx_gt1,
     *           targetIndexes will be the IndexSelection from /gt1l/leads
     * group)
     */
    void mapWriteDataset(H5::Group &outgroup,
                         const std::string &groupname,
                         const H5::DataSet &indataset,
                         IndexSelection *indexes,
                         IndexSelection *targetIndexes,
                         SubsetDataLayers *subsetDataLayers)
    {
        LOG_DEBUG("RvsRefDatasets::mapWriteDataset(): ENTER groupname: "
                  << groupname);

        size_t inDatasetSize =
            indataset.getSpace().getSimpleExtentNpoints(); // input dataset size
        size_t subsettedSize = indexes->size(); // subsetted dataset size
        int32_t *inData =
            new int32_t[inDatasetSize]; // array to store input dataset
        int32_t *indexRef =
            new int32_t[subsettedSize]; // array to store subsetted
                                        // index reference dataset
        int32_t *newIndexRef =
            new int32_t[subsettedSize]; // array to store repaired index
                                        // reference dataset
        bool diffIndex = false;

        indataset.read(inData, indataset.getDataType());
        int index = 0;
        // copy index from the input file according to indexes
        if (indexes->segments.empty())
        {
            for (int i = indexes->minIndexStart; i < indexes->maxIndexEnd; i++)
            {
                indexRef[index] = inData[i];
                index++;
            }
        }
        else
        {
            for (std::map<long, long>::iterator it = indexes->segments.begin();
                 it != indexes->segments.end();
                 it++)
            {
                for (int i = it->first; i < (it->first + it->second); i++)
                {
                    indexRef[index] = inData[i];
                    index++;
                }
            }
        }

        // location - new index value
        // offset - difference between new index and previous new index
        // prevLocation - previous new index
        // count - keeps track of how many indices in the (start, length) have
        // been accounted for prevOldLocation - previous index
        int32_t location = 1, offset = 0, prevLocation = 1, count = 0,
                prevOldLocation = 0;

        long start = 0, length = 0, end = 0, prevLength = 0;

        // if only temporal constraint is specified
        if (targetIndexes->segments.empty())
        {
            start = (targetIndexes->minIndexStart) + 1;
            end = targetIndexes->maxIndexEnd;
            length = end - start;
        }

        std::map<long, long>::iterator startIter =
            targetIndexes->segments.begin();

        // walk through the subsetted index and targetIndexes
        for (int i = 0; i < subsettedSize; i++)
        {
            // if indexRef is 0 or -1, copy over
            if (indexRef[i] == 0 || indexRef[i] == -1)
            {
                newIndexRef[i] = indexRef[i];
                continue;
            }

            // if spatial constraint is specified
            if (!targetIndexes->segments.empty())
            {
                for (std::map<long, long>::iterator it = startIter;
                     it != targetIndexes->segments.end();
                     it++)
                {
                    if (indexRef[i] > it->first &&
                        indexRef[i] <= (it->first + it->second + 1))
                    {
                        if (start != (it->first) + 1)
                        {
                            startIter = it;
                            start = (it->first) + 1;
                            length = it->second;
                            end = start + length;

                            // Ensure that we do not decrement the iterator
                            // if it is at the beginning of
                            // targetIndexes->segments.begin()
                            if (it == targetIndexes->segments.begin())
                                continue;

                            it--;
                            prevLength = it->second;
                            diffIndex = true;
                        }
                        else
                            diffIndex = false;
                        break;
                    }
                }
            }

            // first index
            // calculate offset if first index does not equal to start of the
            // (start, length) pair in targetIndexes
            if (prevOldLocation == 0 &&
                startIter == targetIndexes->segments.begin())
            {
                location = indexRef[i] - (start - 1);
                if (indexRef[i] != start)
                    offset = indexRef[i] - start;
            }
            // anything other than first index
            // offset = index - previous index
            // new index = previous new index + offset
            else
            {
                offset = indexRef[i] - prevOldLocation;
                location = prevLocation + offset;
                // diffIndex - index is in a different (start, length) pair than
                // the previous index reference
                if (diffIndex)
                {
                    offset = indexRef[i] - start;
                    location = prevLocation + (prevLength - count) + offset;
                    count = 0;
                }
            }
            newIndexRef[i] = location;
            count += offset;

            prevLocation = location;
            prevOldLocation = indexRef[i];
        }

        // write the index dataset
        int dimnum = indataset.getSpace().getSimpleExtentNdims();
        hsize_t olddims[dimnum], newdims[dimnum], maxdims[dimnum];
        indataset.getSpace().getSimpleExtentDims(olddims, maxdims);
        newdims[0] = subsettedSize;
        for (int d = 1; d < dimnum; d++)
            newdims[d] = olddims[d];

        H5::DataSpace outspace(dimnum, newdims, maxdims);
        H5::DataType datatype(indataset.getDataType());
        H5::DataSet outdataset(outgroup.createDataSet(
            datasetName, datatype, outspace, indataset.getCreatePlist()));
        outdataset.write(newIndexRef, datatype, H5::DataSpace::ALL, outspace);

        delete[] indexRef;
        delete[] inData;
        delete[] newIndexRef;
    }
};

#endif
