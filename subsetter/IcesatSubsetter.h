#ifndef IcesatSubsetter_H
#define IcesatSubsetter_H

#include <stdlib.h>
#include <string.h>
#include <vector>

#include "Configuration.h"
#include "FwdRefBeginDataset.h"
#include "IndexSelection.h"
#include "LogLevel.h"
#include "ReverseReferenceCoordinates.h"
#include "RvsRefDatasets.h"
#include "SubsetDataLayers.h"
#include "Subsetter.h"
#include "Temporal.h"

#include "H5Cpp.h"

// ICESAT2 specific implementation
class IcesatSubsetter : public Subsetter
{
  public:
    IcesatSubsetter(SubsetDataLayers *subsetDataLayers,
                    std::vector<geobox> *geoboxes,
                    Temporal *temporal,
                    GeoPolygon *geoPolygon,
                    Configuration *config)
        : Subsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, config)
    {
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
        LOG_DEBUG(
            "IcesatSubsetter::writeDataset(): ENTER groupname: " << groupname);

        // write index begin datasets for ATL03 and ATL08
        // if it's segment group with ph_index_beg and it's been subsetted,
        // we have to write this dataset with updated value calculated from
        // segment_ph_cnt

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
        // write index begin for ATL10 if it has been subsetted
        else if (indexes != NULL &&
                 (indexes->getMaxSize() != indexes->size() ||
                  indexes->size() == 1) &&
                 indexes->size() != 0 &&
                 (config->isLeadsGroup(this->getShortName(), groupname) ||
                  config->isFreeboardSwathSegmentGroup(this->getShortName(),
                                                       groupname) ||
                  config->isFreeboardBeamSegmentGroup(this->getShortName(),
                                                      groupname) ||
                  config->isHeightSegmentRateGroup(this->getShortName(),
                                                   groupname) ||
                  config->isFreeboardSegmentGroup(this->getShortName(),
                                                  groupname) ||
                  config->isReferenceSurfaceSectionGroup(this->getShortName(),
                                                         groupname) ||
                  config->isFreeboardRateGroup(this->getShortName(),
                                               groupname) ||
                  config->isFreeboardSegmentHeightsGroup(this->getShortName(),
                                                         groupname) ||
                  config->isFreeboardSegmentGeophysicalGroup(
                      this->getShortName(), groupname)) &&
                 config->getIndexBeginDatasetName(
                     this->getShortName(), groupname, objname, true) == objname)
        {
            RvsRefDatasets *referenceDataset =
                new RvsRefDatasets(this->getShortName(), objname);

            // get target group index selection
            std::string targetGroupname = config->getTargetGroupname(
                this->getShortName(), groupname, objname);
            LOG_DEBUG("IcesatSubsetter::writeDataset() groupname: "
                      << groupname << " objname: " << objname
                      << " targetGroupname: " << targetGroupname);

            // if target group does not exist in input, write index begin as
            // normal dataset
            if (H5Lexists(infile.getLocId(),
                          targetGroupname.c_str(),
                          H5P_DEFAULT) <= 0)
            {
                Subsetter::writeDataset(
                    objname, indataset, outgroup, groupname, indexes);
                return;
            }

            IndexSelection *targetIndexes;
            H5::Group root = infile.openGroup("/");
            H5::Group targetGroup = root.openGroup(targetGroupname);
            Coordinate *coor;
            if (Coordinate::lookUp(targetGroupname))
                coor = Coordinate::lookUpMap[targetGroupname];
            else
                coor =
                    IcesatSubsetter::getCoordinate(root,
                                                   targetGroup,
                                                   targetGroupname,
                                                   this->getSubsetDataLayers(),
                                                   this->getGeobox(),
                                                   this->getTemporal(),
                                                   this->getGeoPolygon(),
                                                   config,
                                                   true);
            if (coor->indexesProcessed)
                targetIndexes = coor->indexes;
            else
                targetIndexes = coor->getIndexSelection();

            // write index begin
            referenceDataset->mapWriteDataset(outgroup,
                                              groupname,
                                              indataset,
                                              indexes,
                                              targetIndexes,
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
        LOG_DEBUG(
            "IcesatSubsetter::getCoordinate(): ENTER groupname: " << groupname);

        bool hasPhotonSegmentGroup =
            config->hasPhotonSegmentGroups(this->getShortName());
        bool isPhotonGroup =
            config->isPhotonGroup(this->getShortName(), groupname);
        bool isLeadsGroup =
            config->isLeadsGroup(this->getShortName(), groupname);
        bool isHeightSegmentRateGroup =
            config->isHeightSegmentRateGroup(this->getShortName(), groupname);
        bool isFreeboardBeamSegmentGroup = config->isFreeboardBeamSegmentGroup(
            this->getShortName(), groupname);
        bool isHeightsGroup =
            config->isHeightsGroup(this->getShortName(), groupname);
        bool isGeophysicalGroup =
            config->isGeophysicalGroup(this->getShortName(), groupname);
        bool isFreeboardSegmentGroup =
            config->isFreeboardSegmentGroup(this->getShortName(), groupname);
        bool isReferenceSurfaceSectionGroup =
            config->isReferenceSurfaceSectionGroup(this->getShortName(),
                                                   groupname);
        bool isFreeboardRateGroup =
            config->isFreeboardRateGroup(this->getShortName(), groupname);
        bool isFreeboardSegmentHeightsGroup =
            config->isFreeboardSegmentHeightsGroup(this->getShortName(),
                                                   groupname);
        bool isFreeboardSegmentGeophysicalGroup =
            config->isFreeboardSegmentGeophysicalGroup(this->getShortName(),
                                                       groupname);
        bool isSubsetDataLayers = subsetDataLayers->is_included(groupname);

        bool freeboardSwathSegment =
            checkFreeboardSwathSegmentExists(root, groupname);
        config->setFreeboardSwathSegment(freeboardSwathSegment);

        if (hasPhotonSegmentGroup && (isPhotonGroup || isLeadsGroup) &&
            (subsetDataLayers->is_included(groupname) || repair))
        {
            LOG_DEBUG("IcesatSubsetter::getCoordinate(): Call "
                      "ForwardReferenceCoordinates::getCoordinate(): "
                      << groupname);
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
        else if (hasPhotonSegmentGroup &&
                     (isHeightSegmentRateGroup ||
                      isFreeboardBeamSegmentGroup && freeboardSwathSegment) ||
                 isFreeboardRateGroup && (isSubsetDataLayers || repair))
        {
            std::string coorGroupname = groupname;
            H5::Group coorGroup = ingroup;

            if (isHeightsGroup || isGeophysicalGroup)
            {
                coorGroupname = config->getBeamFreeboardGroup(
                    this->getShortName(), groupname);
                coorGroup = root.openGroup(coorGroupname);
            }
            else if (isFreeboardSegmentHeightsGroup ||
                     isFreeboardSegmentGeophysicalGroup)
            {
                coorGroupname = config->getFreeboardSegmentGroup(
                    this->getShortName(), groupname);
                coorGroup = root.openGroup(coorGroupname);
            }

            LOG_DEBUG(
                "IcesatSubsetter::getCoordinate(): Call "
                "ReverseReferenceCoordinates::getCoordinate(): groupname: "
                << groupname);

            return ReverseReferenceCoordinates::getCoordinate(
                root,
                coorGroup,
                this->getShortName(),
                subsetDataLayers,
                coorGroupname,
                geoboxes,
                temporal,
                geoPolygon,
                config);
        }
        else if (config->subsetBySuperGroup(this->getShortName(), groupname))
        {
            LOG_DEBUG("IcesatSubsetter::getCoordinate(): Call "
                      "SuperGroupCoordinate::getCoordinate(): groupname: "
                      << groupname);
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
            LOG_DEBUG("IcesatSubsetter::getCoordinate(): Call "
                      "Subsetter::getCoordinate(): groupname: "
                      << groupname);
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

    /**
     * check to see if freeboard swath segment exists
     * @param root Group object for root group
     * @param groupname string group name
     */
    bool checkFreeboardSwathSegmentExists(H5::Group &root,
                                          const std::string &groupname)
    {
        bool exist = true;

        std::string freeboardSwathSegmentName =
            config->getSwathSegmentGroup(this->getShortName(), groupname);

        if (freeboardSwathSegmentName == "" ||
            H5Lexists(root.getLocId(),
                      freeboardSwathSegmentName.c_str(),
                      H5P_DEFAULT) == 0)
        {
            exist = false;
        }

        return exist;
    }
};
#endif
