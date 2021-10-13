#ifndef DatasetLinks_H 
#define DatasetLinks_H

#include <map>
#include <iostream>
#include <string.h>

/**
 * class to track the link information within a group
 */
class DatasetLinks
{
public:    
    DatasetLinks()
    {
        links = new map<haddr_t, string>();
        hardlinks = new map<string, string>();
    }

    ~DatasetLinks()
    {
        delete links;
        delete hardlinks;
    }

    /**
     * call back function for links
     * save the address and the first dataset which points to this address, and,
     * if the address has a dataset, point the current dataset to the previous dataset 
     * groupId: id of the group 
     * name: link name
     * linfo: link information
     * opdata: user-defined pointer of data, here is datasetlinks
     */
    static herr_t linkCallback(hid_t groupId, const char *name, const H5L_info_t *linfo, void *opdata)
    {
        DatasetLinks* datasetlinks = (DatasetLinks*) opdata;
        map<haddr_t, string>* links = datasetlinks->links;
        map<string, string>* hardlinks = datasetlinks->hardlinks;

        // save off the mapping for new address
        if (linfo->type == 0) // hardlink
        {
            haddr_t address = linfo->u.address;
            // if the address not already in the map, save it
            if (links->find(address) == links->end())
            {
                links->insert(pair<haddr_t, string>(address, name));
                //cout << "links address:" << address << ", name=" << name << endl;
            }
            else // the address has a dataset, the current dataset can then point to that dataset
            {
                string sourceDataset = links->find(address)->second;
                cout << "LINK found: type=" << linfo->type << " target " << name << " source " << sourceDataset << endl;
                hardlinks->insert(pair<string, string>(name, sourceDataset));
            }
        }
        return 0;
    }

    /**
     * track link information within a group
     * @param group group to visit
     */
    void trackDatasetLinks(const Group& group)
    {
        H5Literate(group.getLocId(), H5_INDEX_NAME, H5_ITER_INC, NULL, DatasetLinks::linkCallback, (void*)this);
    }
        
    /**
     * check if the object is a hard link
     * @param objname 
     * @return true if the object is a hard link
     */
    bool isHardLink(const string& objname)
    {
        map<string, string>::iterator it = hardlinks->find(objname);
        if (it != hardlinks->end()) return true;
    }
    
    /**
     * get the hard link source if exists
     * @param objname
     * @return 
     */
    string getHardLinkSource(const string& objname)
    {
        map<string, string>::iterator it = hardlinks->find(objname);
        return (it != hardlinks->end())? it->second : string();
    }
        
private:
    /** 
     * keep map of address to the first object name which points to this address,
     * could contain objects other than dataset
     * key: address, value: dataset
     */
    map<haddr_t, string>* links;

    /** 
     * keep track of the dataset names that point to a previous dataset in a group,
     * we haven't seen dataset link to dataset in different group
     *key: target dataset, value: source dataset
     */
    map<string, string>* hardlinks;
};
#endif
