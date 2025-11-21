#ifndef DatasetLinks_H
#define DatasetLinks_H

#include "H5Cpp.h"
#include "LogLevel.h"
#include <iostream>
#include <map>
#include <string.h>

/**
 * class to track the link information within a group
 */
class DatasetLinks
{
  public:
    DatasetLinks()
    {
        links = new std::map<haddr_t, std::string>();
        hardlinks = new std::map<std::string, std::string>();
    }

    ~DatasetLinks()
    {
        delete links;
        delete hardlinks;
    }

    /**
     * call back function for links
     * save the address and the first dataset which points to this address, and,
     * if the address has a dataset, point the current dataset to the previous
     * dataset groupId: id of the group name: link name linfo: link information
     * opdata: user-defined pointer of data, here is datasetlinks
     */
    static herr_t linkCallback(hid_t groupId,
                               const char *name,
                               const H5L_info_t *linfo,
                               void *opdata)
    {
        DatasetLinks *datasetlinks = (DatasetLinks *)opdata;
        std::map<haddr_t, std::string> *links = datasetlinks->links;
        std::map<std::string, std::string> *hardlinks = datasetlinks->hardlinks;

        // save off the mapping for new address
        if (linfo->type == 0) // hardlink
        {
            haddr_t address = linfo->u.address;
            // if the address not already in the map, save it
            if (links->find(address) == links->end())
            {
                links->insert(std::pair<haddr_t, std::string>(address, name));
            }
            else // the address has a dataset, the current dataset can then
                 // point to that dataset
            {
                std::string sourceDataset = links->find(address)->second;
                LOG_DEBUG("DatasetLinks::linkCallback() LINK found: type="
                          << linfo->type << " target " << name << " source "
                          << sourceDataset);
                hardlinks->insert(
                    std::pair<std::string, std::string>(name, sourceDataset));
            }
        }
        return 0;
    }

    /**
     * track link information within a group
     * @param group group to visit
     */
    void trackDatasetLinks(const H5::Group &group)
    {
        LOG_DEBUG("DatasetLinks::trackDatasetLinks(): ENTER");

        H5Literate(group.getLocId(),
                   H5_INDEX_NAME,
                   H5_ITER_INC,
                   NULL,
                   DatasetLinks::linkCallback,
                   (void *)this);
    }

    /**
     * check if the object is a hard link
     * @param objname
     * @return true if the object is a hard link
     */
    bool isHardLink(const std::string &objname)
    {
        std::map<std::string, std::string>::iterator it =
            hardlinks->find(objname);
        return (it != hardlinks->end());
    }

    /**
     * get the hard link source if exists
     * @param objname
     * @return
     */
    std::string getHardLinkSource(const std::string &objname)
    {
        std::map<std::string, std::string>::iterator it =
            hardlinks->find(objname);
        return (it != hardlinks->end()) ? it->second : std::string();
    }

  private:
    /**
     * keep map of address to the first object name which points to this
     * address, could contain objects other than dataset key: address, value:
     * dataset
     */
    std::map<haddr_t, std::string> *links;

    /**
     * keep track of the dataset names that point to a previous dataset in a
     * group, we haven't seen dataset link to dataset in different group key:
     * target dataset, value: source dataset
     */
    std::map<std::string, std::string> *hardlinks;
};
#endif
