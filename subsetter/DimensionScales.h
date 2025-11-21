#ifndef DimensionScales_H
#define DimensionScales_H

#include "H5Cpp.h"
#include "H5DSpublic.h"
#include "LogLevel.h"
#include <stdlib.h>
#include <string.h>
#include <vector>

/**
 * This class handles dimension scales. It provides functionalities to
 * keep track of the dimension scales for each dataset which will be included,
 * and recreates and re-attaches dimension scales in output.
 */
class DimensionScales
{
  public:
    DimensionScales()
    {
        dimScaleDatasets = new std::vector<std::string>();
        datasetScales =
            new std::map<std::string,
                         std::vector<std::pair<std::string, int>> *>();
    }

    ~DimensionScales()
    {
        delete dimScaleDatasets;
        for (std::map<std::string,
                      std::vector<std::pair<std::string, int>> *>::iterator it =
                 datasetScales->begin();
             it != datasetScales->end();
             it++)
        {
            delete it->second;
            it->second = NULL;
        }
        delete datasetScales;
    }

    /**
     * callback function for iterate scales attached to dim of dataset did
     * @param did the dataset
     * @param idx the index in the visiting dimension of dataset did
     * @param dsid Dimension Scale dataset identifier
     * @param opdata arbitrary data to pass to the callback function
     * @return 0 to continue
     */
    static herr_t dimensionScaleCallback(hid_t did,
                                         unsigned idx,
                                         hid_t dsid,
                                         void *opdata)
    {
        std::map<std::string, std::string> *scale =
            (std::map<std::string, std::string> *)opdata;
        std::string datasetName = getObjectName(did);
        std::string scaleDatasetName = getObjectName(dsid);
        scale->insert(
            std::pair<std::string, std::string>(datasetName, scaleDatasetName));
        return 1;
    }

    /**
     * keep track of dimension scales for each dataset,
     * it should be called for each dataset which will be included
     */
    void trackDimensionScales(const H5::DataSet &dataset)
    {
        LOG_DEBUG("DimensionScales::trackDimensionScales(): ENTER");

        hid_t did = dataset.getId();
        std::string datasetName = getObjectName(did);
        // is a dimension scale dataset
        if (H5DSis_scale(did))
        {
            dimScaleDatasets->push_back(datasetName);
        }
        else // dataset scales
        {
            int dimnum = dataset.getSpace().getSimpleExtentNdims();
            // Iterates the callback function through the scales attached to
            // each dimension
            for (int i = 0; i < dimnum; i++)
            {
                int numscales = H5DSget_num_scales(did, i);
                // some dimension scale references in DIMENSION_LIST may not be
                // valid
                int numValidScales = 0;
                // scales attached to dimension i
                for (int j = 0; j < numscales; j++)
                {
                    std::map<std::string, std::string> *scale =
                        new std::map<std::string, std::string>();
                    H5DSiterate_scales(did,
                                       i,
                                       &j,
                                       DimensionScales::dimensionScaleCallback,
                                       scale);
                    if (!scale->empty())
                    {
                        std::string datasetName = scale->begin()->first;
                        std::string scaleDatasetName = scale->begin()->second;
                        std::pair<std::string, int> scaleDim(scaleDatasetName,
                                                             i);
                        if (datasetScales->find(datasetName) ==
                            datasetScales->end())
                        {
                            std::vector<
                                std::pair<std::string, int>> *scaleDims =
                                new std::vector<std::pair<std::string, int>>();
                            datasetScales->insert(
                                std::pair<
                                    std::string,
                                    std::vector<std::pair<std::string, int>> *>(
                                    datasetName, scaleDims));
                        }
                        datasetScales->at(datasetName)->push_back(scaleDim);
                        numValidScales++;
                    }
                    delete scale;
                }
                if (numscales != numValidScales)
                    LOG_DEBUG("DimensionScales::trackDimensionScales(): "
                              "WARNING dataset "
                              << datasetName << " has "
                              << numscales - numValidScales
                              << " invalid scale references in the dimension "
                              << dimnum);
            }
        }
    }

    /**
     * recreate dimension scales and re-attach scales in output
     */
    void recreateDimensionScales(H5::H5File &outfile)
    {
        LOG_DEBUG(
            "DimensionScales::recreateDimensionScales(): ENTER try to recreate "
            << dimScaleDatasets->size()
            << " dimension scales, and re-attach to " << datasetScales->size()
            << " datasets ");

        // H5Lexists prints error message when any element before the final link
        // doesn't exist, so suppress the error message print
        H5E_auto2_t func;
        void *client_data;
        H5::Exception::getAutoPrint(func, &client_data);
        H5::Exception::dontPrint();

        // recreate the dimension scales,
        // save the pointer of the dimension scale datasets for later attaching,
        // the dataset id is valid only when the dataset is open
        std::map<std::string, H5::DataSet *> *scaleDatasetsMap =
            new std::map<std::string, H5::DataSet *>();
        for (std::vector<std::string>::iterator it = dimScaleDatasets->begin();
             it != dimScaleDatasets->end();
             it++)
        {
            std::string ds = *it;
            // if the dataset exists in the file, make it dimension scale
            if (H5Lexists(outfile.getLocId(), ds.c_str(), H5P_DEFAULT) > 0)
            {
                H5::DataSet *dataset = new H5::DataSet(outfile.openDataSet(ds));
                // remove CLASS attribute, set_scale will set CLASS to
                // "DIMENSION_SCALE" and an empty REFERENCE_LIST attribute name
                // and label should have already been copied as attributes
                dataset->removeAttr("CLASS");
                hid_t dsid = dataset->getId();
                if (H5DSis_scale(dsid) || H5DSset_scale(dsid, NULL) >= 0)
                {
                    scaleDatasetsMap->insert(
                        std::pair<std::string, H5::DataSet *>(ds, dataset));
                }
            }
        }

        // re-attach scales
        for (std::map<std::string,
                      std::vector<std::pair<std::string, int>> *>::iterator it =
                 datasetScales->begin();
             it != datasetScales->end();
             it++)
        {
            std::string d = it->first;
            std::vector<std::pair<std::string, int>> *scales = it->second;
            // if the dataset exists in the file, attach scales to it
            if (H5Lexists(outfile.getLocId(), d.c_str(), H5P_DEFAULT) > 0)
            {
                H5::DataSet dd = outfile.openDataSet(d);
                hid_t did = dd.getId();
                for (std::vector<std::pair<std::string, int>>::iterator sit =
                         scales->begin();
                     sit != scales->end();
                     sit++)
                {
                    std::string ds = sit->first;
                    int dim = sit->second;
                    if (scaleDatasetsMap->find(ds) != scaleDatasetsMap->end())
                    {
                        hid_t dsid =
                            scaleDatasetsMap->find(ds)->second->getId();
                        if (H5DSattach_scale(did, dsid, dim) >= 0)
                        {
                        }
                    }
                }
            }
        }

        // restore error message
        H5::Exception::setAutoPrint(func, client_data);

        // close the datasets
        for (std::map<std::string, H5::DataSet *>::iterator it =
                 scaleDatasetsMap->begin();
             it != scaleDatasetsMap->end();
             it++)
        {
            delete it->second;
            it->second = NULL;
        }
        delete scaleDatasetsMap;
    }

    std::vector<std::string> *getDimScaleDatasets()
    {
        return this->dimScaleDatasets;
    }

  private:
    // get the object name by id
    static std::string getObjectName(hid_t objId)
    {
        std::string name;
        size_t len = H5Iget_name(objId, NULL, 0);
        if (len > 0)
        {
            char buffer[len + 1];
            H5Iget_name(objId, buffer, len + 1);
            name = buffer;
        }
        return name;
    }

    /**
     * keep map of dimension scale dataset names
     */
    std::vector<std::string> *dimScaleDatasets;

    /**
     * keep track of the scales for datasets
     * key: dataset name
     * value: scales (dim scale dataset name, dimension idx of dataset) attached
     * to dataset
     */
    std::map<std::string, std::vector<std::pair<std::string, int>> *>
        *datasetScales;
};
#endif
