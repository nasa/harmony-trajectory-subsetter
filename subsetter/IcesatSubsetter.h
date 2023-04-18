#ifndef IcesatSubsetter_H
#define IcesatSubsetter_H 

#include <string.h>
#include <stdlib.h>
#include <vector>

#include "Temporal.h"
#include "Configuration.h"
#include "FwdRefBeginDataset.h"
#include "RvsRefDatasets.h"
#include "ReverseReferenceCoordinates.h"


// ICESAT2 specific implementation
class IcesatSubsetter : public Subsetter
{
public:
    IcesatSubsetter(SubsetDataLayers* subsetDataLayers, std::vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon)
    : Subsetter(subsetDataLayers, geoboxes, temporal, geoPolygon) 
    {
        std::cout << "IcesatSubsetter ctor" << std::endl;
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
    virtual void writeDataset(const std::string& objname, const H5::DataSet& indataset, H5::Group& outgroup, 
                        const std::string& groupname, IndexSelection* indexes)
    {
        // write index begin datasets for ATL03 and ATL08
        // if it's segment group with ph_index_beg and it's been subsetted,
        // we have to write this dataset with updated value calculated from segment_ph_cnt
        //if (indexes != NULL) std::cout << "IcesatSubsetter.writeDataset -- indexes.size(): " << indexes->size() << std::endl;
        H5::H5File infile = getInputFile();
        if (indexes != NULL && indexes->getMaxSize() != indexes->size() && indexes->size() != 0 &&
            Configuration::getInstance()->isSegmentGroup(this->getShortName(), groupname) && 
            Configuration::getInstance()->getIndexBeginDatasetName(this->getShortName(), groupname, objname) ==objname)
        {
            // need to make sure count dataset exists
            std::string countName = Configuration::getInstance()->getCountDatasetName(this->getShortName(), groupname, objname);
            //std::cout << "groupname+countName: " << groupname+countName << std::endl;
            // if the count dataset is not in the output file, create it
            if (H5Lexists(outgroup.getLocId(), countName.c_str(), H5P_DEFAULT) <= 0)
            {
                // if the input file doesn't have the count dataset, write index begin as normal dataset
                if (H5Lexists(infile.getLocId(), (groupname+countName).c_str(), H5P_DEFAULT) <= 0)
                {
                    Subsetter::writeDataset(objname, indataset, outgroup, groupname, indexes);
                    return;
                }

                // get the input dataset for count and write subsetted dataset to output
                H5::DataSet inCountDs = infile.openGroup(groupname).openDataSet(countName);
                Subsetter::writeDataset(countName, inCountDs, outgroup, groupname, indexes);
                // if the count dataset still doesn't exist, don't write index begin
                if (H5Lexists(outgroup.getLocId(), countName.c_str(), H5P_DEFAULT) <= 0)
                    return;
            }
            // write index begin dataset
            FwdRefBeginDataset* photonDataset = new FwdRefBeginDataset(this->getShortName(), objname);
            photonDataset->writeDataset(outgroup, groupname, indataset, indexes, this->getSubsetDataLayers());
            
            
            // copy attributes
            H5::DataSet outdataset(outgroup.openDataSet(objname));
            copyAttributes(indataset, outdataset, groupname);
        }
        // write index begin for ATL10 if it has been subsetted
        else if (indexes != NULL && (indexes->getMaxSize() != indexes->size() ||indexes->size() == 1) && indexes->size() != 0 &&
                (Configuration::getInstance()->isLeadsGroup(this->getShortName(), groupname) ||
                Configuration::getInstance()->isFreeboardSwathSegmentGroup(this->getShortName(), groupname) ||
                Configuration::getInstance()->isFreeboardBeamSegmentGroup(this->getShortName(), groupname) ||
                Configuration::getInstance()->isHeightSegmentRateGroup(this->getShortName(), groupname)) && 
                Configuration::getInstance()->getIndexBeginDatasetName(this->getShortName(), groupname, objname, true) ==objname)
        {
            RvsRefDatasets* referenceDataset = new RvsRefDatasets(this->getShortName(), objname);
            
            // get target group index selection
            std::string targetGroupname = Configuration::getInstance()->getTargetGroupname(this->getShortName(), groupname, objname);
                        
            // if target group does not exist in input, write index begin as normal dataset
            if (H5Lexists(infile.getLocId(), targetGroupname.c_str(), H5P_DEFAULT) <= 0)
            {
                Subsetter::writeDataset(objname, indataset, outgroup, groupname, indexes);
                return;
            }
            
            IndexSelection* targetIndexes;
            H5::Group root = infile.openGroup("/");
            H5::Group targetGroup = root.openGroup(targetGroupname);
            Coordinate* coor;
            if (Coordinate::lookUp(targetGroupname)) coor = Coordinate::lookUpMap[targetGroupname];
            else coor = IcesatSubsetter::getCoordinate(root, targetGroup, targetGroupname, this->getSubsetDataLayers(),
                    this->getGeobox(), this->getTemporal(), this->getGeoPolygon(), true);
            if (coor->indexesProcessed) targetIndexes = coor->indexes;
            else targetIndexes = coor->getIndexSelection();
            
            //std::cout << "targetIndexes.size: " << targetIndexes->size() << std::endl;
            
            // write index begin
            referenceDataset->mapWriteDataset(outgroup, groupname, indataset, indexes, targetIndexes, this->getSubsetDataLayers());
            
            // copy attributes
            H5::DataSet outdataset(outgroup.openDataSet(objname));
            copyAttributes(indataset, outdataset, groupname);
        }
        else
        {
            Subsetter::writeDataset(objname, indataset, outgroup, groupname, indexes);
        } 
    }

private:
    
    virtual Coordinate* getCoordinate(H5::Group& root, H5::Group& ingroup, const std::string& groupname, 
        SubsetDataLayers* subsetDataLayers, std::vector<geobox>* geoboxes, Temporal* temporal, GeoPolygon* geoPolygon, bool repair = false)
    {
        std::cout << "IcesatSubsetter getCoordinate" << std::endl;
        std::cout << "groupname: " << groupname << std::endl;
        bool hasPhotonSegmentGroup = Configuration::getInstance()->hasPhotonSegmentGroups(this->getShortName());
        bool isPhotonGroup = Configuration::getInstance()->isPhotonGroup(this->getShortName(), groupname);
        bool isLeadsGroup = Configuration::getInstance()->isLeadsGroup(this->getShortName(), groupname);
        bool isHeightSegmentRateGroup = Configuration::getInstance()->isHeightSegmentRateGroup(this->getShortName(), groupname);
        bool isFreeboardBeamSegmentGroup = Configuration::getInstance()->isFreeboardBeamSegmentGroup(this->getShortName(), groupname);
        bool isHeightsGroup = Configuration::getInstance()->isHeightsGroup(this->getShortName(), groupname);
        bool isGeophysicalGroup = Configuration::getInstance()->isGeophysicalGroup(this->getShortName(), groupname);
        
        bool freeboardSwathSegment = checkFreeboardSwathSegmentExists(root, groupname);
        Configuration::getInstance()->setFreeboardSwathSegment(freeboardSwathSegment);
                
        if (hasPhotonSegmentGroup && (isPhotonGroup || isLeadsGroup) && (subsetDataLayers->is_included(groupname) || repair))
        {
            return ForwardReferenceCoordinates::getCoordinate(root, ingroup, this->getShortName(), subsetDataLayers, groupname, 
                    geoboxes, temporal, geoPolygon);
        }
        else if (hasPhotonSegmentGroup && (isHeightSegmentRateGroup || isFreeboardBeamSegmentGroup && freeboardSwathSegment)
                && (subsetDataLayers->is_included(groupname) || repair))
        {
            std::string coorGroupname = groupname;
            H5::Group coorGroup = ingroup;
            
            if (isHeightsGroup || isGeophysicalGroup)
            {
                coorGroupname = Configuration::getInstance()->getBeamFreeboardGroup(this->getShortName(), groupname);
                coorGroup = root.openGroup(coorGroupname);
            }
            return ReverseReferenceCoordinates::getCoordinate(root, coorGroup, this->getShortName(), subsetDataLayers, 
                    coorGroupname, geoboxes, temporal, geoPolygon);
        }
        else if (Configuration::getInstance()->subsetBySuperGroup(this->getShortName(), groupname))
        {
            std::cout << "subset by super H5::Group" << std::endl;
            return SuperGroupCoordinate::getCoordinate(root, ingroup, this->getShortName(), subsetDataLayers, 
                    groupname, geoboxes, temporal, geoPolygon);
        }
        else
        {
            return Subsetter::getCoordinate(root, ingroup, groupname, subsetDataLayers, geoboxes, temporal, geoPolygon);
        }
    }
    
    /**
     * check to see if freeboard swath segment exists
     * @param root Group object for root group
     * @param groupname string group name
     */
    bool checkFreeboardSwathSegmentExists(H5::Group& root, const std::string& groupname)
    {
        //std::cout << "checkFreeboardSwathSegmentExists" << std::endl;
        bool exist = true;
        
        std::string freeboardSwathSegmentName = Configuration::getInstance()->getSwathSegmentGroup(this->getShortName(), groupname);
                
        if (freeboardSwathSegmentName == "" || H5Lexists(root.getLocId(), freeboardSwathSegmentName.c_str(), H5P_DEFAULT) == 0)
        {
            exist = false;
        }

        return exist;
    }
};
#endif
