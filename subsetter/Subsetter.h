#ifndef Subsetter_H 
#define Subsetter_H 

#include <algorithm>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "H5Cpp.h"
#include "hdf5_hl.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "Configuration.h"
#include "Coordinate.h"
#include "DatasetLinks.h"
#include "DimensionScales.h"
#include "IndexSelection.h"
#include "geobox.h"
#include "SubsetDataLayers.h"
#include "Temporal.h"
#include "ForwardReferenceCoordinates.h"
#include "HeightSegmentCoordinates.h"
#include "GeoPolygon.h"
#include "geotiff_converter.h"

using namespace std;
using namespace H5;
using namespace boost;

/**
 * This class subsets the HDF5 file 
 */
class Subsetter
{
public:
    
    Subsetter(SubsetDataLayers* subsetDataLayers, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon, string outputFormat="")
    : subsetDataLayers(subsetDataLayers), geoboxes(geoboxes), temporal(temporal), matchingDataFound(false), geoPolygon(geoPolygon),
            outputFormat(outputFormat)
    {
        dimensionScales = new DimensionScales();
    };

    ~Subsetter()
    {  
        delete dimensionScales;
    }

    /**
     * subsets
     * return code 0: successful, 3: no matching data
     * exception should be caught in calling function
     */
    int subset(string infilename, string outfilename)
    {
        int returnCode = 0;
        
        cout << "Opening " << infilename << endl;
        this->infile = H5File( infilename, H5F_ACC_RDONLY );
        cout << "Opening " << outfilename << endl;

        hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
        H5::FileAccPropList faplObj(fapl);
        this->outfile = H5File(outfilename, H5F_ACC_TRUNC, infile.getCreatePlist(), faplObj);
        H5Pclose(fapl);
        
        this->shortName = retrieveShortName(infile);
        // update epoch time if configured for this product
        string epochTime = Configuration::getInstance()->getProductEpoch(shortName);
        if (!epochTime.empty() && this->temporal != NULL)
        {
            this->temporal->updateReferenceTime(epochTime);
        }

        Group ingroup = this->infile.openGroup("/");
        Group outgroup = this->outfile.openGroup("/");

        // copy top level attributes
        copyAttributes(ingroup, outgroup, "/");

        // convert and write group and its datasets recursively 
        copyH5(ingroup, ingroup, outgroup, "/");

        // recreate and re-attach dimension scales in the output file
        dimensionScales->recreateDimensionScales(outfile);
        
        // check if the matching data found
        matchingDataFound = isMatchingDataFound();

        // adding warning attribute
        addProcessingParameterAttribute(outgroup, infilename);
        
        cout << "WRITING " << endl;
        outfile.flush(H5F_SCOPE_GLOBAL);
        cout << "datasets size: " << subsetDataLayers->getDatasets().size() << endl;
        if (!matchingDataFound)
        {
            returnCode = 3;
            if (remove(outfilename.c_str()))
                cout << "Error removing " << outfilename << " with no matching data in it." << endl;
            else
                cout << "Removed output file " << outfilename << " because there was no matching data for the constraints specified." << endl;    
        }
        
        if (outputFormat == "GeoTIFF")
        {
            cout << "Outputting to GeoTIFF" << endl;
            //createGeoTIFFOutput(outfilename);
            geotiff_converter geotiff = geotiff_converter(outfilename, shortName, outgroup, subsetDataLayers);
        }
        
        return returnCode;
    }
    
    /* gets short name from the metadata
     */
    virtual string retrieveShortName(const H5File& file)
    {
        cout << "retrieveShortName" << endl;
        H5std_string str;
        string shortnameFullPath, shortnamePath, shortnameLabel;
        Group root = file.openGroup("/");
        vector<string> shortnamePaths = Configuration::getInstance()->getShortnamePath();
        size_t found;
        for (vector<string>::iterator it = shortnamePaths.begin(); it != shortnamePaths.end(); it++)
        {
            shortnameFullPath = *it;
            cout << shortnameFullPath << endl;
            // group
            // "/" is used in the config file to determine whether the short name attribute is under a 
            // group or dataset
            // short name attribute path with "/" at the end indicates it is a group path
            // otherwise, it is a dataset path
            if (shortnameFullPath.compare(shortnameFullPath.length()-1, 1, "/") == 0)
            {
                shortnameFullPath = shortnameFullPath.substr(0, shortnameFullPath.size()-1);
                found = shortnameFullPath.find_last_of("/");
                shortnamePath = shortnameFullPath.substr(0, found+1);
                shortnameLabel = shortnameFullPath.substr(found+1);
                if (H5Lexists(root.getLocId(), shortnamePath.c_str(), H5P_DEFAULT) > 0 && H5Gopen1(root.getLocId(), shortnamePath.c_str()) > 0)
                {
                    Group group = root.openGroup(shortnamePath);
                    if (H5Aexists(group.getLocId(), shortnameLabel.c_str()) > 0)
                    {
                        Attribute attr = group.openAttribute(shortnameLabel);
                        attr.read(attr.getDataType(), str);
                        break;
                    }
                }
                else if(shortnamePath.size() == 1)
                {
                    if (H5Aexists(root.getLocId(), shortnameLabel.c_str()) > 0)
                    {
                        Attribute attr = root.openAttribute(shortnameLabel);
                        attr.read(attr.getDataType(), str);
                        break;
                    }
                }
            }
            // dataset
            else
            {
                if (H5Lexists(root.getLocId(), shortnamePath.c_str(), H5P_DEFAULT) > 0 && H5Dopen1(root.getLocId(), shortnamePath.c_str()) > 0)
                {
                    found = shortnameFullPath.find_last_of("/");
                    shortnamePath = shortnameFullPath.substr(0, found+1);
                    shortnameLabel = shortnameFullPath.substr(found+1);
                    DataSet data = root.openDataSet(shortnamePath);
                    if (H5Aexists(root.getLocId(), shortnameLabel.c_str()) > 0)
                    {
                        Attribute attr = data.openAttribute(shortnameLabel);
                        attr.read(attr.getDataType(), str);
                        break;
                    }
                }
            }
        }
        cout << "Processing file of type " << string(str) << endl;
        return string(str);
    }

protected:    

    H5File getInputFile() { return this->infile;}
    H5File getOutputFile() { return this->outfile;}
    SubsetDataLayers* getSubsetDataLayers() { return this->subsetDataLayers;}
    string getShortName() { return this->shortName;}
    vector<geobox>* getGeobox() { return this->geoboxes;}
    Temporal* getTemporal() { return this->temporal;}
    GeoPolygon* getGeoPolygon() { return this->geoPolygon; }
    
    /**
     * copy the attributes
     */
    void copyAttributes(const H5Object& inobj, H5Object& outobj, const string& groupname)
    {
        for (int k = 0; k < inobj.getNumAttrs(); k++)
        {
            Attribute attr = inobj.openAttribute(k);
            if (outobj.attrExists(attr.getName())) continue;
            DataType datatype = attr.getDataType();
            //cout << "attr name: " << attr.getName() << endl;
            // assume the attribute with datatype H5T_COMPOUND and H5T_VLEN are for dimension scales,
            // dimension scales are handled separately
            if (attr.getDataType().getClass() == H5T_COMPOUND)
            {
                if (attr.getName() != "REFERENCE_LIST")
                    cout << "Could not handle attribute " << attr.getName()
                    << " with type " << attr.getDataType().getClass() << endl; 
            }
            else if (attr.getDataType().getClass() == H5T_VLEN)
            {
                if (attr.getName() != "DIMENSION_LIST")
                    cout << "Could not handle attribute " << attr.getName()
                    << " with type " << attr.getDataType().getClass() << endl; 
            }
            else
            {
                void* buf = malloc(attr.getInMemDataSize());
                attr.read(attr.getDataType(), buf);
                // if the attribute is coordinates, need to verify it's still valid              
                if (attr.getName() != "coordinates" || isCoordinatesAttributeValid(attr, groupname))
                {
                    Attribute outattr = outobj.createAttribute(attr.getName(), datatype, attr.getSpace());
                    outattr.write(outattr.getDataType(), buf);
                }
                free(buf);
            }
        }
    }
        
    /**
     * subset and write dataset
     */
    virtual void writeDataset(const string& objname, const DataSet& indataset, Group& outgroup, const string& groupname,
                      IndexSelection* indexes)
    {
        // if the dataset is already included in the outgroup, don't create again.
        // e.g. we create /geolocation/segment_ph_cnt before we write /geolocation/ph_index_beg dataset
        if (H5Lexists(outgroup.getLocId(), objname.c_str(), H5P_DEFAULT) > 0)
        {
            cout << "writeDataset " << objname << " already exists in output" << endl;
            return;
        }
        DataSpace inspace(indataset.getSpace());
        int dimnum = indataset.getSpace().getSimpleExtentNdims();
        cout << "write_dataset " << objname << " with " << dimnum << " dimensions" << endl;
        hsize_t olddims[dimnum], newdims[dimnum],maxdims[dimnum];
        indataset.getSpace().getSimpleExtentDims(olddims,maxdims);
                
        // if indexes exists and dimensions of the dataset are the same as the coordinate dataset
        // set the newdims to that, otherwise use the olddims (copy everything)
        int dim = 0;  //matching dimension
        for (int d = 0; d < dimnum; d++)
        {
            //newdims[d] = (indexes != NULL && indexes->getMaxSize()==olddims[d]) ? indexes->size() : olddims[d];
            if (indexes != NULL && indexes->getMaxSize()==olddims[d])
            {
                newdims[d] = indexes->size();
            }
            else
            {
                newdims[d] = olddims[d];
                inconsistentDatasets.push_back(groupname+objname);
            }
            if (newdims[d] == 0 || olddims[d] == 0) return;
            if (newdims[d] != olddims[d])
            {
                dim = d;
                cout << "selecting " << newdims[d] << " instead of " << olddims[d] << " in outspace. " << endl;
            }
        }
         
        DataSpace outspace(dimnum, newdims,maxdims);
        DataType datatype(indataset.getDataType());
        DSetCreatPropList plist = indataset.getCreatePlist();
        if (indataset.getCreatePlist().getLayout() == H5D_CONTIGUOUS)
        {
            //cout << "contiguous" << endl;
            plist.setLayout(H5D_CHUNKED);
            plist.setChunk(dimnum, olddims);
            plist.setAllocTime(H5D_ALLOC_TIME_INCR);
        }
        DataSet outdataset(outgroup.createDataSet(objname, datatype, outspace, plist));
        copyAttributes(indataset, outdataset, groupname);

        // outspace is the same, selects everything
        if (newdims[dim] == olddims[dim]) 
        {
            inspace.selectAll();
        }
        else // selects indexes, indexes must be present
        {
            inspace.selectNone(); // selects none first            
            hsize_t offset[dimnum], count[dimnum];
            // spatial subsetting            
            for (map<long,long>::iterator it = indexes->segments.begin();it!=indexes->segments.end();it++)
            {
                offset[dim] = it->first;
                for (int j=0;j<dimnum;j++)
                {
                    if (j!=dim) offset[j]=0;
                }
                count[dim] = it->second;                
                //cout << "selecting " << count[0] << " from inspace with offset: " << offset[0] << endl;
                for (int j=0;j<dimnum;j++)
                {
                    if  (j!=dim) count[j]=newdims[j];
                }
                inspace.selectHyperslab(H5S_SELECT_OR, count, offset);
            }
            //cout << "Total spatially selected is " << inspace.getSelectNpoints() << endl;
    
            // adding temporal if no spatial subsetting
            if (indexes->segments.empty())
            {
                offset[0] = indexes->minIndexStart;
                for (int j=0;j<dimnum;j++)
                {
                    if (j!=dim) offset[j]=0;
                }
                count[dim] = indexes->maxIndexEnd - indexes->minIndexStart;
                //cout << "selecting " << count[0] << " temporal match inspace with offset " << offset[0] << endl;
                for (int j=0;j<dimnum;j++)
                {
                    if (j!=dim) count[j]=newdims[j];
                }
                inspace.selectHyperslab(H5S_SELECT_OR, count, offset);
            }
            //cout << "Total selected is " << inspace.getSelectNpoints() << endl;
        }
        
        void* buf = malloc(inspace.getSelectNpoints() * datatype.getSize());        
        indataset.read(buf, datatype, outspace, inspace);        
        //cout << "writing " << objname << endl;
        outdataset.write(buf, datatype, DataSpace::ALL, outspace);
        free(buf);
    }
    
    virtual Coordinate* getCoordinate(Group& root, Group& ingroup, const string& groupname, 
        SubsetDataLayers* subsetDataLayers, vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon, bool repair = false)
    {
        Coordinate* coor = Coordinate::getCoordinate(root, ingroup, groupname, shortName, subsetDataLayers, geoboxes, temporal, geoPolygon);
        return coor;
    }
    
private:

    /**
     * subset and write group and its datasets recursively
     * in IN: input group
     * inRootGroup IN: the root group of the input H5
     * groupname IN: group name
     * Group OUT: output group gets updated
     */
    int copyH5(Group& in, Group& inRootGroup, Group& out, string groupname)
    {
        // check if it's metadata group
        string metadataGroup = "/METADATA/";
        bool isMetadataGroup = (boost::to_upper_copy<std::string>(groupname).compare(0, metadataGroup.length(), metadataGroup) == 0) ? true : false;

        // indexes is NULL when no subsetting is applied to the group
        IndexSelection* indexes = NULL;
        if (!isMetadataGroup && Configuration::getInstance()->isGroupSubsettable(shortName, groupname))
        {            
            /*Coordinate* coor = new Coordinate(groupname, this->geoboxes, this->temporal);
            coor->getCoordinate(inRootGroup, in, shortName, subsetDataLayers);
            indexes =coor->indexes;*/
            if (geoboxes != NULL || temporal != NULL || geoPolygon != NULL) 
            {
                Coordinate* coor = getCoordinate(inRootGroup, in, groupname, subsetDataLayers, geoboxes, temporal, geoPolygon);
                if (coor->indexesProcessed)
                {
                    indexes = coor->indexes;
                }
                else{
                    indexes = coor->getIndexSelection();
                }
            }
        }
        
        // iterate links in the group and save the information,
        // ignore the /Metdata group (case insensitive) and its subgroup
        // it seems that icesat II doesn't have dataset links
        DatasetLinks* datasetlinks = new DatasetLinks();
        if (!isMetadataGroup)
            datasetlinks->trackDatasetLinks(in);
        
        short resolution;
            
        // loop through all the objects, recursing on groups and copying datasets,
        // determine if the group/dataset included
        for (int i = 0; i < in.getNumObjs(); i++)
        {
            string type_name, objname;
            objname = in.getObjnameByIdx(i);
            in.getObjTypeByIdx(i, type_name);
            string objectFullName = groupname + objname;
            
            cout << i << ":" << groupname + objname << ":" << type_name << endl;
            
            //recurse into group directories
            if (type_name == "group" && subsetDataLayers->is_included(groupname + objname + "/"))
            {
                //open this group and recurse on it
                Group ingroup(in.openGroup(objname));
                // Create the same group in the output file
                Group outgroup(out.createGroup(objname));
                
                if (outputFormat == "GeoTIFF" && !isMetadataGroup)
                {
                    requiredDatasets = Configuration::getInstance()->getRequiredDatasetsByFormat(outputFormat, groupname+objname+"/", shortName, resolution);
                }
                
                copyAttributes(ingroup, outgroup, groupname);
                copyH5(ingroup, inRootGroup, outgroup, groupname + objname + "/");
                // unlink if no object is copied over
                if (outgroup.getNumObjs() == 0 && ingroup.getNumObjs() != 0)
                    out.unlink(objname);
            }
            else if ((type_name == "symbolic link" || type_name == "dataset") && 
                    (subsetDataLayers->is_dataset_included(groupname + objname) || 
                    std::find(requiredDatasets.begin(), requiredDatasets.end(), objname) != requiredDatasets.end()))
            {
                // if the dataset can be linked to previous dataset which is included, 
                // then create the link and done with this dataset.                 
                if (datasetlinks->isHardLink(objname))
                {
                    string source = datasetlinks->getHardLinkSource(objname);
                    // if source dataset is included in the subset data layers and exist in outfile
                    if (subsetDataLayers->is_dataset_included(groupname + source) && 
                            H5Lexists(out.getLocId(), source.c_str(), H5P_DEFAULT) > 0)
                    {
                        out.link(H5L_TYPE_HARD, source, objname);
                        continue;
                    }
                }
                DataSet indataset(in.openDataSet(objname));
                // keep track of dimension scales for each dataset written
                dimensionScales->trackDimensionScales(indataset);
                if (Configuration::getInstance()->isPhotonDataset(this->getShortName(), groupname+objname))
                {
                    cout << "groupname+objname: " << groupname+objname << endl;
                    Coordinate* coor = getCoordinate(inRootGroup, in, groupname+objname, subsetDataLayers, geoboxes, temporal, geoPolygon);
                    IndexSelection* newIndexes = coor->getIndexSelection();
                    writeDataset(objname, indataset, out, groupname, newIndexes);
                    
                }
                else
                {
                    writeDataset(objname, indataset, out, groupname, indexes);
                }
            }
        }

        delete datasetlinks;
        //if (indexes != NULL) delete indexes;
        return 0;
    }

    /**
     * check if the coordinates attribute is still valid, since the
     * datasets which the attribute points to may not exist anymore
     */
    bool isCoordinatesAttributeValid(Attribute& attr, const string& groupname)
    {
        // examples of coordinates attribute value:
        // ICESAT2: coordinates = delta_time, reference_photon_lat, reference_photon_lon
        //          coordinates = ../delta_time, ../latitude, ../longitude
        // SMAPL1L2: coordinates = /Soil_Moisture_Retrieval_Data/latitude /Soil_Moisture_Retrieval_Data/longitude
        // note: This code doesn't handle GLAS dataset coordinates attribute which doesn't provide
        //       the correct path of the coordinate datasets. e.g.
        //       GLAH06.033 GLAH06_633_2107_002_0110_3_01_0001.H5
        //       /Data_40HZ/Elevation_Surfaces/d_elev/coordinates has value DS_UTCTime_40, but 
        //       dataset /Data_40HZ/DS_UTCTime_40 is under parent group
        //
        string attrValue;
        char_separator<char> delim(" ,");
        attr.read(attr.getDataType(), attrValue);
        tokenizer<char_separator<char>> datasets(attrValue, delim);
        //cout << "groupname: " << groupname << " coordinates: " << attrValue << endl;
        BOOST_FOREACH(string dataset, datasets)
        {            
            // if the dataset doesn't start with /, it has relative path,
            // add groupname to get full path.
            if (dataset.compare(0, 1, "/") != 0)
            {                
                // dataset full path
                dataset = groupname + dataset;
                // normalize the dataset path e.g. /pathx/pathy/../data to /pathx/data               
                string doublePeriods = "..";               
                if (dataset.find(doublePeriods) != string::npos)
                {                    
                    string pathDelim("/");
                    // first split the paths to separate parts
                    vector<string> paths;
                    split(paths, dataset, is_any_of(pathDelim));
                    // then remove the ".." element and the element before it
                    vector<string>::iterator it = find(paths.begin(), paths.end(), doublePeriods);
                    while (it != paths.end())
                    {
                        paths.erase(it - 1, it + 1);
                        it = find(paths.begin(), paths.end(), doublePeriods);
                    }
                    dataset = join(paths, pathDelim);
                    //cout << "normalized dataset string " << dataset << endl;
                }
            }
            //cout << "isCoordinatesAttributeValid dataset: " << dataset << endl;
            // if any of the dataset is not included, 
            // then the Coordinates attribute is not valid
            if (!subsetDataLayers->is_dataset_included(dataset))
            {
                return false;
            }
        }
        return true;
    }
    
    /* Add an attribute to capture processing parameters
     * complete list of subset data layer list, temporal and spatial bounds
     */
    void addProcessingParameterAttribute(H5Object& outobj, string& infilename)
    {
        cout << "addProcessingParameterAttribute" << endl;

        SubsetDataLayers* fullDatasetList = subsetDataLayers;
        
        const H5std_string attributeName("Processing Parameters");

        string message("This file was generated by the ICESat-2 Service version 1.0, ");
        message += "which performed the following operations on ";
        // message += basename(infilename.c_str());
        boost::filesystem::path p(infilename.c_str());
        message += p.filename().string();
        message += "\nExtracted the datasets named:\n";
        // a list of datasets included in the output file
        fullDatasetList->expand_all(infilename);
        vector < set <string> > datasets = fullDatasetList->getDatasets();
        vector < set <string> >::iterator it = datasets.begin();
        set <string>::iterator set_it;
        while (it != datasets.end())
        {
            set_it = it->begin();
            while (set_it != it->end())
            {
                message += *set_it + "\n";
                set_it++;
            }
            it++;
        }

        // spatial constraint
        if (geoboxes != NULL)
        {
            vector<geobox>::iterator it = geoboxes->begin();
            message += "within the spatial constraint defined by the bounding box ";
            while (it != geoboxes->end())
            {
                string west = boost::lexical_cast< std::string >(it->getWest());
                string east = boost::lexical_cast< std::string >(it->getEast());
                string south = boost::lexical_cast< std::string >(it->getSouth());
                string north = boost::lexical_cast< std::string >(it->getNorth());
                message += "(" + west + ", " + south + ", " + east + ", " + north + ")";
                it++;
            }
            message += "\n";
        }
        
        // temporal constraint
        if (temporal != NULL)
        {
            message += "within the temporal constraints defined by (" + 
                       temporal->getStartTime() + ", " + temporal->getEndTime() + ")";
        }

        const H5std_string attributeData(message.c_str());
        StrType fls_type(0, attributeData.length());
        DataSpace attributeSpace(H5S_SCALAR);
        Attribute warningAttribute = outobj.createAttribute(attributeName, fls_type, attributeSpace);
        warningAttribute.write(fls_type, attributeData.c_str());
    }

    /**
     * check if matching data found in the output.
     * If the result has only one root item, return false, since the meatadata group is always included.
     * Otherwise return true in the following cases:
     * (1). Only parameter subsetting
     * (2). Spatial/temporal subsetting, user request only contains unsubsettable datasets
     * (3). Spatial/temporal subsetting, user request contains subsettable datasets, and output has 
     *      subsettable datasets
     */
    bool isMatchingDataFound()
    {
        cout << "isMatchingDataFound()" << endl;
        bool matchingDataFound = false;
        Group ingroup = infile.openGroup("/");
        Group outgroup = outfile.openGroup("/");
        cout << "num of groups: " << outgroup.getNumObjs() << endl;
        // the output has more than metadata group if matching data is found        
        if (outgroup.getNumObjs() > 1)
        {
            // (1) only parameter subsetting
            if (this->temporal == NULL && this->geoboxes == NULL && this->geoPolygon == NULL)
                matchingDataFound = true;
            // (2)(3). spatial/temporal subsetting
            else
            {
                // check if request contains subsettable datasets
                // expand subsetDataLayers to get all the datasets/groups requested in the input file
                subsetDataLayers->expand_group(ingroup, "");
                //subsetDataLayers->print_datasets();
                bool subsettableDatasetRequested = containsSubsettableDatasets(subsetDataLayers->getDatasets());
                // (2) user only requested unsubsettable datasets,then return matching data
                if (!subsettableDatasetRequested)
                    matchingDataFound = true;
                else //(3) has to check if the output has any subsettable dataset 
                {
                    // use a new SubsetDataLayer object to get a list of the datasets/group in the output
                    SubsetDataLayers* osub = new SubsetDataLayers(vector<string>());
                    osub->expand_group(outgroup, "");
                    //osub->print_datasets();
                    matchingDataFound = containsSubsettableDatasets(osub->getDatasets());
                }
            }
        }
        return matchingDataFound;
    }
    
    /**
     * check to see whether any dataset in the given dataset list is subsettable.
     * If a dataset is not metadata, not a root dataset, not a dimension scale dataset, 
     * subsettable based on the configuration, and has inconsistent dataset size 
     * then the dataset is subsettable. 
     * @param datasets vector<set<string>> datasets list to check
     * @return true if any of the datasets is subsettable
     */
    bool containsSubsettableDatasets(vector<set<string>> datasets)
    {
        string metadataGroup = "/METADATA/";
        bool subsettable = false;
        vector < set <string> >::iterator it = datasets.begin();
        vector<string>* dimScales = dimensionScales->getDimScaleDatasets();
        while (it != datasets.end() && !subsettable)
        {
            set<string>::iterator set_iter = it->begin();
            while (set_iter != it->end())
            {
                string datasetName = *set_iter;
                //cout << "datasetName: " << datasetName << endl;
                bool isMetadataGroup =
                (boost::to_upper_copy<std::string>(datasetName).compare(0, metadataGroup.length(), metadataGroup) == 0) ?
                true : false;
                // note the datasetName from the SubsetDataLayer somehow ends with '/'
                if (!isMetadataGroup &&
                    count(datasetName.begin(), datasetName.end(), '/') > 2 && // not a root dataset
                    Configuration::getInstance()->isGroupSubsettable(shortName, datasetName) &&
                    std::find(dimScales->begin(), dimScales->end(), datasetName.substr(0, datasetName.size()-1)) == dimScales->end() &&
                    std::find(inconsistentDatasets.begin(), inconsistentDatasets.end(), datasetName.substr(0, datasetName.size()-1)) 
                        == inconsistentDatasets.end())
                {
                    cout << "subsettable " << datasetName << endl;
                    subsettable = true;
                    break;
                }
                set_iter++;
            }
            it++;
        }
        cout << "containsSubsettableDatasets returns " << subsettable << endl;
        return subsettable;
    }    
    
    /* private members */
    
    // subset data layers
    SubsetDataLayers* subsetDataLayers;
    
    // inconsistent datasets
    vector<string> inconsistentDatasets;
    
    // spatial search criteria
    vector<geobox>* geoboxes;
    
    // temporal search criteria
    Temporal* temporal;
    
    // polygon search criteria
    GeoPolygon* geoPolygon;
    
    // information on dimension scales
    DimensionScales* dimensionScales;
    
    // input and output H5File 
    H5File infile;
    H5File outfile;
    
    // product short name
    string shortName;
    
    // output format
    string outputFormat;
    
    //required datasets by format
    vector<string> requiredDatasets;
    
    //set to true when matching data is found
    bool matchingDataFound; 
};
#endif