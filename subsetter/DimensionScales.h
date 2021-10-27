#ifndef DimensionScales_H
#define DimensionScales_H 

#include "H5DSpublic.h"
#include "H5Cpp.h"
#include <string.h>
#include <stdlib.h>
#include <vector>
using namespace std;

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
        dimScaleDatasets = new vector<string>();
        datasetScales = new map<string, vector<pair<string, int>>*>();
    }

    ~DimensionScales()
    {
        delete dimScaleDatasets;
        for (map<string, vector<pair < string, int>>*>::iterator it = datasetScales->begin();
             it != datasetScales->end(); it++)
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
    static herr_t dimensionScaleCallback(hid_t did, unsigned idx, hid_t dsid, void *opdata)
    {   
        map<string,string>* scale = (map<string,string>*)opdata;
        string datasetName = getObjectName(did);
        string scaleDatasetName = getObjectName(dsid);
        //cout << datasetName << " has scale " << scaleDatasetName << " at index " << idx << endl;
        scale->insert(pair<string,string>(datasetName, scaleDatasetName));        
        return 1;
    }

    /**
     * keep track of dimension scales for each dataset,
     * it should be called for each dataset which will be included
     */
    void trackDimensionScales(const DataSet& dataset)
    {
        hid_t did = dataset.getId();
        string datasetName = getObjectName(did); 
        // is a dimension scale dataset
        if (H5DSis_scale(did))
        {
            dimScaleDatasets->push_back(datasetName);
        }
        else // dataset scales
        {
            int dimnum = dataset.getSpace().getSimpleExtentNdims();
            // Iterates the callback function through the scales attached to each dimension
            for (int i = 0; i < dimnum; i++)
            {
                int numscales = H5DSget_num_scales(did,i);
                // some dimension scale references in DIMENSION_LIST may not be valid
                int numValidScales = 0;
                // scales attached to dimension i
                for (int j = 0; j < numscales; j++)
                {                    
                    map<string,string>* scale = new map<string,string>();                    
                    H5DSiterate_scales(did, i, &j, DimensionScales::dimensionScaleCallback, scale);
                    if (!scale->empty())
                    {   
                        string datasetName = scale->begin()->first;
                        string scaleDatasetName = scale->begin()->second;                    
                        pair<string, int> scaleDim(scaleDatasetName, i);
                        //cout << datasetName << " with scale " << scaleDatasetName << " at dim " << i << endl;
                        if (datasetScales->find(datasetName) == datasetScales->end())
                        {
                            vector<pair<string, int>>*scaleDims = new vector<pair<string, int>>();
                            datasetScales->insert(pair<string, vector<pair<string, int>>*>(datasetName, scaleDims));
                        }
                        datasetScales->at(datasetName)->push_back(scaleDim);
                        numValidScales++;
                    }
                    delete scale;
                }
                if (numscales != numValidScales)
                    cout << "WARNING dataset " << datasetName << " has " << numscales-numValidScales
                         << " invalid scale references in the dimension " << dimnum << endl; 
            }                
        }
    }

    /**
     * recreate dimension scales and re-attach scales in output
     */
    void recreateDimensionScales(H5File& outfile)
    {
        cout << "DimensionScales.recreateDimensionScales try to recreate "
             << dimScaleDatasets->size() << " dimension scales, and re-attach to "
             << datasetScales->size() << " datasets " << endl;

        // H5Lexists prints error message when any element before the final link doesn't exist,
        // so suppress the error message print
        // save off error handler
        H5E_auto2_t func;
        void* client_data;
        Exception::getAutoPrint(func, &client_data);
        Exception::dontPrint();

        // recreate the dimension scales,
        // save the pointer of the dimension scale datasets for later attaching,
        // the dataset id is valid only when the dataset is open
        map<string, DataSet*>* scaleDatasetsMap = new map<string, DataSet*>();
        for (vector<string>::iterator it = dimScaleDatasets->begin(); it != dimScaleDatasets->end(); it++)
        {
            string ds = *it;
            // if the dataset exists in the file, make it dimension scale
            if (H5Lexists(outfile.getLocId(), ds.c_str(), H5P_DEFAULT) > 0)
            {
                DataSet* dataset = new DataSet(outfile.openDataSet(ds));
                // remove CLASS attribute, set_scale will set CLASS to "DIMENSION_SCALE" 
                // and an empty REFERENCE_LIST attribute
                // name and label should have already been copied as attributes
                dataset->removeAttr("CLASS");
                hid_t dsid = dataset->getId();
                if (H5DSis_scale(dsid) || H5DSset_scale(dsid, NULL) >= 0)
                {
                    //cout << " set dataset to dimension scale " << ds << endl;
                    scaleDatasetsMap->insert(pair<string, DataSet*>(ds, dataset));
                }
            }
        }

        // re-attach scales        
        for (map<string, vector<pair < string, int>>*>::iterator it = datasetScales->begin();
             it != datasetScales->end(); it++)
        {
            string d = it->first;
            vector<pair<string, int>>*scales = it->second;
            // if the dataset exists in the file, attach scales to it
            if (H5Lexists(outfile.getLocId(), d.c_str(), H5P_DEFAULT) > 0)
            {
                //cout << "dataset exists " << d << endl;
                DataSet dd = outfile.openDataSet(d);                
                hid_t did = dd.getId();                
                for (vector<pair < string, int>>::iterator sit = scales->begin(); sit != scales->end(); sit++)
                {
                    string ds = sit->first;
                    int dim = sit->second;
                    if (scaleDatasetsMap->find(ds) != scaleDatasetsMap->end())
                    {
                        hid_t dsid = scaleDatasetsMap->find(ds)->second->getId();
                        if (H5DSattach_scale(did, dsid, dim) >= 0)
                        {
                            //cout << " attach " << ds << " to " << d << " at " << dim << endl;
                        }
                    }
                }
            }
        }

        // restore error message
        Exception::setAutoPrint(func,client_data);

        // close the datasets
        for (map<string, DataSet*>::iterator it = scaleDatasetsMap->begin();
             it != scaleDatasetsMap->end(); it++)
        {
            delete it->second;
            it->second = NULL;
        }        
        delete scaleDatasetsMap;
    }
    
    vector<string>* getDimScaleDatasets() { return this->dimScaleDatasets; }

private:

    // get the object name by id
    static string getObjectName(hid_t objId)
    {
        string name;
        size_t len = H5Iget_name(objId, NULL, 0);
        if (len > 0)
        {
            char buffer[len];
            H5Iget_name(objId, buffer, len + 1);
            name = buffer;
        }
        return name;
    }

    /**
     * keep map of dimension scale dataset names     
     */
    vector<string>* dimScaleDatasets;

    /**
     * keep track of the scales for datasets
     * key: dataset name
     * value: scales (dim scale dataset name, dimension idx of dataset) attached to dataset
     */
    map<string, vector<pair<string, int>>*>* datasetScales;
};
#endif
