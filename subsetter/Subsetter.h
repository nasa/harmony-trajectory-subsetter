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
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include "Configuration.h"
#include "Coordinate.h"
#include "DatasetLinks.h"
#include "DimensionScales.h"
#include "ForwardReferenceCoordinates.h"
#include "GeoPolygon.h"
#include "HeightSegmentCoordinates.h"
#include "IndexSelection.h"
#include "LogLevel.h"
#include "SubsetDataLayers.h"
#include "Temporal.h"
#include "geobox.h"
#include "geotiff_converter.h"

/**
 * This is the base class for subsetting an HDF5 file.
 */
class Subsetter
{
  public:
    Subsetter(SubsetDataLayers *subsetDataLayers,
              std::vector<geobox> *geoboxes,
              Temporal *temporal,
              GeoPolygon *geoPolygon,
              Configuration *config,
              std::string outputFormat = "")
        : subsetDataLayers(subsetDataLayers), geoboxes(geoboxes),
          temporal(temporal), matchingDataFound(false), geoPolygon(geoPolygon),
          config(config), outputFormat(outputFormat)
    {
        dimensionScales = new DimensionScales();
    };

    ~Subsetter() { delete dimensionScales; }

    // configuration information
    Configuration *config;

    /**
     * @brief This function performs the subset.
     *
     * @param infilename The input granule file path.
     * @param outfilename The output granule file path.
     * @return Error code (0 - success, 3 - no match data found)
     */
    int subset(std::string infilename,
               std::string outfilename,
               std::string collShortName)
    {
        LOG_DEBUG("Subsetter::subset(): ENTER");

        int returnCode = 0;

        // Open the input HDF5 file.
        LOG_DEBUG("Subsetter::subset(): Opening " << infilename);
        this->infile = H5::H5File(infilename, H5F_ACC_RDONLY);

        // Create (or overwrite the existing) output file with
        // the creation properties of the input file and with
        // the default access property list of the latest HDF5
        // library release version.
        LOG_DEBUG("Subsetter::subset(): Opening " << outfilename);
        hid_t fileAccessPropList = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_libver_bounds(
            fileAccessPropList, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
        H5::FileAccPropList fileAccessPropListObj(fileAccessPropList);
        this->outfile = H5::H5File(outfilename,
                                   H5F_ACC_TRUNC,
                                   infile.getCreatePlist(),
                                   fileAccessPropListObj);
        H5Pclose(fileAccessPropList);

        this->shortName = retrieveShortName(infile);
        if (this->shortName.empty() && !collShortName.empty())
        {
            this->shortName = collShortName;
            LOG_INFO("Subsetter::subset(): shortname: " << collShortName);
        }
        else if (this->shortName.empty() && collShortName.empty())
        {
            LOG_DEBUG(
                "Subsetter::subset(): ERROR: The short name could not be retrieved \
                        from the collection or was not defined in the command line arguments");
        }

        // Update epoch time if it's configured for this product,
        // otherwise return an empty string.
        std::string epochTime = config->getProductEpoch(shortName);
        if (!epochTime.empty() && this->temporal != NULL)
        {
            this->temporal->updateReferenceTime(epochTime);
        }

        // Open the top level/root group in both in the input and
        // output file.
        H5::Group ingroup = this->infile.openGroup("/");
        H5::Group outgroup = this->outfile.openGroup("/");

        // Copy the top level/root attributes to the output.
        copyAttributes(ingroup, outgroup, "/");

        // Convert and write group and its datasets recursively
        // to the root group.
        copyH5(ingroup, ingroup, outgroup, "/");

        // Write subsets for datasets for the special case where
        // the dataset has no spatial coordinates in requests that
        // only spatial constraints.
        writeRequiredTemporalSubsets(ingroup);

        // Recreate and re-attach dimension scales in the output file.
        dimensionScales->recreateDimensionScales(outfile);

        // Check if matching data is found.
        matchingDataFound = isMatchingDataFound(infile, outfile);

        if (!matchingDataFound)
        {
            returnCode = 3;
            if (remove(outfilename.c_str()))
                LOG_ERROR(
                    "Subsetter::subset(): No matching data for the constraints "
                    "specified.  Unable to remove: "
                    << outfilename);
            else
                LOG_INFO(
                    "Subsetter::subset(): No matching data for the constraints "
                    "specified. Removed output file "
                    << outfilename);
        }
        else
            LOG_INFO(
                "Subsetter::subset(): WRITING output file: " << outfilename);

        // Add an attribute that contains the specified parameters,
        // including the dataset list and the temporal and spatial bounds,
        // if specified.
        addProcessingParameterAttribute(outgroup, infilename);

        outfile.flush(H5F_SCOPE_GLOBAL);
        LOG_INFO("Subsetter::subset(): Datasets size: "
                 << subsetDataLayers->getDatasets().size());

        // If the specified output format is a GeoTIFF, call the GeoTIFF
        // converter.
        if (outputFormat == "GeoTIFF")
        {
            LOG_DEBUG("Subsetter::subset(): Outputting to GeoTIFF");
            geotiff_converter geotiff = geotiff_converter(
                outfilename, shortName, outgroup, subsetDataLayers, config);
        }

        return returnCode;
    }

    /**
     * @brief Retrieve the mission short name from an HDF5 file's metadata.
     *
     * @param file An HDF5 file.
     * @return The collection short name.
     */
    virtual std::string retrieveShortName(const H5::H5File &file)
    {
        LOG_DEBUG("Subsetter::retrieveShortName(): ENTER");

        H5std_string str;
        std::string shortnameFullPath, shortnamePath, shortnameLabel;
        H5::Group root = file.openGroup("/");
        std::vector<std::string> shortnamePaths = config->getShortnamePath();
        size_t found;

        H5E_auto2_t oldFunction;
        void *oldClientData;

        H5Eget_auto2(H5E_DEFAULT, &oldFunction, &oldClientData);
        //  Turn off error handling
        H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

        for (std::vector<std::string>::iterator it = shortnamePaths.begin();
             it != shortnamePaths.end();
             it++)
        {
            shortnameFullPath = *it;
            LOG_DEBUG(shortnameFullPath);
            // group
            // "/" is used in the config file to determine whether the short
            // name attribute is under a group or dataset short name attribute
            // path with
            // "/" at the end indicates it is a group path otherwise, it is a
            // dataset path
            if (shortnameFullPath.compare(
                    shortnameFullPath.length() - 1, 1, "/") == 0)
            {
                shortnameFullPath =
                    shortnameFullPath.substr(0, shortnameFullPath.size() - 1);
                found = shortnameFullPath.find_last_of("/");
                shortnamePath = shortnameFullPath.substr(0, found + 1);
                shortnameLabel = shortnameFullPath.substr(found + 1);
                if (H5Lexists(root.getLocId(),
                              shortnamePath.c_str(),
                              H5P_DEFAULT) > 0 &&
                    H5Gopen1(root.getLocId(), shortnamePath.c_str()) > 0)
                {
                    H5::Group group = root.openGroup(shortnamePath);
                    if (H5Aexists(group.getLocId(), shortnameLabel.c_str()) > 0)
                    {
                        H5::Attribute attr =
                            group.openAttribute(shortnameLabel);
                        attr.read(attr.getDataType(), str);
                        break;
                    }
                }
                else if (shortnamePath.size() == 1)
                {
                    if (H5Aexists(root.getLocId(), shortnameLabel.c_str()) > 0)
                    {
                        H5::Attribute attr = root.openAttribute(shortnameLabel);
                        attr.read(attr.getDataType(), str);
                        break;
                    }
                }
            }
            // dataset
            else
            {
                if (H5Lexists(root.getLocId(),
                              shortnamePath.c_str(),
                              H5P_DEFAULT) > 0 &&
                    H5Dopen1(root.getLocId(), shortnamePath.c_str()) > 0)
                {
                    found = shortnameFullPath.find_last_of("/");
                    shortnamePath = shortnameFullPath.substr(0, found + 1);
                    shortnameLabel = shortnameFullPath.substr(found + 1);
                    H5::DataSet data = root.openDataSet(shortnamePath);
                    if (H5Aexists(root.getLocId(), shortnameLabel.c_str()) > 0)
                    {
                        H5::Attribute attr = data.openAttribute(shortnameLabel);
                        attr.read(attr.getDataType(), str);
                        break;
                    }
                }
            }
        }

        //  Restore previous error handler
        H5Eset_auto2(H5E_DEFAULT, oldFunction, oldClientData);

        LOG_DEBUG("Subsetter::retrieveShortName(): Processing file of type "
                  << std::string(str));
        return std::string(str);
    }

    /**
     * @brief Determine what groups, if any, require special temporal
     * subsetting.
     *
     *        There are cases where a spatial-only request is made and a
     *        requested group has no spatial coordinates, but does have temporal
     * coordinates (e.g., `bckgrd_atlas` in ATL03). This function cycles through
     * every group recursively and checks if any groups meet this criteria.
     *
     * @param rootGroup The collection root group.
     * @param inputGroup The group object being recursed.
     * @param groupName The name of the group being recursed.
     */
    void addGroupsRequiringTemporalSubsetting(H5::Group &rootGroup,
                                              H5::Group &inputGroup,
                                              std::string groupName)
    {
        // Check that the request has only spatial constraints.
        if (temporal == NULL && (geoboxes != NULL || geoPolygon != NULL))
        {
            // Cycle through every object within the input group.
            for (int index = 0; index < inputGroup.getNumObjs(); index++)
            {
                std::string objectName = inputGroup.getObjnameByIdx(index);
                std::string objectPath = groupName + objectName + "/";
                std::string typeName = "";
                inputGroup.getObjTypeByIdx(index, typeName);

                // If the object is a group (as opposed to a dataset), is
                // subsettable, and included in the output, check to see if
                // it has only temporal and no spatial coordinates.

                // Check if the input group is a metadata group.
                std::string metadataGroup = "/METADATA/";
                bool isMetadataGroup =
                    (boost::to_upper_copy<std::string>(objectPath)
                         .compare(0, metadataGroup.length(), metadataGroup) ==
                     0)
                        ? true
                        : false;

                if (typeName == "group" && !isMetadataGroup &&
                    config->isGroupSubsettable(shortName, objectPath) &&
                    subsetDataLayers->is_included(objectPath))
                {
                    // Access the group's coordinates to check if it only has
                    // temporal and no spatial coordinates.
                    Coordinate *coor = getCoordinate(rootGroup,
                                                     inputGroup,
                                                     objectPath,
                                                     subsetDataLayers,
                                                     geoboxes,
                                                     temporal,
                                                     geoPolygon,
                                                     config);

                    if (coor->hasTemporalOnlyCoordinates())
                    {
                        LOG_DEBUG("Group " << objectPath << " has no spatial ");
                        LOG_DEBUG(
                            "coordinates but requires spatial subsetting. ");
                        LOG_DEBUG("Temporal subsetting will be applied.\n");
                        this->groupsRequiringTemporalSubsetting.push_back(
                            objectPath);
                    }
                    H5::Group objectGroup = this->infile.openGroup(objectPath);
                    addGroupsRequiringTemporalSubsetting(
                        rootGroup, objectGroup, objectPath);
                }
            }
        }
    }

    std::vector<std::string> getGroupsRequiringTemporalSubsetting()
    {
        return this->groupsRequiringTemporalSubsetting;
    }
    std::string getShortName() { return this->shortName; }

    /**
     * @brief Check if matching data found in the output.
     *
     *        Return false if the result has zero or one group and that group is
     * the metadata group. Otherwise, the following cases are considered: (1)
     * Only parameter subsetting. (2) Spatial/temporal subsetting, user request
     * only contains unsubsettable datasets. (3) Spatial/temporal subsetting,
     * user request contains subsettable datasets, and output has subsettable
     * datasets.
     *
     * @return true
     * @return false
     */
    bool isMatchingDataFound(H5::H5File infileH5, H5::H5File outfileH5)
    {
        LOG_DEBUG("Subsetter::isMatchingDataFound(): ENTER");

        H5::Group ingroup = infileH5.openGroup("/");
        H5::Group outgroup = outfileH5.openGroup("/");
        LOG_DEBUG("Subsetter::isMatchingDataFound(): num of groups: "
                  << outgroup.getNumObjs());

        // Check for no objects found within the subset
        if (outgroup.getNumObjs() <= 0)
        {
            return false;
        }

        // There may be an instance where the first and only group in the
        // file is the metadata group, which is not subsettable.
        std::string group_name =
            boost::to_upper_copy<std::string>(outgroup.getObjnameByIdx(0));
        if (outgroup.getNumObjs() == 1 && group_name == "METADATA")
        {
            return false;
        }

        // (1) If only variable subsetting, then we know that
        // data clearly exists and outgroup.getNumObjs() > 1.
        if (this->temporal == NULL && this->geoboxes == NULL &&
            this->geoPolygon == NULL)
        {
            return true;
        }

        // Check if infileH5::ingroup request contains subsettable datasets.
        // Expand subsetDataLayers to get all the datasets/groups
        // requested in the input file.
        subsetDataLayers->expand_group(ingroup, "");
        bool inGroupSubsettableDatasetRequested =
            containsSubsettableDatasets(subsetDataLayers->getDatasets());

        LOG_DEBUG("Subsetter::isMatchingDataFound(): "
                  "inGroupSubsettableDatasetRequesteds: "
                  << std::boolalpha << inGroupSubsettableDatasetRequested);

        // (2) If the user only requested unsubsettable datasets,
        // then return matching data.
        if (inGroupSubsettableDatasetRequested == false)
        {
            return true;
        }

        // (3) If not, check if the output has any subsettable dataset.
        // Use a new SubsetDataLayer object to get a list of the datasets/group
        // in the output.
        std::unique_ptr<SubsetDataLayers> outGroupSubsetDataLayers =
            std::make_unique<SubsetDataLayers>(std::vector<std::string>());
        outGroupSubsetDataLayers->expand_group(outgroup, "");
        bool outGroupSubsettableDatasetRequested = containsSubsettableDatasets(
            outGroupSubsetDataLayers->getDatasets());

        LOG_DEBUG("Subsetter::isMatchingDataFound(): "
                  "outGroupSubsettableDatasetRequested: "
                  << std::boolalpha << outGroupSubsettableDatasetRequested);
        return outGroupSubsettableDatasetRequested;
    }

  protected:
    // product short name
    std::string shortName;

    // input and output H5::H5File
    H5::H5File infile;
    H5::H5File outfile;

    H5::H5File getInputFile() { return this->infile; }
    H5::H5File getOutputFile() { return this->outfile; }
    SubsetDataLayers *getSubsetDataLayers() { return this->subsetDataLayers; }
    std::vector<geobox> *getGeobox() { return this->geoboxes; }
    Temporal *getTemporal() { return this->temporal; }
    GeoPolygon *getGeoPolygon() { return this->geoPolygon; }

    /**
     * @brief Copy the group attributes from the input file to the output file.
     *
     * @param inputFile The input HDF5 file object.
     * @param outputFile The output HDF5 file object.
     * @param groupname The group whose attributes are being copied.
     */
    void copyAttributes(const H5::H5Object &inputFile,
                        H5::H5Object &outputFile,
                        const std::string &groupname)
    {
        LOG_DEBUG(
            "Subsetter::copyAttributes(): ENTER groupname: " << groupname);

        for (int k = 0; k < inputFile.getNumAttrs(); k++)
        {
            H5::Attribute attribute = inputFile.openAttribute(k);
            if (outputFile.attrExists(attribute.getName()))
            {
                continue;
            }
            H5::DataType datatype = attribute.getDataType();

            // The attribute with datatype H5T_COMPOUND and H5T_VLEN
            // are for dimension scales which are handled separately.
            if (attribute.getDataType().getClass() == H5T_COMPOUND)
            {
                if (attribute.getName() != "REFERENCE_LIST")
                {
                    LOG_DEBUG("Subsetter::copyAttributes(): Could not handle "
                              "attribute "
                              << attribute.getName() << " with type "
                              << attribute.getDataType().getClass());
                }
            }
            else if (attribute.getDataType().getClass() == H5T_VLEN)
            {
                if (attribute.getName() != "DIMENSION_LIST")
                {
                    LOG_DEBUG("Subsetter::copyAttributes(): Could not handle "
                              "attribute "
                              << attribute.getName() << " with type "
                              << attribute.getDataType().getClass());
                }
            }
            // Otherwise, copy over the attribute to the output file.
            else
            {
                void *buf = malloc(attribute.getInMemDataSize());
                attribute.read(attribute.getDataType(), buf);
                // If the attribute is `coordinates`, we need to verify
                // it's still valid since the datasets which the attribute
                // points to may not exist anymore.
                if (attribute.getName() != "coordinates" ||
                    isCoordinatesAttributeValid(attribute, groupname))
                {
                    H5::Attribute outattr = outputFile.createAttribute(
                        attribute.getName(), datatype, attribute.getSpace());
                    outattr.write(outattr.getDataType(), buf);
                }
                free(buf);
            }
        }
    }

    /**
     * @brief Subset and write dataset.
     *
     * @param objname The name of the dataset.
     * @param indataset The input dataset.
     * @param outgroup The dataset's output parent group.
     * @param groupname The name of the dataset's parent group.
     * @param indexes The subset dataset indexes.
     */
    virtual void writeDataset(const std::string &objname,
                              const H5::DataSet &indataset,
                              H5::Group &outgroup,
                              const std::string &groupname,
                              IndexSelection *indexes)
    {
        LOG_DEBUG("Subsetter::writeDataset(): ENTER groupname: " << groupname);

        // If the dataset is already included in the outgroup, don't create it
        // again. e.g. we create /geolocation/segment_ph_cnt before we write
        // /geolocation/ph_index_beg dataset
        if (H5Lexists(outgroup.getLocId(), objname.c_str(), H5P_DEFAULT) > 0)
        {
            LOG_DEBUG("Subsetter::writeDataset(): objname: "
                      << objname << " already exists in output");
            return;
        }

        // If the request includes a spatial and/or temporal constraint, the
        // subset index selection may have changed the overall time range,
        // so it should be updated accordingly.
        if ((indexes != NULL) &&
            (geoboxes != NULL || temporal != NULL || geoPolygon != NULL))
        {
            updateTimeRange(objname, indataset, indexes);
        }

        // Calculate new dimensions.
        int dimnum = indataset.getSpace().getSimpleExtentNdims();
        LOG_DEBUG("Subsetter::writeDataset(): objname: "
                  << objname << " with " << dimnum << " dimensions");
        hsize_t olddims[dimnum];
        hsize_t newdims[dimnum];
        hsize_t maxdims[dimnum];
        indataset.getSpace().getSimpleExtentDims(olddims, maxdims);

        // If new indexes exist and dimensions of the dataset are the same
        // as the coordinate dataset, set the new dimensions to that.
        // Otherwise use the old dimension and copy over everything.
        int dim = 0; // Matching dimension
        for (int d = 0; d < dimnum; d++)
        {
            if (indexes != NULL && indexes->getMaxSize() == olddims[d])
            {
                newdims[d] = indexes->size();
            }
            else
            {
                newdims[d] = olddims[d];
                inconsistentDatasets.push_back(groupname + objname);
            }

            if (newdims[d] == 0 || olddims[d] == 0)
                return;

            if (newdims[d] != olddims[d])
            {
                dim = d;
                LOG_DEBUG("Subsetter::writeDataset(): selecting "
                          << newdims[d] << " instead of " << olddims[d]
                          << " in outspace. ");
            }
        }

        // Construct new dataspace, datatype, list of properties.
        H5::DataSpace outspace(dimnum, newdims, maxdims);
        H5::DataType datatype(indataset.getDataType());
        H5::DSetCreatPropList plist = indataset.getCreatePlist();
        if (indataset.getCreatePlist().getLayout() == H5D_CONTIGUOUS &&
            dimnum > 0)
        {
            plist.setLayout(H5D_CHUNKED);
            plist.setChunk(dimnum, olddims);
            plist.setAllocTime(H5D_ALLOC_TIME_INCR);
        }

        // Construct the new dataset.
        H5::DataSet outdataset(
            outgroup.createDataSet(objname, datatype, outspace, plist));
        copyAttributes(indataset, outdataset, groupname);
        H5::DataSpace inspace(indataset.getSpace());

        // Select the input dataset regions to write to the output dataspace.

        // If the output dimensions of the dataset are unchanged, select the
        // entire input dataspace.
        if (newdims[dim] == olddims[dim])
        {
            inspace.selectAll();
        }
        else // Otherwise, select data regions using new indexes.
        {
            inspace.selectNone(); // Reset selection region.
            hsize_t offset[dimnum];
            hsize_t count[dimnum];

            // Select data regions using spatial subset constraints.
            for (std::map<long, long>::iterator it = indexes->segments.begin();
                 it != indexes->segments.end();
                 it++)
            {
                offset[dim] = it->first;
                for (int j = 0; j < dimnum; j++)
                {
                    if (j != dim)
                        offset[j] = 0;
                }
                count[dim] = it->second;
                for (int j = 0; j < dimnum; j++)
                {
                    if (j != dim)
                        count[j] = newdims[j];
                }
                inspace.selectHyperslab(H5S_SELECT_OR, count, offset);
            }

            // Select data regions using temporal constraints in two cases:
            // 1) Temporal subsetting with no spatial subsetting.
            // 2) Spatial subset is not within the bounds of the temporal
            //    constraints.
            if (indexes->segments.empty())
            {
                offset[0] = indexes->minIndexStart;
                for (int j = 0; j < dimnum; j++)
                {
                    if (j != dim)
                        offset[j] = 0;
                }
                count[dim] = indexes->maxIndexEnd - indexes->minIndexStart;
                for (int j = 0; j < dimnum; j++)
                {
                    if (j != dim)
                        count[j] = newdims[j];
                }
                inspace.selectHyperslab(H5S_SELECT_OR, count, offset);
            }
        }

        // Allocate memory to write selected data spaces to the output.
        void *buf = malloc(inspace.getSelectNpoints() * datatype.getSize());
        indataset.read(buf, datatype, outspace, inspace);
        outdataset.write(buf, datatype, H5::DataSpace::ALL, outspace);
        free(buf);
    }

    virtual Coordinate *getCoordinate(H5::Group &root,
                                      H5::Group &ingroup,
                                      const std::string &groupname,
                                      SubsetDataLayers *subsetDataLayers,
                                      std::vector<geobox> *geoboxes,
                                      Temporal *temporal,
                                      GeoPolygon *geoPolygon,
                                      Configuration *config,
                                      bool repair = false)
    {
        Coordinate *coor = Coordinate::getCoordinate(root,
                                                     ingroup,
                                                     groupname,
                                                     shortName,
                                                     subsetDataLayers,
                                                     geoboxes,
                                                     temporal,
                                                     geoPolygon,
                                                     config);
        return coor;
    }

  private:
    /**
     * @brief Subset and write a group and its datasets recursively.
     *
     * @param in The input group to be subset.
     * @param inRootGroup The input root group.
     * @param out The output group.
     * @param groupname The name of the group.
     * @return status - Success = 0.
     */
    int copyH5(H5::Group &in,
               H5::Group &inRootGroup,
               H5::Group &out,
               std::string groupname)
    {
        LOG_DEBUG("Subsetter::copyH5(): ENTER groupname: " << groupname);

        // Check if the input group is a metadata group.
        std::string metadataGroup = "/METADATA/";
        bool isMetadataGroup =
            (boost::to_upper_copy<std::string>(groupname).compare(
                 0, metadataGroup.length(), metadataGroup) == 0)
                ? true
                : false;

        // Indexes is NULL when no subsetting has been applied to the group.
        IndexSelection *indexes = NULL;
        if (!isMetadataGroup &&
            config->isGroupSubsettable(shortName, groupname))
        {
            // If spatial or temporal subsets are requested, use the
            // respective coordinate variables of each group to calculate the
            // index subsets that are to be applied to each dataset in the
            // group.
            if (geoboxes != NULL || temporal != NULL || geoPolygon != NULL)
            {
                Coordinate *coor = getCoordinate(inRootGroup,
                                                 in,
                                                 groupname,
                                                 subsetDataLayers,
                                                 geoboxes,
                                                 temporal,
                                                 geoPolygon,
                                                 config);
                if (coor->indexesProcessed)
                {
                    indexes = coor->indexes;
                }
                else
                {
                    indexes = coor->getIndexSelection();
                }
            }
        }

        // Iterate through the links in the group and save the information,
        // ignoring the METADATA group (case insensitive) and its subgroups.
        // Note: It appears that ICESat-2 doesn't have dataset links.
        DatasetLinks *datasetlinks = new DatasetLinks();
        if (!isMetadataGroup)
            datasetlinks->trackDatasetLinks(in);

        short resolution;

        // Loop through all the objects in the input group, recursing on
        // groups and copying datasets.
        // Then determine if the group/dataset has been requested.
        for (int i = 0; i < in.getNumObjs(); i++)
        {
            std::string type_name, objname;
            objname = in.getObjnameByIdx(i);
            in.getObjTypeByIdx(i, type_name);
            std::string objectFullName = groupname + objname;

            LOG_DEBUG("Subsetter::copyH5(): " << i << ":" << groupname + objname
                                              << ":" << type_name);

            // Add groups and dataset to cache to verify granule version
            config->addShortNameGroupDatasetFromGranuleFile(
                shortName, objectFullName + "/");

            // Recurse into the group's sub-groups.
            if (type_name == "group" &&
                subsetDataLayers->is_included(groupname + objname + "/"))
            {
                H5::Group ingroup(in.openGroup(objname));
                H5::Group outgroup(out.createGroup(objname));

                if (outputFormat == "GeoTIFF" && !isMetadataGroup)
                {
                    requiredDatasets = config->getRequiredDatasetsByFormat(
                        outputFormat,
                        groupname + objname + "/",
                        shortName,
                        resolution);
                }

                copyAttributes(ingroup, outgroup, groupname);
                copyH5(
                    ingroup, inRootGroup, outgroup, groupname + objname + "/");

                // Unlink if no object has been copied over.
                if (outgroup.getNumObjs() == 0 && ingroup.getNumObjs() != 0)
                    out.unlink(objname);
            }
            else if ((type_name == "symbolic link" || type_name == "dataset") &&
                     (subsetDataLayers->is_dataset_included(groupname +
                                                            objname) ||
                      std::find(requiredDatasets.begin(),
                                requiredDatasets.end(),
                                objname) != requiredDatasets.end()))
            {
                // If the dataset can be linked to previous dataset which is
                // included, then create the link and be done with this dataset.
                if (datasetlinks->isHardLink(objname))
                {
                    std::string source =
                        datasetlinks->getHardLinkSource(objname);
                    // Check if the source dataset is included in the subset
                    // data layers and exists in outfile.
                    if (!source.empty() &&
                        subsetDataLayers->is_dataset_included(groupname +
                                                              source) &&
                        H5Lexists(out.getLocId(), source.c_str(), H5P_DEFAULT) >
                            0)
                    {
                        out.link(H5L_TYPE_HARD, source, objname);
                        continue;
                    }
                }
                H5::DataSet indataset(in.openDataSet(objname));

                // Keep track of dimension scales for each dataset written.
                dimensionScales->trackDimensionScales(indataset);

                // Write the dataset to the output group, where photon datasets
                // require revised index selections based on which dataset is
                // being subset.
                if (config->isPhotonDataset(this->getShortName(),
                                            groupname + objname))
                {
                    LOG_DEBUG("Subsetter::copyH5(): groupname+objname: "
                              << groupname + objname);
                    Coordinate *coor = getCoordinate(inRootGroup,
                                                     in,
                                                     groupname + objname,
                                                     subsetDataLayers,
                                                     geoboxes,
                                                     temporal,
                                                     geoPolygon,
                                                     config);
                    IndexSelection *newIndexes = coor->getIndexSelection();
                    writeDataset(
                        objname, indataset, out, groupname, newIndexes);
                }
                else
                {
                    writeDataset(objname, indataset, out, groupname, indexes);
                }
            }
        }

        delete datasetlinks;

        return 0;
    }

    /**
     * @brief Determine if a dataset is a time coordinate and update the
     *        time range accordingly.
     *
     * @param objname Then name of the dataset.
     * @param indataset The input dataset.
     * @param indexes The subset dataset indexes.
     */
    void updateTimeRange(const std::string &objname,
                         const H5::DataSet &indataset,
                         IndexSelection *indexes)
    {

        // Check if this is a time coordinate dataset.
        std::vector<std::string> datasetName(1, objname);
        std::string timeName, latitudeName, longitudeName, ignoreName;
        config->getMatchingCoordinateDatasetNames(this->getShortName(),
                                                  datasetName,
                                                  timeName,
                                                  latitudeName,
                                                  longitudeName,
                                                  ignoreName);
        if (!timeName.empty())
        {
            LOG_DEBUG("Subbsetter::updateTimeRange(): objname: "
                      << objname << " is a time coordinate dataset.");

            // Check if this time coordinate has been subset (either temporally
            // or spatially), so we can glean the subset time range.
            // If so, extract the dataset's time range.
            if (indexes->size() != 0 and
                indexes->size() != indexes->getMaxSize())
            {
                size_t inputCoordinateSize =
                    indataset.getSpace().getSimpleExtentNpoints();
                std::vector<double> dataVector(inputCoordinateSize);
                double *data = dataVector.data();
                indataset.read(data, indataset.getDataType());

                double minimumTime = 0;
                double maximumTime = 0;

                // If we have either spatial or temporal subsetting, the
                // index segments encompass both spatial and temporal ranges.
                if (!indexes->segments.empty())
                {
                    auto firstSegment = indexes->segments.begin();
                    auto lastSegment = prev(indexes->segments.end());
                    minimumTime = data[firstSegment->first];
                    maximumTime =
                        data[lastSegment->first + lastSegment->second - 1];
                }
                else // Otherwise we only have temporal subset constraints.
                {
                    minimumTime = data[indexes->minIndexStart];
                    maximumTime = data[indexes->maxIndexEnd - 1];
                }

                // Assign time range if no range has been assigned yet.
                if ((timeRange.first == 0) && (0 == timeRange.second))
                {
                    timeRange.first = minimumTime;
                    timeRange.second = maximumTime;
                }

                // We want the subsetter time range to reflect the widest
                // range found across every time dataset in the output.
                if (minimumTime < this->timeRange.first)
                {
                    timeRange.first = minimumTime;
                }

                if (maximumTime > this->timeRange.second)
                {
                    timeRange.second = maximumTime;
                }
            }

            LOG_DEBUG("Subbsetter::updateTimeRange(): Current time range: ("
                      << std::fixed << timeRange.first << ", "
                      << timeRange.second << ")");
        }
    }

    /**
     * @brief Subsets and writes groups recursively that need special
     *        temporal subsetting.
     *
     *        This occurs in the case where a group has no spatial coordinates
     *        in requests that only spatial constraints.
     *
     * @param rootGroup This root group.
     */
    void writeRequiredTemporalSubsets(H5::Group &rootGroup)
    {
        LOG_DEBUG("Subsetter::writeRequiredTemporalSubsets(): ENTER");

        addGroupsRequiringTemporalSubsetting(rootGroup, rootGroup, "/");

        if (!groupsRequiringTemporalSubsetting.empty())
        {
            // Construct temporal constraints using calculated time range.
            LOG_DEBUG("Applying temporal range (" << timeRange.first << ", ");
            LOG_DEBUG(timeRange.second
                      << ") for groups requiring spatial subsetting");
            this->temporal = new Temporal(timeRange.first, timeRange.second);

            // Write requested groups recursively.
            for (std::string groupname :
                 this->groupsRequiringTemporalSubsetting)
            {
                // Remove group from coordinate look-up map.
                Coordinate::lookUpMap.erase(groupname);

                // Remove and recreate output group.
                H5::Group group = this->infile.openGroup(groupname);
                H5::Group outGroupExisting = this->outfile.openGroup(groupname);
                H5Ldelete(outGroupExisting.getLocId(),
                          groupname.c_str(),
                          H5P_DEFAULT);
                H5::Group newOutGroup = this->outfile.createGroup(groupname);
                copyH5(group, rootGroup, newOutGroup, groupname);
            }
        }
    }

    /**
     * @brief Check if the `coordinates` attribute is still valid, since the
     *        datasets which the attribute points to may not exist anymore.
     *
     * @param attr The attribute to be checked.
     * @param groupname The name of the group the attribute is attributed to.
     * @return true
     * @return false
     */
    bool isCoordinatesAttributeValid(H5::Attribute &attr,
                                     const std::string &groupname)
    {
        // Examples of `coordinates` attribute value:
        // ICESAT2: coordinates = delta_time, reference_photon_lat,
        // reference_photon_lon
        //          coordinates = ../delta_time, ../latitude, ../longitude
        // SMAPL1L2: coordinates = /Soil_Moisture_Retrieval_Data/latitude
        // /Soil_Moisture_Retrieval_Data/longitude note: This code doesn't
        // handle GLAS dataset `coordinates` attribute which doesn't provide
        //       the correct path of the coordinate datasets. e.g.
        //       GLAH06.033 GLAH06_633_2107_002_0110_3_01_0001.H5
        //       /Data_40HZ/Elevation_Surfaces/d_elev/coordinates has value
        //       DS_UTCTime_40, but dataset /Data_40HZ/DS_UTCTime_40 is under
        //       parent group
        //
        std::string attrValue;
        boost::char_separator<char> delim(" ,");
        attr.read(attr.getDataType(), attrValue);
        boost::tokenizer<boost::char_separator<char>> datasets(attrValue,
                                                               delim);
        BOOST_FOREACH (std::string dataset, datasets)
        {
            // Check of the dataset has a relative path (i.e., doesn't start
            // with /). If so, add the groupname to get the full path.
            if (dataset.compare(0, 1, "/") != 0)
            {
                dataset = groupname + dataset;
                // Normalize the dataset path e.g. /pathx/pathy/../data to
                // /pathx/data.
                std::string doublePeriods = "..";
                if (dataset.find(doublePeriods) != std::string::npos)
                {
                    std::string pathDelim("/");
                    // First split the paths to separate parts.
                    std::vector<std::string> paths;
                    split(paths, dataset, boost::is_any_of(pathDelim));
                    // Next remove the ".." element and the element before it.
                    std::vector<std::string>::iterator it =
                        find(paths.begin(), paths.end(), doublePeriods);
                    while (it != paths.end())
                    {
                        paths.erase(it - 1, it + 1);
                        it = find(paths.begin(), paths.end(), doublePeriods);
                    }
                    dataset = boost::join(paths, pathDelim);
                }
            }
            // If any of the datasets are not included, then the `coordinates`
            // attribute is not valid.
            if (!subsetDataLayers->is_dataset_included(dataset))
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Add an attribute to capture processing parameters, which is
     *        complete list of subset data layers, temporal, and spatial bounds.
     *
     * @param outobj - The HDF5 object to which attributes are being added to.
     * @param infilename - The input file.
     */
    void addProcessingParameterAttribute(H5::H5Object &outobj,
                                         std::string &infilename)
    {
        LOG_DEBUG("Subsetter::addProcessingParameterAttribute(): ENTER");

        SubsetDataLayers *fullDatasetList = subsetDataLayers;

        const H5std_string attributeName("Processing Parameters");

        std::string message(
            "This file was generated by the ICESat-2 Service version 1.0, ");
        message += "which performed the following operations on ";
        boost::filesystem::path p(infilename.c_str());
        message += p.filename().string();
        message += "\nExtracted the datasets named:\n";
        // A list of datasets to be included in the output file.
        fullDatasetList->expand_all(infilename);
        std::vector<std::set<std::string>> datasets =
            fullDatasetList->getDatasets();
        std::vector<std::set<std::string>>::iterator it = datasets.begin();
        std::set<std::string>::iterator set_it;
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

        // Spatial constraints.
        if (geoboxes != NULL)
        {
            std::vector<geobox>::iterator it = geoboxes->begin();
            message +=
                "within the spatial constraint defined by the bounding box ";
            while (it != geoboxes->end())
            {
                std::string west =
                    boost::lexical_cast<std::string>(it->getWest());
                std::string east =
                    boost::lexical_cast<std::string>(it->getEast());
                std::string south =
                    boost::lexical_cast<std::string>(it->getSouth());
                std::string north =
                    boost::lexical_cast<std::string>(it->getNorth());
                message += "(" + west + ", " + south + ", " + east + ", " +
                           north + ")";
                it++;
            }
            message += "\n";
        }

        // Temporal constraints.
        if (temporal != NULL)
        {
            message += "within the temporal constraints defined by (" +
                       temporal->getStartTime() + ", " +
                       temporal->getEndTime() + ")";
        }

        const H5std_string attributeData(message.c_str());
        H5::StrType fls_type(0, attributeData.length());
        H5::DataSpace attributeSpace(H5S_SCALAR);
        H5::Attribute warningAttribute =
            outobj.createAttribute(attributeName, fls_type, attributeSpace);
        warningAttribute.write(fls_type, attributeData.c_str());
    }

    /**
     * @brief Check to see whether any dataset in the given dataset list is
     * subsettable.
     *
     *        Returns true if a dataset:
     *        - Is not metadata a root dataset or a dimension scale.
     *        - Is subsettable according to the configuration.
     *        - Has inconsistent dataset size.
     *
     * @param datasets Datasets list to check for at least one subsettable
     * dataset.
     * @return true
     * @return false
     */
    bool containsSubsettableDatasets(
        std::vector<std::set<std::string>> datasets)
    {
        std::string metadataGroup = "/METADATA/";
        bool subsettable = false;
        std::vector<std::set<std::string>>::iterator it = datasets.begin();
        std::vector<std::string> *dimScales =
            dimensionScales->getDimScaleDatasets();

        while (it != datasets.end() && !subsettable)
        {
            std::set<std::string>::iterator set_iter = it->begin();
            while (set_iter != it->end())
            {
                std::string datasetName = *set_iter;
                bool isMetadataGroup =
                    (boost::to_upper_copy<std::string>(datasetName)
                         .compare(0, metadataGroup.length(), metadataGroup) ==
                     0)
                        ? true
                        : false;
                // Note the datasetName from the SubsetDataLayer somehow ends
                // with '/'.
                if (!isMetadataGroup &&
                    count(datasetName.begin(), datasetName.end(), '/') >
                        2 && // This is not a root dataset.
                    config->isGroupSubsettable(shortName, datasetName) &&
                    std::find(dimScales->begin(),
                              dimScales->end(),
                              datasetName.substr(0, datasetName.size() - 1)) ==
                        dimScales->end() &&
                    std::find(inconsistentDatasets.begin(),
                              inconsistentDatasets.end(),
                              datasetName.substr(0, datasetName.size() - 1)) ==
                        inconsistentDatasets.end())
                {
                    LOG_DEBUG(
                        "Subsetter::containsSubsettableDatasets(): subsettable "
                        << datasetName);
                    subsettable = true;
                    break;
                }
                set_iter++;
            }
            it++;
        }
        LOG_DEBUG("Subsetter::containsSubsettableDatasets(): "
                  "containsSubsettableDatasets returns "
                  << subsettable);
        return subsettable;
    }

    /* private members */

    // subset data layers
    SubsetDataLayers *subsetDataLayers;

    // inconsistent datasets
    std::vector<std::string> inconsistentDatasets;

    // spatial search criteria
    std::vector<geobox> *geoboxes;

    // temporal search criteria
    Temporal *temporal;

    // polygon search criteria
    GeoPolygon *geoPolygon;

    // information on dimension scales
    DimensionScales *dimensionScales;

    // output format
    std::string outputFormat;

    // required datasets by format
    std::vector<std::string> requiredDatasets;

    // set to true when matching data is found
    bool matchingDataFound;

    // subset time range <minimum, maximum>
    std::pair<double, double> timeRange;

    // datasets that require temporal subsetting when only spatial
    // constraints are defined.
    std::vector<std::string> groupsRequiringTemporalSubsetting;
};
#endif
