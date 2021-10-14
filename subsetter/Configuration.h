#ifndef Configuration_H
#define Configuration_H

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional/optional.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <algorithm>
#include <exception>
#include <map>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <iostream>

using namespace std;
using namespace boost;
namespace pt = boost::property_tree;

// class to handle configuration information
class Configuration
{
public:

    /**
     * get singleton instance
     * @return 
     */
    static Configuration* getInstance()
    {
        if (!myInstance)
            myInstance = new Configuration();
        return myInstance;
    }
    
    /**
     * destroy instance
     */
    static void destroyInstance()
    {
	if (myInstance) delete myInstance;
        myInstance = NULL;
    }
 
    /**
     * initialize the information from json configuration file
     * @param configFile the configuration file
     */
    void initialize(const string& configFile)
    {
        pt::ptree root;
        string s;
        stringstream ss;
        try
        {
            s = removeComment(configFile);
            ss << s;
            pt::read_json(ss, root);

            // coordinate dataset names for each shortname pattern
            if (root.get_child_optional("CoordinateDatasetNames"))
            {
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("CoordinateDatasetNames"))
                {
                    string shortNamePattern = nodei.first;
                    map<string, vector < string>> datasets;
                    // list of possible names for each type (time, latitude, longitude)

                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string datasetType = nodej.first;
                        vector<string> names;

                        BOOST_FOREACH(pt::ptree::value_type &nodek, nodej.second)
                        {
                            //cout << "list of dataset names " << shortNamePattern
                            //     << " " << datasetType << " " << nodek.second.get_value<string>() << endl;
                            names.push_back(nodek.second.get_value<string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    coordinateDatasetNames.insert(make_pair(shortNamePattern, datasets));
                }
            }

            // get product epoch
            if (root.get_child_optional("ProductEpochs"))
            {
                BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("ProductEpochs"))
                {
                    //cout << "ProductEpochs " << v.first << " " << v.second.data() << endl;
                    productEpochs.insert(make_pair(v.first, v.second.data()));
                }
            }

            // get subsettable group patterns
            if (root.get_child_optional("SubsettableGroups"))
            {
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("SubsettableGroups"))
                {
                    string shortNamePattern = nodei.first;
                    vector<string> groupPatterns;

                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string value = nodej.second.get_value<string>();
                        groupPatterns.push_back(value);
                        //cout << "SubsettableGroups " << shortNamePattern << " " << value << endl;
                    }
                    subsettableGroups.insert(make_pair(shortNamePattern, groupPatterns));
                }
            }

            // get unsubsettable group patterns
            if (root.get_child_optional("UnsubsettableGroups"))
            {
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("UnsubsettableGroups"))
                {
                    string shortNamePattern = nodei.first;
                    vector<string> groupPatterns;

                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string value = nodej.second.get_value<string>();
                        groupPatterns.push_back(value);
                        //cout << "UnsubsettableGroups " << shortNamePattern << " " << value << endl;
                    }
                    unsubsettableGroups.insert(make_pair(shortNamePattern, groupPatterns));
                }
            }
            
            // get PhotonSegment group patterns
            if (root.get_child_optional("PhotonSegmentGroups"))
            {               
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("PhotonSegmentGroups"))
                {
                    string shortNamePattern = nodei.first;
                    map<string, string> keyinfo;
                    // list of keys and their value
                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string key = nodej.first;
                        string value = nodej.second.get_value<string>();                        
                        keyinfo.insert(make_pair(key, value));
                    }
                    photonSegmentGroups.insert(make_pair(shortNamePattern, keyinfo));
                }
            }
            
            // get shortname attribute path
            if (root.get_child_optional("ShortNamePath"))
            {                
                BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("ShortNamePath"))
                {
                    cout << "shortname path " << v.second.data() << endl;
                    //shortnames.insert(make_pair(v.first, v.second.data()));
                    shortnames.push_back(v.second.data());
                }
            }
            
            // get mission from shortname
            if (root.get_child_optional("Missions"))
            {
                BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("Missions"))
                {
                    //cout << "ProductEpochs " << v.first << " " << v.second.data() << endl;
                    missions.insert(make_pair(v.first, v.second.data()));
                }
            }
            
            if (root.get_child_optional("SuperGroupCoordinates"))
            {
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("SuperGroupCoordinates"))
                {
                    string shortNamePattern = nodei.first;
                    map<string, vector < string>> datasets;
                    // list of possible names for each type (time, latitude, longitude)

                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string datasetType = nodej.first;
                        vector<string> names;

                        BOOST_FOREACH(pt::ptree::value_type &nodek, nodej.second)
                        {
                            //cout << "list of dataset names " << shortNamePattern
                            //     << " " << datasetType << " " << nodek.second.get_value<string>() << endl;
                            names.push_back(nodek.second.get_value<string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    superCoordinateDatasets.insert(make_pair(shortNamePattern, datasets));
                }
            }
            
            if (root.get_child_optional("SuperGroups"))
            {
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("SuperGroups"))
                {
                    string shortNamePattern = nodei.first;
                    map<string, vector < string>> datasets;

                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string datasetType = nodej.first;
                        vector<string> names;

                        BOOST_FOREACH(pt::ptree::value_type &nodek, nodej.second)
                        {
                            //cout << "list of dataset names " << shortNamePattern
                            //     << " " << datasetType << " " << nodek.second.get_value<string>() << endl;
                            names.push_back(nodek.second.get_value<string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    superGroups.insert(make_pair(shortNamePattern, datasets));
                }
            }
            
            if (root.get_child_optional("RequiredDatasetsByFormat"))
            {
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("RequiredDatasetsByFormat"))
                {
                    string format = nodei.first;
                    map<string, vector<string>> datasets;
                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string datasetType = nodej.first;
                        vector<string> names;
                        
                        // list of possible lat/lon/row/column datasets
                        BOOST_FOREACH(pt::ptree::value_type &nodek, nodej.second)
                        {
                            //cout << "list of dataset names " << shortNamePattern
                            //     << " " << datasetType << " " << nodek.second.get_value<string>() << endl;
                            names.push_back(nodek.second.get_value<string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    requiredDatasets.insert(make_pair(format, datasets));
                    
                }
            }
            
            if (root.get_child_optional("Resolution"))
            {
                BOOST_FOREACH(pt::ptree::value_type &nodei, root.get_child("Resolution"))
                {
                    string groupnamePattern = nodei.first;
                    map<string, string> keyinfo;
                    // list of keys and their value
                    BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
                    {
                        string key = nodej.first;
                        string value = nodej.second.get_value<string>();                        
                        keyinfo.insert(make_pair(key, value));
                    }
                    resolutions.insert(make_pair(groupnamePattern, keyinfo));
                }
            }
            
            if (root.get_child_optional("Projections"))
            {
                BOOST_FOREACH(pt::ptree::value_type &v, root.get_child("Projections"))
                {
                    projections.insert(make_pair(v.first, v.second.data()));
                }
            }
        }
        catch (std::exception &ex)
        {
            cerr << "ERROR: failed to parse the configuration file " << configFile << endl;
            std::exception_ptr p = current_exception();
            rethrow_exception(p);
        }
    }
    
    /**
     * removes comments in SubsetterConfig.json
     * Boost does not support comments in JSON files after 1.59
     * @param configFile the configuration file
     * @return json string without comment
     */
    string removeComment(const string& configFile)
    {
        cout << "removeComment" << endl;
        std::ifstream is(configFile);
        string s, str;
        s.reserve(is.rdbuf()->in_avail());
        char c;
        
        while(is.get(c))
        {
            if(s.capacity() == s.size())
                s.reserve(s.capacity() *3);
            s.append(1, c);
        } 
        
        // regular expression to match comments
        string commentPattern = "(//[^\\n]*|/\\*.*?\\*/)";
        str = regex_replace(s, regex(commentPattern), "");
        //cout << str << endl;
        return str;
        
    }
    
    /**
     * 
     */
    void setFreeboardSwathSegment(bool exist)
    {
        freeboardSwathSegment = exist;
    }
    
    /**
     * @return boolean value whether the freeboard swath segment group exists in the input file
     */
    bool freeboardSwathSegmentExist()
    {
        return freeboardSwathSegment;
    }
    
    /**
     * get shortname attribute path
     * @return shortname attribute paths
     */
    vector<string> getShortnamePath()
    {
        return shortnames;
    }
    
    /**
     * get mission name from product shortname
     * @param shortname product shortname
     * @return mission mission name
     */
    string getMissionFromShortName(const string& shortname)
    {
        string mission;
        
        for (map<string, string>::iterator it = missions.begin(); it != missions.end(); it++)
        {
            if (regex_match(shortname, regex(it->first)))
            {
                mission = it->second;
                break;
            }
        }
        
        return mission;
    }

    /**
     * get super coordinate dataset names
     * @param shortname product shortname
     * @param groupname group name
     * @param super group group name
     * @return super coordinate names
     */
    map<string, vector<string>> getSuperCoordinates(const string& shortname)
    {
        map<string, vector<string>> datasets;
        for (map<string, map<string, vector<string>>>::iterator it = superCoordinateDatasets.begin();
                it != superCoordinateDatasets.end(); it++)
        {
            if (regex_match(shortname, regex(it->first)))
            {
                datasets = it->second;
                break;
            }
        }
        return datasets;
    }
    
    /**
     * get super group names
     * @param shortname product name
     * @groupname group name
     * @param super group group names
     */
    void getSuperCoordinateGroupname(const string& shortname, string groupname, vector<string>& superGroupnames)
    {
        for (map<string, map<string, vector<string>>>::iterator it = superGroups.begin();
                it != superGroups.end(); it++)
        {
            if (regex_match(shortname, regex(it->first)))
            {
                string pathDelim("/");
                vector<string> paths;
                split(paths, groupname, is_any_of(pathDelim));
                string groupNum = paths.at(1);
                paths.clear();
                for(map<string, vector<string>>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
                {
                    for (vector<string>::iterator it3 = it2->second.begin(); it3 != it2->second.end(); it3++)
                    {
                        split(paths, *it3, is_any_of(pathDelim));
                        string groupPattern = paths.at(1);
                        paths.clear();
                        string superGroupname = *it3;
                        replace_last(superGroupname, groupPattern, groupNum);
                        superGroupnames.push_back(superGroupname);
                    }
                }
                break;
            }
        }
    }
    
    /**
     * get the required datasets for the output format and pixel resolution
     * @param format - output format
     * @param groupname - group name
     * @param shortname - short name
     * @param resolution - pixel resolution
     */
    void getRequiredDatasetsAndResolution(const string& format, const string& groupname, const string& shortName,
         string& rowName, string& colName, string& latName, string& lonName, short& resolution)
    {
        vector<string> datasets = getRequiredDatasetsByFormat(format, groupname, shortName, resolution);
        string datasetName;
        
        for (vector<string>::iterator it = datasets.begin(); it != datasets.end(); it++)
        {
            datasetName = *it;
            if(datasetName.find("row") != string::npos)
                rowName = datasetName;
            if(datasetName.find("col") != string::npos)
                colName = datasetName;
            if(datasetName.find("lat") != string::npos)
                latName = datasetName;
            if(datasetName.find("lon") != string::npos)
                lonName = datasetName;
        }
    }
    
    /**
     * get lat/lon/row/column datasets
     * @param format output format
     * @param group name
     * @return lat/lon/row/column datasets names
     */
     vector<string> getRequiredDatasetsByFormat(const string& format, const string& groupname, const string& shortname, short& resolution)
    {
        vector<string> datasets;
        for (map<string, map<string, vector<string>>>::iterator it = requiredDatasets.begin();
                it != requiredDatasets.end(); it++)
        {
            if (regex_match(format, regex(it->first)))
            {
                for (map<string, vector<string>>::iterator map_it = it->second.begin(); map_it != it->second.end(); map_it++)
                {
                    if(regex_match(groupname, regex(map_it->first)))
                    {
                        datasets = map_it->second;
                        getResolution(groupname, shortname, resolution);
                        break;
                    }
                }
                if (datasets.size() == 0)
                {
                    datasets = it->second["DEFAULT"];
                    getResolution(groupname, shortname, resolution);
                }
            }
        }
        return datasets;
    }
     
    /**
     * get the pixel resolution
     * @param groupname - group name
     * @param shortname - short name
     * @param resolution - pixel resolution
     */
    void getResolution(const string& groupname, const string shortname, short& resolution)
    {
        for(map<string, map<string, string>>::iterator it = resolutions.begin(); it != resolutions.end(); it++)
        {
            if (regex_match(groupname, regex(it->first)))
            {
                for(map<string, string>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
                {
                    if(regex_match(shortname, regex(it2->first)))
                    {
                       resolution = lexical_cast<short>(it2->second);
                       break;
                    }
                }
            }
        }
    }

    /**
     * get projection for geotiff format by group name
     * @param groupname - group name
     * @return 
     */
    string getProjection(const string& groupname)
    {
        string projection;
        
        for(map<string, string>::iterator it = projections.begin(); it != projections.end(); it++)
        {
            if (regex_match(groupname, regex(it->first)))
            {
                projection = it->second;
                break;
            }
        }
        
        if (projection.empty())
        {
            projection = projections["DEFAULT"];
        }
        
        return projection;
    }
    
    /**
     * Check if a group is subsettable based on the configuration.
     * If no configuration found for this shortName, the group is subsettable
     * @param shortName shortname for this product
     * @param group group name including path
     * @return true if the group is subsettable
     */
    bool isGroupSubsettable(const string& shortName, const string& group)
    {
        bool isSubsettable = true;
        //// first check subsettable groups
        // check if it's configured to be subsettable
        bool shortNameMatched = false, patternMatched = false;
        for (map<string, vector<string>>::iterator it = subsettableGroups.begin();
             it != subsettableGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            vector<string> groupPatterns = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                shortNameMatched = true;
                patternMatched = matchRegexpList(groupPatterns, group);
                break;
            }
        }
        // if the shortName is configured and group is not specified, the group is not subsettable        
        if (shortNameMatched && !patternMatched)
        {               
            cout << group << " isn't subsettable " << endl;
            return false;
        }
       
        //// second check unsubsettable group        
        for (map<string, vector<string>>::iterator it = unsubsettableGroups.begin();
             it != unsubsettableGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            vector<string> groupPatterns = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {   
                if (matchRegexpList(groupPatterns, group)) isSubsettable = false;
                break;                
            }
        }
        if (!isSubsettable) cout << group << " is not subsettable " << endl;
        return isSubsettable;
    }

    /**
     * get configured product epoch
     * @param shortName
     * @return epoch if configured, empty string if no configuration found
     */
    string getProductEpoch(const string& shortName)
    {
        string value;
        for (map<string, string>::iterator it = productEpochs.begin(); it != productEpochs.end(); it++)
        {
            if (regex_match(shortName, regex(it->first)))
            {
                value = it->second;
                break;
            }
        }
        return value;
    }
    
    /**
     * matching the coordinate dataset names with configured ones
     * @param shortName product shortname
     * @param names list of possible coordinate names for checking against configuration
     * @param timeName out: the time dataset name matched
     * @param latitudeName out: the latitude dataset name matched
     * @param longitudeName out: the longitude dataset name matched
     */
    void getMatchingCoordinateDatasetNames(const string& shortName, vector<string>& names, 
         string& timeName, string& latitudeName, string& longitudeName, string& ignoreName)
    {
        bool shortNameMatched = false, patternMatched = false;
        for (map<string, map<string, vector<string>>>::iterator it = coordinateDatasetNames.begin();
             it != coordinateDatasetNames.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, vector<string>> coordinates = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                // time dataset
                if (coordinates.find("time") != coordinates.end())
                {
                    vector<string> availableNames = coordinates.find("time")->second;
                    timeName = findStringInBothVectors(availableNames, names);
                }
                // latitude
                if (coordinates.find("latitude") != coordinates.end())
                {
                    vector<string> availableNames = coordinates.find("latitude")->second;
                    latitudeName = findStringInBothVectors(availableNames, names);
                }
                // longitude
                if (coordinates.find("longitude") != coordinates.end())
                {
                    vector<string> availableNames = coordinates.find("longitude")->second;
                    longitudeName = findStringInBothVectors(availableNames, names);
                }
                if (coordinates.find("ignore") != coordinates.end())
                {
                    vector<string> availableNames = coordinates.find("ignore")->second;
                    ignoreName = findStringInBothVectors(availableNames, names);
                }
                break;
            }
        }
        cout << "Configuration.getMatchingCoordinateDatasetNames matched " << timeName << "," 
             << latitudeName << "," << longitudeName << "," << ignoreName << endl;
    }
    
    /**
     * check if a product has Photon and Segment Groups
     * @param shortName product shortname
     * @return whether the product configured to have Photon and Segment groups
     */
    bool hasPhotonSegmentGroups(const string& shortName)
    {
        bool matched = false;        
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;            
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {                
                matched = true;
                break;
            }
        }
        return matched;
    }
    
    /**
     * check if a group would be subsetted by the "super group" method
     * @param shortName product shortname
     * @return whether the group would be subsetted using the super group
     */
    bool subsetBySuperGroup(const string& shortName, const string& groupname)
    {
        bool matched = false;
        for (map<string, map<string, vector<string>>>::iterator it = superGroups.begin();
                it != superGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                for (map<string, vector<string>>::iterator it2 = it->second.begin(); 
                        it2 != it->second.end(); it2++)
                {
                    string groupPattern = it2->first;
                    if (regex_match(groupname, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
    /**
     * check if a group is a segment group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a segment group
     */
    bool isSegmentGroup(const string& shortName, const string& group)
    {
        //cout << "isSegmentGroup" << endl;
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {                
                if (keyinfo.find("SegmentGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("SegmentGroup");
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true; 
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
    /**
     * check if a group is a photon group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isPhotonGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {                
                if (keyinfo.find("PhotonGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("PhotonGroup");
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true; 
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
     /**
     * check if a group is a photon dataset
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isPhotonDataset(const string& shortName, const string& dataset)
    {
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {                
                if (keyinfo.find("PhotonDataset") != keyinfo.end())
                {
                    string datasetPattern = keyinfo.at("PhotonDataset");
                    if (regex_search(dataset, regex(datasetPattern)))
                    {
                        matched = true; 
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
    /**
     * check if a group is a freeboard swath segment group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isFreeboardSwathSegmentGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        //cout << "group: " << group << endl;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("FreeboardSwathSegmentGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("FreeboardSwathSegmentGroup")+"$";
                    //cout << "groupPattern: " << groupPattern << endl;
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }

    /**
     * check if a group is a freeboard beam segment group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isFreeboardBeamSegmentGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("FreeboardBeamSegmentGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("FreeboardBeamSegmentGroup");
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
        /**
     * check if a group is a leads group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isLeadsGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("LeadsGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("LeadsGroup");
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }

    /**
     * check if a group is a swath freeboard group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isSwathFreeboardGroup(const string& shortName, const string& group)
    {
        //cout << "isSwathFreeboardGroup" << endl;
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            //cout << "shortname: " << shortName << endl;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("SwathFreeboardGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("SwathFreeboardGroup");
                    //cout << "groupPattern: " << groupPattern << endl;
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
        /**
     * check if a group is a beam freeboard group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isBeamFreeboardGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("BeamFreeboardGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("BeamFreeboardGroup");
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
    /**
     * check if a group is a heights group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isHeightsGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("HeightsGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("HeightsGroup");
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
    /**
     * check if a group is a geophysical group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isGeophysicalGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        // check shortname and group match 
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("GeophysicalGroup") != keyinfo.end())
                {
                    string groupPattern = keyinfo.at("GeophysicalGroup");
                    if (regex_search(group, regex(groupPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        return matched;
    }
    
    /*
     * check if a group needs Leads group for subsetting
     * @param shortname product shortname
     * @param group groupname
     * @return whether the group needs leads group for subsetting
     */
    bool isHeightSegmentRateGroup(const string& shortName, const string& group)
    {
        bool matched = false;
        
        if (isSwathFreeboardGroup(shortName, group) || isBeamFreeboardGroup(shortName, group)
                 || isHeightsGroup(shortName, group) || isGeophysicalGroup(shortName, group))
        {
            matched = true;
        }
        
        return matched;
    }
    
    /**
     * get corresponding referenced group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getReferencedGroupname(const string& shortName, const string& group)
    {
        //cout << "getReferencedGroupname" << endl;
        string referencedGroupname;

        if (isPhotonGroup(shortName, group))
        {
            referencedGroupname = getSegmentGroupOfPhotonGroup(shortName, group);
        }
        else if (isPhotonDataset(shortName, group))
        {
            referencedGroupname = getSegmentGroupOfPhotonDataset(shortName, group);
        }
        else if (isSwathFreeboardGroup(shortName, group))
        {
            referencedGroupname = getSwathSegmentGroup(shortName, group);
        }
        else if (isHeightSegmentRateGroup(shortName, group) && !isSwathFreeboardGroup(shortName, group))
        {
            referencedGroupname = getBeamSegmentGroup(shortName, group);
        }
        else if (isLeadsGroup(shortName, group))
        {
            if (freeboardSwathSegment)
            {
                referencedGroupname = getSwathSegmentGroup(shortName, group);
            }
            else
            {
                referencedGroupname = getBeamSegmentGroup(shortName, group);
            }
        }
        else if (isFreeboardBeamSegmentGroup(shortName, group))
        {
            if (freeboardSwathSegment)
            {
                referencedGroupname = getSwathSegmentGroup(shortName, group);
            }
            else
            {
                referencedGroupname = getBeamSegmentGroup(shortName, group);
            }
        }
        //cout << "referencedGroupname: " << referencedGroupname << endl;
        return referencedGroupname;
    }
        
    /**
     * get target group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getTargetGroupname(const string& shortName, const string& group, const string& datasetName = "")
    {
        //cout << "getTargetGroupname" << endl;
        string targetGroupname;

        if (isSegmentGroup(shortName, group))
        {
            targetGroupname = getPhotonGroupOfSegmentGroup(shortName, group);
        }
        else if (isHeightSegmentRateGroup(shortName, group) && !isSwathFreeboardGroup(shortName, group))
        {
            targetGroupname = getBeamSegmentGroup(shortName, group);
        }
        else if (isSwathFreeboardGroup(shortName, group))
        {
            targetGroupname = getSwathSegmentGroup(shortName, group);
        }
        else if (isFreeboardSwathSegmentGroup(shortName, group))
        {
            targetGroupname = getLeadsGroup(shortName, group, datasetName);
        }
        else if (isFreeboardBeamSegmentGroup(shortName, group))
        {
            if (isBeamIndex(shortName, datasetName)) targetGroupname = getLeadsGroup(shortName, group, datasetName);
            else if (isSwathHeightIndex(shortName, datasetName))
            {
                if (freeboardSwathSegment)
                {
                    targetGroupname = getSwathSegmentGroup(shortName, group);
                }
                else
                {
                    targetGroupname = group;
                }
            }
        }
        else if (isLeadsGroup(shortName, group))
        {
            targetGroupname = getHeightSegmentRateGroup(shortName, group);
        }
        //cout << "targetGroupname: " << targetGroupname << endl;
        return targetGroupname;
    }

    
    /**
     * get corresponding photon group for a given segment group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getPhotonGroupOfSegmentGroup(const string& shortName, const string& group)
    {
        string photonGroup;
        // only if the group is photon group 
        if (isSegmentGroup(shortName, group))
        {
            for (map<string, map < string, string>>::iterator it = photonSegmentGroups.begin();
            it != photonSegmentGroups.end(); it++)
            {
                string shortNamePattern = it->first;
                map<string, string> keyinfo = it->second;
                // shortname matched
                if (regex_match(shortName, regex(shortNamePattern)))
                {
                    // get the photon and segment group names which are at the end of path pattern 
                    string pathDelim("/");
                    vector<string> paths;
                    split(paths, keyinfo.at("PhotonGroup"), is_any_of(pathDelim));
                    string photonGroupName = paths.at(paths.size() - 2);
                    paths.clear();
                    split(paths, keyinfo.at("SegmentGroup"), is_any_of(pathDelim));
                    string segmentGroupName = paths.at(paths.size() - 2);
                    // replace photonGroupName with segmentGroupName to get corresponding segment group'
                    photonGroup = group;
                    replace_last(photonGroup, segmentGroupName, photonGroupName);
                    //cout << "Configuration.getSegmentGroupOfPhotonGroup " << photonGroupName 
                    //     << " " << segmentGroupName << " " << segmentGroup << endl; 
                    break;
                }
            }
        }
        return photonGroup; 
    }
    
    /**
     * get corresponding segment group for a given photon group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getSegmentGroupOfPhotonGroup(const string& shortName, const string& group)
    {
        string segmentGroup;
        // only if the group is photon group 
        if (isPhotonGroup(shortName, group))
        {
            for (map<string, map < string, string>>::iterator it = photonSegmentGroups.begin();
            it != photonSegmentGroups.end(); it++)
            {
                string shortNamePattern = it->first;
                map<string, string> keyinfo = it->second;
                // shortname matched
                if (regex_match(shortName, regex(shortNamePattern)))
                {
                    // get the photon and segment group names which are at the end of path pattern 
                    string pathDelim("/");
                    vector<string> paths;
                    split(paths, keyinfo.at("PhotonGroup"), is_any_of(pathDelim));
                    string photonGroupName = paths.at(paths.size() - 2);
                    paths.clear();
                    split(paths, keyinfo.at("SegmentGroup"), is_any_of(pathDelim));
                    string segmentGroupName = paths.at(paths.size() - 2);
                    // replace photonGroupName with segmentGroupName to get corresponding segment group'
                    segmentGroup = group;
                    replace_last(segmentGroup, photonGroupName, segmentGroupName);
                    //cout << "Configuration.getSegmentGroupOfPhotonGroup " << photonGroupName 
                    //     << " " << segmentGroupName << " " << segmentGroup << endl; 
                    break;
                }
            }
        }
        return segmentGroup; 
    }
    
    /**
     * get corresponding segment group for a given photon dataset
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getSegmentGroupOfPhotonDataset(const string& shortName, const string& dataset)
    {
        string segmentGroup;
        
        if (isPhotonDataset(shortName, dataset))
        {
            for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
                    it!= photonSegmentGroups.end(); it++)
            {
                string shortNamePattern = it->first;
                map<string, string> keyinfo = it->second;
                //shortname matched
                if (regex_match(shortName, regex(shortNamePattern)))
                {
                    string pathDelim("/");
                    vector<string> paths;
                    split(paths, keyinfo.at("PhotonDataset"), is_any_of(pathDelim));
                    string photonGroup = paths.at(1);
                    paths.clear();
                    split(paths, dataset, is_any_of(pathDelim));
                    string photon = paths.at(1);
                    segmentGroup = keyinfo.at("SegmentGroup");
                    replace_last(segmentGroup, photonGroup, photon);
                    //cout << "Configuration.getSegmentGroupOfPhotonGroup " << photonGroup 
                    //     << " " << photon << " " << segmentGroup << endl;
                    
                }
            }
        }
        
        return segmentGroup;
    }
 
    /**
     * get corresponding swath segment group for a given leads group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getSwathSegmentGroup(const string& shortName, const string& group)
    {
        string swathSegmentGroup;
        
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("FreeboardSwathSegmentGroup") != keyinfo.end() )
                {
                    swathSegmentGroup = keyinfo.at("FreeboardSwathSegmentGroup");
                }
            }
        }
        
        //swathSegmentGroup = swathSegmentGroup.substr(0, swathSegmentGroup.length()-1);
        return swathSegmentGroup;
    }
    
    /**
     * get corresponding beam segment group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getBeamSegmentGroup(const string& shortName, const string& group)
    {
        string beamSegmentGroup;
        vector<string> paths;
        string pathDelim("/");
        string groundTrack;
        string groundTrackPattern;

        if ((isHeightSegmentRateGroup(shortName, group) && !isSwathFreeboardGroup(shortName, group))||
                isLeadsGroup(shortName, group) || isSwathHeightIndex(shortName, group))
        {
            for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
                 it != photonSegmentGroups.end(); it++)
            {
                string shortNamePattern = it->first;
                map<string, string> keyinfo = it->second;
                if (regex_match(shortName, regex(shortNamePattern)))
                {
                    beamSegmentGroup = keyinfo.at("FreeboardBeamSegmentGroup");
                    split(paths, group, is_any_of(pathDelim));
                    groundTrack = paths.at(1);
                    paths.clear();
                    split(paths, beamSegmentGroup, is_any_of(pathDelim));
                    groundTrackPattern = paths.at(1);
                    paths.clear();
                    replace_last(beamSegmentGroup, groundTrackPattern, groundTrack);
                }
            }
        }
        //cout << "beamSegmentGroup: " << beamSegmentGroup << endl;
        return beamSegmentGroup;
    }
    
    /**
     * get corresponding beam segment group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getBeamFreeboardGroup(const string& shortName, const string& group)
    {
        string beamFreeboardGroup;
        vector<string> paths;
        string pathDelim("/");
        string groundTrack;
        string groundTrackPattern;

        if (isHeightSegmentRateGroup(shortName, group) && !isSwathFreeboardGroup(shortName, group))
        {
            for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
                 it != photonSegmentGroups.end(); it++)
            {
                string shortNamePattern = it->first;
                map<string, string> keyinfo = it->second;
                if (regex_match(shortName, regex(shortNamePattern)))
                {
                    beamFreeboardGroup = keyinfo.at("BeamFreeboardGroup");
                    split(paths, group, is_any_of(pathDelim));
                    groundTrack = paths.at(1);
                    paths.clear();
                    split(paths, beamFreeboardGroup, is_any_of(pathDelim));
                    groundTrackPattern = paths.at(1);
                    paths.clear();
                    replace_last(beamFreeboardGroup, groundTrackPattern, groundTrack);
                }
            }
        }
        //cout << "beamFreeboardGroup: " << beamFreeboardGroup << endl;
        return beamFreeboardGroup;
    }
    
    /**
     * get corresponding leads group for a given beam freeboard/swath freeboard group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getLeadsGroup(const string& shortName, const string& group, const string& datasetName="")
    {
        string leadsGroup;
                
        boost::unordered_map<string, string> groundTrackMap{{"gt1","gt1l"},{"gt2","gt1r"},
                {"gt3","gt2l"},{"gt4","gt2r"},{"gt5","gt3l"},{"gt6","gt3r"}};

        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                string pathDelim("/");
                vector<string> paths;
                split(paths, group, is_any_of(pathDelim));
                string groundTrack;
                if (isSwathFreeboardGroup(shortName, group)) groundTrack = paths.at(2);
                else if (isFreeboardBeamSegmentGroup(shortName, group)) groundTrack = paths.at(1);
                else if (isFreeboardSwathSegmentGroup(shortName, group))
                {
                    paths.clear();
                    string pathDelimDash("_");
                    //cout << "datasetName: " << datasetName << endl;
                    split(paths, datasetName, is_any_of(pathDelimDash));
                    //groundTrack = groundTrackMap.at(paths.at(paths.size()-1));
                    //cout << "groundTrack: " << paths.at(paths.size()-1) << endl;
                    string groundTrackPattern = paths.at(paths.size()-1);
                    if (groundTrackPattern.length() == 3) groundTrack = groundTrackMap.at(paths.at(paths.size()-1));
                    else groundTrack = paths.at(paths.size()-1);
                }
                else groundTrack = paths.at(1);
                paths.clear();
                split(paths, keyinfo.at("LeadsGroup"), is_any_of(pathDelim));
                string leadsGroupname = paths.at(paths.size() - 2);
                leadsGroup = "/" + groundTrack + "/" + leadsGroupname + "/";
            }
        }
        return leadsGroup;
    }
    
    /**
     * get corresponding height segment rate group for a given leads group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    string getHeightSegmentRateGroup(const string& shortName, const string& groupname)
    {
        //cout << "getHeightSegmentRateGroup" << endl;
        string heightsGroupname;
        string pathDelim("/");
        vector<string> paths;
        
        if (isLeadsGroup(shortName, groupname))
        {
            for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
                 it != photonSegmentGroups.end(); it++)
            {
                string shortNamePattern = it->first;
                map<string, string> keyinfo = it->second;
                if (regex_match(shortName, regex(shortNamePattern)))
                {
                    heightsGroupname = keyinfo.at("HeightsGroup");
                    split(paths, heightsGroupname, is_any_of(pathDelim));
                    string groundTrackPattern = paths.at(1);
                    //cout << "groundTrackPattern: " << groundTrackPattern << endl;
                    paths.clear();
                    split(paths, groupname, is_any_of(pathDelim));
                    string groundTrack = paths.at(1);
                    replace_last(heightsGroupname, groundTrackPattern, groundTrack);
                    //cout << "heightGroupname: " << heightsGroupname << endl;
                    
                }
            }
        }
        
        return heightsGroupname;
    }
    
    /**
     * get configured index begin and count dataset names based on group name
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name 
     * @param count out: count datasetname
     */
    void getDatasetNames(const string& shortName, const string& groupname, string& indexBegin, string& count)
    {
        if (isPhotonGroup(shortName, groupname) || isPhotonDataset(shortName, groupname))
        {
            getSegmentDatasetNames(shortName, indexBegin, count);
            if (isPhotonDataset(shortName, groupname))
            {
                getMatchedDatasetNames(groupname, indexBegin, count);
            }
        }
        else if (isHeightSegmentRateGroup(shortName, groupname))
        {
            getLeadsDatasetNames(shortName, indexBegin, count);
        }
        else if (isLeadsGroup(shortName, groupname))
        {
            if (freeboardSwathSegment)
            {
                getSwathSegmentDatasetNames(shortName, groupname, indexBegin, count, "");
            }
            else
            {
                getBeamSegmentDatasetNames(shortName, indexBegin, count);
            }
        }
    }
    
    /**
     * get corresponding index begin and count dataset names from given photon dataset
     * @datasetname photon dataset name
     * @indexBegin index begin dataset name
     * @count count dataset name
     */
    void getMatchedDatasetNames(const string& datasetname, string& indexBegin, string& count)
    {
        //return if indexBegin/count dataset names do not require pattern matching
        if (indexBegin.find("[") == std::string::npos) return;
        string delim("/x");
        vector<string> paths;
        split(paths, datasetname, is_any_of(delim));
        string sub;
        if (datasetname.find("/") == std::string::npos) sub = paths.at(0);
        else sub = paths.at(paths.size()-2);
        paths.clear();
        split(paths, indexBegin, is_any_of(delim));
        string pattern = paths.at(0);
        paths.clear();
        replace_last(indexBegin, pattern, sub);
        replace_last(count, pattern, sub);
    }
    
    /**
     * get configured segment photonIndexBegin dataset name
     * @param shortName product shortname
     * @return photonIndexBegin dataset name
     */
    string getIndexBeginDatasetName(const string& shortName, const string& groupname, const string& datasetName="", bool repair = false)
    {
        //cout << "getIndexBeginDatasetName" << endl;
        string indexBegin, count;
        if (isSegmentGroup(shortName, groupname))
        {
            getSegmentDatasetNames(shortName, indexBegin, count);
            getMatchedDatasetNames(datasetName, indexBegin, count);
        }
        else if (isLeadsGroup(shortName, groupname))
        {
            getLeadsDatasetNames(shortName, indexBegin, count);
        }
        else if (isBeamFreeboardGroup(shortName, groupname))
        {
            getBeamFreeboardIndexBegin(shortName, indexBegin);
        }
        else if (isFreeboardBeamSegmentGroup(shortName, groupname))
        {
            if (!repair || isSwathHeightIndex(shortName, datasetName)) getBeamSegmentDatasetNames(shortName, indexBegin);
            else getBeamSegmentDatasetNames(shortName, indexBegin, count);
            
        }
        else if (isSwathFreeboardGroup(shortName, groupname))
        {
            getSwathFreeboardIndexName(shortName, indexBegin);
        }
        else if(isFreeboardSwathSegmentGroup(shortName, groupname))
        {	
            getSwathSegmentDatasetNames(shortName, groupname, indexBegin, count, datasetName);
        }
        //cout << "indexBegin: " << indexBegin << endl;
        return indexBegin;
    }
    
    /**
     * get configured segment segmentPhotonCount dataset name
     * @param shortName product shortname
     * @return segmentPhotonCount dataset name
     */
    string getCountDatasetName(const string& shortName, const string& groupname, const string& datasetName="")
    {
        string indexBegin, count;
        if (isSegmentGroup(shortName, groupname))
        {
            getSegmentDatasetNames(shortName, indexBegin, count);
            getMatchedDatasetNames(datasetName, indexBegin, count);
        }
        else if (isLeadsGroup(shortName, groupname))
        {
            getLeadsDatasetNames(shortName, indexBegin, count);
        }
        else if(isFreeboardSwathSegmentGroup(shortName, groupname))
        {
            getSwathSegmentDatasetNames(shortName, groupname, indexBegin, count,datasetName);
        }
        else if (isFreeboardBeamSegmentGroup(shortName, groupname))
        {
            getBeamSegmentDatasetNames(shortName, indexBegin, count);
        }

	//cout << "count: " << count << endl;
        return count;
    }
    
    /**
     * get configured segment and photon dataset names
     * @param shortName product shortname
     * @param photonIndexBegin out: photonIndexBegin dataset name 
     * @param segmentPhotonCount out: segmentPhotonCount datasetname
     */
    void getSegmentDatasetNames(const string& shortName, string& photonIndexBegin, string& segmentPhotonCount)
    {
        //// first check shortname matches,
        // then get the datasetnames
        for (map<string, map<string,string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string,string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, regex(shortNamePattern)))
            {          
                if (keyinfo.find("PhotonIndexBegin") != keyinfo.end() && keyinfo.find("SegmentPhotonCount") != keyinfo.end())
                {
                    photonIndexBegin = keyinfo.at("PhotonIndexBegin");
                    segmentPhotonCount = keyinfo.at("SegmentPhotonCount");
                    break;
                }
            }
        }
    }
    
    /**
     * get configured index begin and count dataset names from leads group(ATL10)
     * @param shortName product shortname
     * @param indexBegin out: index begin dataset name 
     * @param count out: count datasetname
     */
    void getLeadsDatasetNames(const string& shortName, string& indexBegin, string& count)
    {
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("LeadsIndex") != keyinfo.end() && keyinfo.find("LeadsCount") != keyinfo.end())
                {
                    indexBegin = keyinfo.at("LeadsIndex");
                    count = keyinfo.at("LeadsCount");
                    break;
                }
            }
        }
    }
    
    /**
     * get configured index begin and count dataset names for swath segment group(ATL10)
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name 
     * @param count out: count datasetname
     */
    void getSwathSegmentDatasetNames(const string& shortName, const string& groupname, string& indexBegin, string& count, const string& datasetName)
    {
	//cout << "datasetName: " << datasetName << endl;
        boost::unordered_map<string, string> groundTrack{{"gt1l","gt1"},{"gt1r","gt2"},
                {"gt2l","gt3"},{"gt2r","gt4"},{"gt3l","gt5"},{"gt3r","gt6"}};
        string pathDelim("_/");
        vector<string> paths;
        //split(paths, groupname, is_any_of(pathDelim));
        //string groundTrackNum = groundTrack.at(paths.at(1));
        //paths.clear();
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("SwathIndex") != keyinfo.end() && keyinfo.find("SwathCount") != keyinfo.end())
                {
		    indexBegin = keyinfo.at("SwathIndex");
                    count = keyinfo.at("SwathCount");
		    string groundTrackNum;
		    split(paths, indexBegin, is_any_of(pathDelim));
		    string groundTrackPattern = paths.at(paths.size()-1);
		    paths.clear();
                    if (!datasetName.empty())
		    {
			split(paths, datasetName, is_any_of(pathDelim));
                        groundTrackNum = paths.at(paths.size()-1);
    			paths.clear();
		    }
		    else
		    {
                        split(paths, groupname, is_any_of(pathDelim));
                        groundTrackNum = paths.at(1);
                        //groundTrackNum = groundTrack.at(paths.at(1));
                        //cout << "groundTrackNum: " << groundTrackNum << endl;
			paths.clear();
		    }
		    //cout << "groundTrackNum: " << groundTrackNum << endl;
                    replace_last(indexBegin, groundTrackPattern, groundTrackNum);
                    replace_last(count, groundTrackPattern, groundTrackNum);
                    //cout << "indexBegin: " << indexBegin << endl;
                    //cout << "count: " << count << endl;
                    break;
                }
            }
        }
    }
    
    void groundTrackRename(string groupname, string& indexBegin, string& count)
    {
        boost::unordered_map<string, string> groundTrack{{"gt1l","gt1"},{"gt1r","gt2"},
                {"gt2l","gt3"},{"gt2r","gt4"},{"gt3l","gt5"},{"gt3r","gt6"}};
        string pathDelim("_/");
        vector<string> paths;
        
        split(paths, groupname, is_any_of(pathDelim));
        string groundTrackNum = groundTrack.at(paths.at(1));
        replace_last(indexBegin, paths.at(1), groundTrackNum);
        replace_last(count, paths.at(1), groundTrackNum);
        //cout << "indexBegin: " << indexBegin << endl;
        //cout << "count: " << count << endl;
    }
    
    /**
     * get configured index begin and count dataset names for swath segment group(ATL10)
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name 
     */
    void getBeamSegmentDatasetNames(const string& shortName, string& indexBegin, string& count)
    {
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("BeamIndex") != keyinfo.end() && keyinfo.find("BeamCount") != keyinfo.end())
                {
                    indexBegin = keyinfo.at("BeamIndex");
                    count = keyinfo.at("BeamCount");
                    break;
                }
            }
        }
    }
    
    /**
     * get configured index begin dataset name for swath segment group(ATL10)
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name 
     */
    void getBeamFreeboardIndexBegin(const string& shortName, string& indexBegin)
    {
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("BeamFreeboardIndex") != keyinfo.end())
                {
                    indexBegin = keyinfo.at("BeamFreeboardIndex");
                    break;
                }
            }
        }
    }
    
    /**
     * get configured index begin dataset name for swath segment group(ATL10)
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name 
     */
    void getBeamSegmentDatasetNames(const string& shortName, string& indexBegin)
    {
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("SwathHeightIndex") != keyinfo.end())
                {
                    indexBegin = keyinfo.at("SwathHeightIndex");
                    break;
                }
            }
        }
    }
    
    /**
     * get configured index begin dataset name for swath segment group(ATL10)
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name 
     */
    void getSwathFreeboardIndexName(const string& shortName, string& indexBegin)
    {
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("SwathHeightIndex") != keyinfo.end())
                {
                    indexBegin = keyinfo.at("SwathHeightIndex");
                    break;
                }
            }
        }
    }
    
    /**
     * check if a dataset if swath  index begin dataset
     * @param shortName product shortname
     * @param datasetName dataset name
     * @return whether the group is a swath index begin dataset
     */
    bool isSwathHeightIndex(const string& shortName, const string datasetName)
    {
        bool matched = false;
        
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("SwathHeightIndex") != keyinfo.end())
                {
                    string datasetPattern = keyinfo.at("SwathHeightIndex");
                    if (regex_search(datasetName, regex(datasetPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        
        return matched;
    }
    
    /**
     * check if a dataset if beam index begin dataset
     * @param shortName product shortname
     * @param datasetName dataset name
     * @return whether the group is a beam index dataset
     */
    bool isBeamIndex(const string& shortName, const string datasetName)
    {
        bool matched = false;
        
        for (map<string, map<string, string>>::iterator it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end(); it++)
        {
            string shortNamePattern = it->first;
            map<string, string> keyinfo = it->second;
            if (regex_match(shortName, regex(shortNamePattern)))
            {
                if (keyinfo.find("BeamIndex") != keyinfo.end())
                {
                    string datasetPattern = keyinfo.at("BeamIndex");
                    if (regex_search(datasetName, regex(datasetPattern)))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }
        
        return matched;
    }
    
private:

    Configuration() {};
    Configuration(Configuration const&) {};
    Configuration& operator=(Configuration const&){};
    ~Configuration(){};
  
    // return a value in vectorB which is also in vectorA,
    // an empty string is returned if no matches
    string findStringInBothVectors(vector<string> vectorA, vector<string> vectorB)
    {
        string returnValue;
        for (vector<string>::iterator ait = vectorA.begin(); ait != vectorA.end(); ait++)
        {
            string value = *ait;
            vector<string>::iterator it = find(vectorB.begin(), vectorB.end(), value);
            if (it != vectorB.end())
            {
                returnValue = *it;
            }
        }
        return returnValue;
    }


    // check if a given string matches any regexp on the list, only search if the string
    // has the pattern since the string can have extra information
    // e.g. value: /profile_1/calibration matches the regexpList which has /profile_1
    bool matchRegexpList(vector<string> regexpList, string value)
    {
        bool matchFound = false;  
        for (vector<string>::iterator it = regexpList.begin(); it != regexpList.end(); it++)
        {
            if (regex_search(value, regex(*it)))
            {
                matchFound = true; 
                break;
            }
        }
        return matchFound;
    }

    static Configuration* myInstance;

    /**
     * coordinate dataset names for each shortname pattern
     * key: shortname pattern, 
     * value: list of possible names for each type (time, latitude, longitude)
     *        (key: dataset type, e.g. time, latitude, longitude, 
     *         value: list of possible dataset names)
     * e.g.
     *   "ATL[\\d]{2}_SDP": 
     *   {
     *       "time": ["delta_time"],
     *       "latitude": ["latitude", "reference_photon_lat", "lat_ph"] ,
     *       "longitude": ["longitude", "reference_photon_lon", "lon_ph"]
     *   }
     */
    map<string, map<string, vector<string>>> coordinateDatasetNames;

    /**
     * key: shortname pattern, value: epoch time
     * e.g.
     * "ATL[\\d]{2}_SDP": "2005-01-01T00.00.00.000000"
     */
    map<string, string> productEpochs;

    /**
     * subsettable groups 
     * key: shortname pattern
     * value: list of group patterns
     */
    map<string, vector<string>> subsettableGroups;
    
    /**
     * unsubsettable groups 
     * key: shortname pattern
     * value: list of group patterns
     * e.g.
     * "ATL[\\d]{2}_SDP": [
     *       "/ancillary_data",
     *       "/atlas_impulse_response",
     *       "/orbit_info",
     *       "/profile_[\\d]+/calibration",
     *       "/atlas",
     *       "/quality_assessment"
     *   ]
     */
    map<string, vector<string>> unsubsettableGroups;

    /**
     * PhotonSegmentGroups
     * key: shortName pattern
     * value: map of feature key and value/pattern
     * e.g.
     * "PhotonSegmentGroups":
     * {
     *     "ATL03_SDP":
     *     {
     *         "CoordinateClass": "PhotonCoordinates",
     *         "SegmentGroup": "/gt[\\w]+/geolocation/",
     *         "PhotonGroup":  "/gt[\\w]+/heights/",
     *         "PhotonIndexBegin": "ph_index_beg",
     *         "SegmentPhotonCount": "segment_ph_cnt"
     *     }
     * }
     */
    map<string, map<string, string>> photonSegmentGroups;
    
    /**
     * shortname attribute path
     * key: mission name
     * value: shortname attribute path
     */
    vector<string> shortnames;
    
    map<string, map<string, vector<string>>> superCoordinateDatasets;
    
    map<string, map<string, vector<string>>> superGroups;
    
    map<string, string> missions;
    
    map<string, map<string, vector<string>>> requiredDatasets;
    
    map<string, map<string, string>> resolutions;
    
    map<string, string> projections;
    
    bool freeboardSwathSegment;
};
Configuration* Configuration::myInstance = NULL;
#endif
