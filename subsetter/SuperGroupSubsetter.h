#ifndef SuperGroupSubsetter_H
#define SuperGroupSubsetter_H

#include "Configuration.h"
#include "GeoPolygon.h"
#include "LogLevel.h"
#include "SubsetDataLayers.h"
#include "SuperGroupCoordinate.h"
#include "Temporal.h"
#include "geobox.h"

#include "H5Cpp.h"

// GEDI specific implementation
class SuperGroupSubsetter : public Subsetter
{
  public:
    SuperGroupSubsetter(SubsetDataLayers *subsetDataLayers,
                        std::vector<geobox> *geoboxes,
                        Temporal *temporal,
                        GeoPolygon *geoPolygon,
                        Configuration *config)
        : Subsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, config)
    {
        LOG_DEBUG("SuperGroupSubsetter::SuperGroupSubsetter(): constructor");
    }

  protected:
    /**
     * subset and write dataset
     * @param objname string dataset object name
     * @param indataset DataSet the input dataset
     * @param outgroup Group the output group
     * @param groupname string group name
     * @param indexes IndexSelection index selection object
     */
    virtual void writeDataset(const std::string &objname,
                              const H5::DataSet &indataset,
                              H5::Group &outgroup,
                              const std::string &groupname,
                              IndexSelection *indexes)
    {
        LOG_DEBUG("SuperGroupSubsetter::writeDataset(): ENTER groupname: "
                  << groupname);

        H5::H5File infile = getInputFile();
        if (indexes != NULL && indexes->getMaxSize() != indexes->size() &&
            indexes->size() != 0 &&
            config->isSegmentGroup(this->getShortName(), groupname) &&
            config->getIndexBeginDatasetName(
                this->getShortName(), groupname, objname) == objname)
        {
            // need to make sure count dataset exists
            std::string countName = config->getCountDatasetName(
                this->getShortName(), groupname, objname);
            // if the count dataset is not in the output file, create it
            if (H5Lexists(
                    outgroup.getLocId(), countName.c_str(), H5P_DEFAULT) <= 0)
            {
                // if the input file doesn't have the count dataset, write index
                // begin as normal dataset
                if (H5Lexists(infile.getLocId(),
                              (groupname + countName).c_str(),
                              H5P_DEFAULT) <= 0)
                {
                    Subsetter::writeDataset(
                        objname, indataset, outgroup, groupname, indexes);
                    return;
                }

                // get the input dataset for count and write subsetted dataset
                // to output
                H5::DataSet inCountDs =
                    infile.openGroup(groupname).openDataSet(countName);
                Subsetter::writeDataset(
                    countName, inCountDs, outgroup, groupname, indexes);
                // if the count dataset still doesn't exist, don't write index
                // begin
                if (H5Lexists(outgroup.getLocId(),
                              countName.c_str(),
                              H5P_DEFAULT) <= 0)
                    return;
            }
            // write index begin dataset
            FwdRefBeginDataset *photonDataset =
                new FwdRefBeginDataset(this->getShortName(), objname, config);
            photonDataset->writeDataset(outgroup,
                                        groupname,
                                        indataset,
                                        indexes,
                                        this->getSubsetDataLayers());

            // copy attributes
            H5::DataSet outdataset(outgroup.openDataSet(objname));
            copyAttributes(indataset, outdataset, groupname);
        }
        else
        {
            Subsetter::writeDataset(
                objname, indataset, outgroup, groupname, indexes);
        }
    }

  private:
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
        LOG_DEBUG("SuperGroupSubsetter::getCoordinate(): ENTER groupname: "
                  << groupname);

        bool hasPhotonSegmentDataset =
            config->hasPhotonSegmentGroups(this->getShortName());
        bool isPhotonDataset =
            config->isPhotonDataset(this->getShortName(), groupname);
        if (hasPhotonSegmentDataset && isPhotonDataset)
        {
            return ForwardReferenceCoordinates::getCoordinate(
                root,
                ingroup,
                this->getShortName(),
                subsetDataLayers,
                groupname,
                geoboxes,
                temporal,
                geoPolygon,
                config);
        }
        else if (groupname != "/")
        {
            return SuperGroupCoordinate::getCoordinate(root,
                                                       ingroup,
                                                       this->getShortName(),
                                                       subsetDataLayers,
                                                       groupname,
                                                       geoboxes,
                                                       temporal,
                                                       geoPolygon,
                                                       config);
        }
        else
        {
            return Subsetter::getCoordinate(root,
                                            ingroup,
                                            groupname,
                                            subsetDataLayers,
                                            geoboxes,
                                            temporal,
                                            geoPolygon,
                                            config);
        }
    }
};
#endif
