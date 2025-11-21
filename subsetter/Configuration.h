#ifndef Configuration_H
#define Configuration_H

#include "LogLevel.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/unordered_map.hpp>
#include <exception>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace property_tree = boost::property_tree;

// class to handle configuration information
class Configuration
{
  public:
    /**
     * initialize the information from json configuration file
     * @param configFile the configuration file
     */
    Configuration(const std::string &configFile)
    {
        property_tree::ptree root;
        std::string s;
        std::stringstream ss;
        try
        {
            s = removeComment(configFile);
            ss << s;
            property_tree::read_json(ss, root);

            // coordinate dataset names for each shortname pattern
            if (root.get_child_optional("CoordinateDatasetNames"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("CoordinateDatasetNames"))
                {
                    std::string shortNamePattern = nodei.first;
                    std::map<std::string, std::vector<std::string>> datasets;
                    // list of possible names for each type (time, latitude,
                    // longitude)

                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string datasetType = nodej.first;
                        std::vector<std::string> names;

                        BOOST_FOREACH (property_tree::ptree::value_type &nodek,
                                       nodej.second)
                        {
                            names.push_back(
                                nodek.second.get_value<std::string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    coordinateDatasetNames.insert(
                        make_pair(shortNamePattern, datasets));
                }
            }

            // get product epoch
            if (root.get_child_optional("ProductEpochs"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &v,
                               root.get_child("ProductEpochs"))
                {
                    productEpochs.insert(make_pair(v.first, v.second.data()));
                }
            }

            // get subsettable group patterns
            if (root.get_child_optional("SubsettableGroups"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("SubsettableGroups"))
                {
                    std::string shortNamePattern = nodei.first;
                    std::vector<std::string> groupPatterns;

                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string value =
                            nodej.second.get_value<std::string>();
                        groupPatterns.push_back(value);
                    }
                    subsettableGroups.insert(
                        make_pair(shortNamePattern, groupPatterns));
                }
            }

            // get unsubsettable group patterns
            if (root.get_child_optional("UnsubsettableGroups"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("UnsubsettableGroups"))
                {
                    std::string shortNamePattern = nodei.first;
                    std::vector<std::string> groupPatterns;

                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string value =
                            nodej.second.get_value<std::string>();
                        groupPatterns.push_back(value);
                    }
                    unsubsettableGroups.insert(
                        make_pair(shortNamePattern, groupPatterns));
                }
            }

            // get PhotonSegment group patterns
            if (root.get_child_optional("PhotonSegmentGroups"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("PhotonSegmentGroups"))
                {
                    std::string shortNamePattern = nodei.first;
                    std::map<std::string, std::string> keyinfo;
                    // list of keys and their value
                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string key = nodej.first;
                        std::string value =
                            nodej.second.get_value<std::string>();
                        keyinfo.insert(make_pair(key, value));
                    }
                    photonSegmentGroups.insert(
                        make_pair(shortNamePattern, keyinfo));
                }
            }

            // get shortname attribute path
            if (root.get_child_optional("ShortNamePath"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &v,
                               root.get_child("ShortNamePath"))
                {
                    shortnames.push_back(v.second.data());
                }
            }

            // get mission from shortname
            if (root.get_child_optional("Missions"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &v,
                               root.get_child("Missions"))
                {
                    missions.insert(make_pair(v.first, v.second.data()));
                }
            }

            if (root.get_child_optional("SuperGroupCoordinates"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("SuperGroupCoordinates"))
                {
                    std::string shortNamePattern = nodei.first;
                    std::map<std::string, std::vector<std::string>> datasets;
                    // list of possible names for each type (time, latitude,
                    // longitude)

                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string datasetType = nodej.first;
                        std::vector<std::string> names;

                        BOOST_FOREACH (property_tree::ptree::value_type &nodek,
                                       nodej.second)
                        {
                            names.push_back(
                                nodek.second.get_value<std::string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    superCoordinateDatasets.insert(
                        make_pair(shortNamePattern, datasets));
                }
            }

            if (root.get_child_optional("SuperGroups"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("SuperGroups"))
                {
                    std::string shortNamePattern = nodei.first;
                    std::map<std::string, std::vector<std::string>> datasets;

                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string datasetType = nodej.first;
                        std::vector<std::string> names;

                        BOOST_FOREACH (property_tree::ptree::value_type &nodek,
                                       nodej.second)
                        {
                            names.push_back(
                                nodek.second.get_value<std::string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    superGroups.insert(make_pair(shortNamePattern, datasets));
                }
            }

            if (root.get_child_optional("RequiredDatasetsByFormat"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("RequiredDatasetsByFormat"))
                {
                    std::string format = nodei.first;
                    std::map<std::string, std::vector<std::string>> datasets;
                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string datasetType = nodej.first;
                        std::vector<std::string> names;

                        // list of possible lat/lon/row/column datasets
                        BOOST_FOREACH (property_tree::ptree::value_type &nodek,
                                       nodej.second)
                        {
                            names.push_back(
                                nodek.second.get_value<std::string>());
                        }
                        datasets.insert(make_pair(datasetType, names));
                    }
                    requiredDatasets.insert(make_pair(format, datasets));
                }
            }

            if (root.get_child_optional("Resolution"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                               root.get_child("Resolution"))
                {
                    std::string groupnamePattern = nodei.first;
                    std::map<std::string, std::string> keyinfo;
                    // list of keys and their value
                    BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                                   nodei.second)
                    {
                        std::string key = nodej.first;
                        std::string value =
                            nodej.second.get_value<std::string>();
                        keyinfo.insert(make_pair(key, value));
                    }
                    resolutions.insert(make_pair(groupnamePattern, keyinfo));
                }
            }

            if (root.get_child_optional("Projections"))
            {
                BOOST_FOREACH (property_tree::ptree::value_type &v,
                               root.get_child("Projections"))
                {
                    projections.insert(make_pair(v.first, v.second.data()));
                }
            }
        }
        catch (std::exception &ex)
        {
            LOG_ERROR("ERROR: failed to parse the configuration file "
                      << configFile);
            std::exception_ptr p = std::current_exception();
            rethrow_exception(p);
        }
    }

    /**
     * removes comments in SubsetterConfig.json
     * Boost does not support comments in JSON files after 1.59
     * @param configFile the configuration file
     * @return json string without comment
     */
    std::string removeComment(const std::string &configFile)
    {
        std::ifstream is(configFile);
        std::string s, str;
        s.reserve(is.rdbuf()->in_avail());
        char c;

        while (is.get(c))
        {
            if (s.capacity() == s.size())
                s.reserve(s.capacity() * 3);
            s.append(1, c);
        }

        // regular expression to match comments
        std::string commentPattern = "(//[^\\n]*|/\\*.*?\\*/)";
        str = regex_replace(s, boost::regex(commentPattern), "");

        return str;
    }

    /**
     *
     */
    void setFreeboardSwathSegment(bool exist) { freeboardSwathSegment = exist; }

    /**
     * @return boolean value whether the freeboard swath segment group exists in
     * the input file
     */
    bool freeboardSwathSegmentExist() { return freeboardSwathSegment; }

    /**
     * get shortname attribute path
     * @return shortname attribute paths
     */
    std::vector<std::string> getShortnamePath() { return shortnames; }

    /**
     * get mission name from product shortname
     * @param shortname product shortname
     * @return mission mission name
     */
    std::string getMissionFromShortName(const std::string &shortname)
    {
        std::string mission;

        for (std::map<std::string, std::string>::iterator it = missions.begin();
             it != missions.end();
             it++)
        {
            if (regex_match(shortname, boost::regex(it->first)))
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
    std::map<std::string, std::vector<std::string>> getSuperCoordinates(
        const std::string &shortname)
    {
        std::map<std::string, std::vector<std::string>> datasets;
        for (std::map<std::string,
                      std::map<std::string, std::vector<std::string>>>::iterator
                 it = superCoordinateDatasets.begin();
             it != superCoordinateDatasets.end();
             it++)
        {
            if (regex_match(shortname, boost::regex(it->first)))
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
    void getSuperCoordinateGroupname(const std::string &shortname,
                                     std::string groupname,
                                     std::vector<std::string> &superGroupnames)
    {
        for (std::map<std::string,
                      std::map<std::string, std::vector<std::string>>>::iterator
                 it = superGroups.begin();
             it != superGroups.end();
             it++)
        {
            if (regex_match(shortname, boost::regex(it->first)))
            {
                std::string pathDelim("/");
                std::vector<std::string> paths;
                split(paths, groupname, boost::is_any_of(pathDelim));
                std::string groupNum = paths.at(1);
                paths.clear();
                for (std::map<std::string, std::vector<std::string>>::iterator
                         it2 = it->second.begin();
                     it2 != it->second.end();
                     it2++)
                {
                    for (std::vector<std::string>::iterator it3 =
                             it2->second.begin();
                         it3 != it2->second.end();
                         it3++)
                    {
                        split(paths, *it3, boost::is_any_of(pathDelim));
                        std::string groupPattern = paths.at(1);
                        paths.clear();
                        std::string superGroupname = *it3;
                        boost::replace_last(
                            superGroupname, groupPattern, groupNum);
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
    void getRequiredDatasetsAndResolution(const std::string &format,
                                          const std::string &groupname,
                                          const std::string &shortName,
                                          std::string &rowName,
                                          std::string &colName,
                                          std::string &latName,
                                          std::string &lonName,
                                          short &resolution)
    {
        std::vector<std::string> datasets = getRequiredDatasetsByFormat(
            format, groupname, shortName, resolution);
        std::string datasetName;

        for (std::vector<std::string>::iterator it = datasets.begin();
             it != datasets.end();
             it++)
        {
            datasetName = *it;
            if (datasetName.find("row") != std::string::npos)
                rowName = datasetName;
            if (datasetName.find("col") != std::string::npos)
                colName = datasetName;
            if (datasetName.find("lat") != std::string::npos)
                latName = datasetName;
            if (datasetName.find("lon") != std::string::npos)
                lonName = datasetName;
        }
    }

    /**
     * get lat/lon/row/column datasets
     * @param format output format
     * @param group name
     * @return lat/lon/row/column datasets names
     */
    std::vector<std::string> getRequiredDatasetsByFormat(
        const std::string &format,
        const std::string &groupname,
        const std::string &shortname,
        short &resolution)
    {
        std::vector<std::string> datasets;
        for (std::map<std::string,
                      std::map<std::string, std::vector<std::string>>>::iterator
                 it = requiredDatasets.begin();
             it != requiredDatasets.end();
             it++)
        {
            if (regex_match(format, boost::regex(it->first)))
            {
                for (std::map<std::string, std::vector<std::string>>::iterator
                         map_it = it->second.begin();
                     map_it != it->second.end();
                     map_it++)
                {
                    if (regex_match(groupname, boost::regex(map_it->first)))
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
    void getResolution(const std::string &groupname,
                       const std::string shortname,
                       short &resolution)
    {
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = resolutions.begin();
             it != resolutions.end();
             it++)
        {
            if (regex_match(groupname, boost::regex(it->first)))
            {
                for (std::map<std::string, std::string>::iterator it2 =
                         it->second.begin();
                     it2 != it->second.end();
                     it2++)
                {
                    if (regex_match(shortname, boost::regex(it2->first)))
                    {
                        resolution = boost::lexical_cast<short>(it2->second);
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
    std::string getProjection(const std::string &groupname)
    {
        std::string projection;

        for (std::map<std::string, std::string>::iterator it =
                 projections.begin();
             it != projections.end();
             it++)
        {
            if (regex_match(groupname, boost::regex(it->first)))
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
    bool isGroupSubsettable(const std::string &shortName,
                            const std::string &group)
    {
        LOG_DEBUG("Configuration::isGroupSubsettable(): Enter");

        bool isSubsettable = true;
        //// first check subsettable groups
        // check if it's configured to be subsettable
        bool shortNameMatched = false, patternMatched = false;
        for (std::map<std::string, std::vector<std::string>>::iterator it =
                 subsettableGroups.begin();
             it != subsettableGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::vector<std::string> groupPatterns = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                shortNameMatched = true;
                patternMatched = matchRegexpList(groupPatterns, group);
                break;
            }
        }
        // if the shortName is configured and group is not specified, the group
        // is not subsettable
        if (shortNameMatched && !patternMatched)
        {
            LOG_DEBUG("Configuration::isGroupSubsettable() group: "
                      << group << " is NOT subsettable ");
            return false;
        }

        //// second check unsubsettable group
        for (std::map<std::string, std::vector<std::string>>::iterator it =
                 unsubsettableGroups.begin();
             it != unsubsettableGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::vector<std::string> groupPatterns = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (matchRegexpList(groupPatterns, group))
                    isSubsettable = false;
                break;
            }
        }

        LOG_DEBUG(
            "Configuration::isGroupSubsettable() group: "
            << group
            << (isSubsettable ? " is subsettable" : " is NOT subsettable"));

        return isSubsettable;
    }

    /**
     * get configured product epoch
     * @param shortName
     * @return epoch if configured, empty string if no configuration found
     */
    std::string getProductEpoch(const std::string &shortName)
    {
        std::string value;
        for (std::map<std::string, std::string>::iterator it =
                 productEpochs.begin();
             it != productEpochs.end();
             it++)
        {
            if (regex_match(shortName, boost::regex(it->first)))
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
     * @param names list of possible coordinate names for checking against
     * configuration
     * @param timeName out: the time dataset name matched
     * @param latitudeName out: the latitude dataset name matched
     * @param longitudeName out: the longitude dataset name matched
     */
    void getMatchingCoordinateDatasetNames(const std::string &shortName,
                                           std::vector<std::string> &names,
                                           std::string &timeName,
                                           std::string &latitudeName,
                                           std::string &longitudeName,
                                           std::string &ignoreName)
    {
        bool shortNameMatched = false, patternMatched = false;
        for (std::map<std::string,
                      std::map<std::string, std::vector<std::string>>>::iterator
                 it = coordinateDatasetNames.begin();
             it != coordinateDatasetNames.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::vector<std::string>> coordinates =
                it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                // time dataset
                if (coordinates.find("time") != coordinates.end())
                {
                    std::vector<std::string> availableNames =
                        coordinates.find("time")->second;
                    timeName = findStringInBothVectors(availableNames, names);
                }
                // latitude
                if (coordinates.find("latitude") != coordinates.end())
                {
                    std::vector<std::string> availableNames =
                        coordinates.find("latitude")->second;
                    latitudeName =
                        findStringInBothVectors(availableNames, names);
                }
                // longitude
                if (coordinates.find("longitude") != coordinates.end())
                {
                    std::vector<std::string> availableNames =
                        coordinates.find("longitude")->second;
                    longitudeName =
                        findStringInBothVectors(availableNames, names);
                }
                if (coordinates.find("ignore") != coordinates.end())
                {
                    std::vector<std::string> availableNames =
                        coordinates.find("ignore")->second;
                    ignoreName = findStringInBothVectors(availableNames, names);
                }
                break;
            }
        }
        LOG_DEBUG("Configuration::getMatchingCoordinateDatasetNames(): matched "
                  << timeName << "," << latitudeName << "," << longitudeName
                  << "," << ignoreName);
    }

    /**
     * @brief Get the time coordinate dataset name.
     *
     * @param datasetName The dataset whose coordinates are being accessed.
     * @param shortName The collection shortname.
     * @return The name of the time dataset.
     */
    std::string getTimeCoordinateName(std::string &datasetName,
                                      std::string &shortName)
    {
        std::vector<std::string> datasetNames(1, datasetName);
        std::string timeName, latitudeName, longitudeName, ignoreName;
        this->getMatchingCoordinateDatasetNames(shortName,
                                                datasetNames,
                                                timeName,
                                                latitudeName,
                                                longitudeName,
                                                ignoreName);

        return timeName;
    }

    /**
     * @brief Get the latitude coordinate dataset name.
     *
     * @param datasetName The dataset whose coordinates are being accessed.
     * @param shortName The collection shortname.
     * @return The name of the latitude dataset.
     */
    std::string getLatitudeCoordinateName(std::string &datasetName,
                                          std::string &shortName)
    {
        std::vector<std::string> datasetNames(1, datasetName);
        std::string timeName, latitudeName, longitudeName, ignoreName;
        this->getMatchingCoordinateDatasetNames(shortName,
                                                datasetNames,
                                                timeName,
                                                latitudeName,
                                                longitudeName,
                                                ignoreName);

        return latitudeName;
    }

    /**
     * @brief Get the longitude coordinate dataset name.
     *
     * @param datasetName The dataset whose coordinates are being accessed.
     * @param shortName The collection shortname.
     * @return The name of the longitude dataset.
     */
    std::string getLongitudeCoordinateName(std::string &datasetName,
                                           std::string &shortName)
    {
        std::vector<std::string> datasetNames(1, datasetName);
        std::string timeName, latitudeName, longitudeName, ignoreName;
        this->getMatchingCoordinateDatasetNames(shortName,
                                                datasetNames,
                                                timeName,
                                                latitudeName,
                                                longitudeName,
                                                ignoreName);

        return longitudeName;
    }

    /**
     * check if a product has Photon and Segment Groups
     * @param shortName product shortname
     * @return whether the product configured to have Photon and Segment groups
     */
    bool hasPhotonSegmentGroups(const std::string &shortName)
    {
        bool matched = false;
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            // shortname matched
            if (regex_match(shortName, boost::regex(shortNamePattern)))
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
    bool subsetBySuperGroup(const std::string &shortName,
                            const std::string &groupname)
    {
        bool matched = false;
        for (std::map<std::string,
                      std::map<std::string, std::vector<std::string>>>::iterator
                 it = superGroups.begin();
             it != superGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                for (std::map<std::string, std::vector<std::string>>::iterator
                         it2 = it->second.begin();
                     it2 != it->second.end();
                     it2++)
                {
                    std::string groupPattern = it2->first;
                    if (regex_match(groupname, boost::regex(groupPattern)))
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
     * check if a group is a segment, photon, or dataset
     * @param shortName product shortname
     * @param group group name
     * @param patternName pattern name
     * @return true or false
     */
    bool isGroupPatternMatched(const std::string &shortName,
                               const std::string &group,
                               const std::string patternName)
    {
        bool matched = false;
        // check shortname and group match
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find(patternName.c_str()) != keyinfo.end())
                {
                    std::string groupPattern = keyinfo.at(patternName.c_str());
                    if (regex_search(group, boost::regex(groupPattern)))
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
    bool isSegmentGroup(const std::string &shortName, const std::string &group)
    {
        const std::string patternName("SegmentGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a photon group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isPhotonGroup(const std::string &shortName, const std::string &group)
    {
        const std::string patternName("PhotonGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a photon dataset
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isPhotonDataset(const std::string &shortName,
                         const std::string &dataset)
    {
        const std::string patternName("PhotonDataset");
        return isGroupPatternMatched(shortName, dataset, patternName);
    }

    /**
     * check if a group is a freeboard swath segment group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isFreeboardSwathSegmentGroup(const std::string &shortName,
                                      const std::string &group)
    {
        bool matched = false;
        // check shortname and group match
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find("FreeboardSwathSegmentGroup") != keyinfo.end())
                {
                    std::string groupPattern =
                        keyinfo.at("FreeboardSwathSegmentGroup") + "$";
                    if (regex_search(group, boost::regex(groupPattern)))
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
    bool isFreeboardBeamSegmentGroup(const std::string &shortName,
                                     const std::string &group)
    {
        const std::string patternName("FreeboardBeamSegmentGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a freeboard segment group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isFreeboardSegmentGroup(const std::string &shortName,
                                 const std::string &group)
    {
        const std::string patternName("FreeboardSegmentGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a leads group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isLeadsGroup(const std::string &shortName, const std::string &group)
    {
        const std::string patternName("LeadsGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a Reference Surface Section group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isReferenceSurfaceSectionGroup(const std::string &shortName,
                                        const std::string &group)
    {
        const std::string patternName("ReferenceSurfaceSectionGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a swath freeboard group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isSwathFreeboardGroup(const std::string &shortName,
                               const std::string &group)
    {
        const std::string patternName("SwathFreeboardGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a beam freeboard group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isBeamFreeboardGroup(const std::string &shortName,
                              const std::string &group)
    {
        const std::string patternName("BeamFreeboardGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a heights group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isHeightsGroup(const std::string &shortName, const std::string &group)
    {
        const std::string patternName("HeightsGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a geophysical group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isGeophysicalGroup(const std::string &shortName,
                            const std::string &group)
    {
        const std::string patternName("GeophysicalGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a Freeboard Segment Heights Group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isFreeboardSegmentHeightsGroup(const std::string &shortName,
                                        const std::string &group)
    {
        const std::string patternName("FreeboardSegmentHeightsGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a group is a geophysical group
     * @param shortName product shortname
     * @param group group name
     * @return whether the group is a photon group
     */
    bool isFreeboardSegmentGeophysicalGroup(const std::string &shortName,
                                            const std::string &group)
    {
        const std::string patternName("FreeboardSegmentGeophysicalGroup");
        return isGroupPatternMatched(shortName, group, patternName);
    }

    /**
     * check if a dataset if beam index begin dataset
     * @param shortName product shortname
     * @param datasetName dataset name
     * @return whether the group is a beam index dataset
     */
    bool isBeamIndex(const std::string &shortName,
                     const std::string datasetName)
    {
        const std::string patternName("BeamIndex");
        return isGroupPatternMatched(shortName, datasetName, patternName);
    }

    /*
     * check if a group needs Leads group for subsetting
     * @param shortname product shortname
     * @param group groupname
     * @return whether the group needs leads group for subsetting
     */
    bool isHeightSegmentRateGroup(const std::string &shortName,
                                  const std::string &group)
    {
        bool matched = false;

        if (isSwathFreeboardGroup(shortName, group) ||
            isBeamFreeboardGroup(shortName, group) ||
            isHeightsGroup(shortName, group) ||
            isGeophysicalGroup(shortName, group))
        {
            matched = true;
        }

        return matched;
    }

    /*
     * check if a group needs Leads group for subsetting
     * @param shortname product shortname
     * @param group groupname
     * @return whether the group needs leads group for subsetting
     */
    bool isFreeboardRateGroup(const std::string &shortName,
                              const std::string &group)
    {
        bool matched = false;

        if (isFreeboardSegmentGroup(shortName, group) ||
            isFreeboardSegmentHeightsGroup(shortName, group) ||
            isFreeboardSegmentGeophysicalGroup(shortName, group))
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
    std::string getReferencedGroupname(const std::string &shortName,
                                       const std::string &group)
    {
        LOG_DEBUG(
            "Configuration::getReferencedGroupname() ENTER group:" << group);

        std::string referencedGroupname;

        if (isPhotonGroup(shortName, group))
        {
            referencedGroupname =
                getSegmentGroupOfPhotonGroup(shortName, group);
        }
        else if (isPhotonDataset(shortName, group))
        {
            referencedGroupname =
                getSegmentGroupOfPhotonDataset(shortName, group);
        }
        else if (isSwathFreeboardGroup(shortName, group))
        {
            referencedGroupname = getSwathSegmentGroup(shortName, group);
        }
        else if (isHeightSegmentRateGroup(shortName, group) &&
                 !isSwathFreeboardGroup(shortName, group))
        {
            referencedGroupname =
                getFreeboardBeamSegmentGroup(shortName, group);
        }
        else if (isLeadsGroup(shortName, group))
        {
            if (freeboardSwathSegment)
            {
                referencedGroupname = getSwathSegmentGroup(shortName, group);
            }
            else if (getVersionNumber(shortName) == ATL10v005)
            {
                referencedGroupname =
                    getFreeboardBeamSegmentGroup(shortName, group);
            }
            else if (getVersionNumber(shortName) == ATL10v006)
            {
                referencedGroupname =
                    getReferenceSurfaceSectionGroup(shortName, group);
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
                referencedGroupname =
                    getFreeboardBeamSegmentGroup(shortName, group);
            }
        }
        else if (isFreeboardRateGroup(shortName, group))
        {
            referencedGroupname =
                getReferenceSurfaceSectionGroup(shortName, group);
        }
        else if (isReferenceSurfaceSectionGroup(shortName, group))
        {
            if (freeboardSwathSegment)
            {
                referencedGroupname = getSwathSegmentGroup(shortName, group);
            }
            else
            {
                referencedGroupname =
                    getReferenceSurfaceSectionGroup(shortName, group);
            }
        }

        LOG_DEBUG("Configuration::getReferencedGroupname() group:"
                  << group << " referencedGroupname: " << referencedGroupname);

        return referencedGroupname;
    }

    /**
     * get target group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getTargetGroupname(const std::string &shortName,
                                   const std::string &group,
                                   const std::string &datasetName = "")
    {
        LOG_DEBUG("Configuration::getTargetGroupname(): ENTER group:" << group);

        std::string targetGroupname;

        if (isSegmentGroup(shortName, group))
        {
            targetGroupname = getPhotonGroupOfSegmentGroup(shortName, group);
        }
        else if (isHeightSegmentRateGroup(shortName, group) &&
                 !isSwathFreeboardGroup(shortName, group))
        {
            targetGroupname = getFreeboardBeamSegmentGroup(shortName, group);
        }
        else if (isSwathFreeboardGroup(shortName, group))
        {
            targetGroupname = getSwathSegmentGroup(shortName, group);
        }
        else if (isFreeboardSwathSegmentGroup(shortName, group))
        {
            targetGroupname = getLeadsGroup(shortName, group, datasetName);
        }
        else if (isFreeboardBeamSegmentGroup(shortName, group) ||
                 isReferenceSurfaceSectionGroup(shortName, group))
        {
            if (isBeamIndex(shortName, datasetName))
            {
                targetGroupname = getLeadsGroup(shortName, group, datasetName);
            }
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
            if (getVersionNumber(shortName) == ATL10v005)
            {
                targetGroupname = getHeightSegmentRateGroup(shortName, group);
            }
            else if (getVersionNumber(shortName) == ATL10v006)
            {
                targetGroupname =
                    getFreeboardSegmentHeightsGroup(shortName, group);
            }
        }
        else if (isFreeboardRateGroup(shortName, group))
        {
            targetGroupname = getReferenceSurfaceSectionGroup(shortName, group);
        }

        LOG_DEBUG("Configuration::getTargetGroupname() group:"
                  << group << "targetGroupname: " << targetGroupname);

        return targetGroupname;
    }

    /**
     * get corresponding photon group for a given segment group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getPhotonGroupOfSegmentGroup(const std::string &shortName,
                                             const std::string &group)
    {
        std::string photonGroup;
        // only if the group is photon group
        if (isSegmentGroup(shortName, group))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                // shortname matched
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    // get the photon and segment group names which are at the
                    // end of path pattern
                    std::string pathDelim("/");
                    std::vector<std::string> paths;
                    split(paths,
                          keyinfo.at("PhotonGroup"),
                          boost::is_any_of(pathDelim));
                    std::string photonGroupName = paths.at(paths.size() - 2);
                    paths.clear();
                    split(paths,
                          keyinfo.at("SegmentGroup"),
                          boost::is_any_of(pathDelim));
                    std::string segmentGroupName = paths.at(paths.size() - 2);
                    // replace photonGroupName with segmentGroupName to get
                    // corresponding segment group'
                    photonGroup = group;
                    boost::replace_last(
                        photonGroup, segmentGroupName, photonGroupName);
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
    std::string getSegmentGroupOfPhotonGroup(const std::string &shortName,
                                             const std::string &group)
    {
        std::string segmentGroup;
        // only if the group is photon group
        if (isPhotonGroup(shortName, group))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                // shortname matched
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    // get the photon and segment group names which are at the
                    // end of path pattern
                    std::string pathDelim("/");
                    std::vector<std::string> paths;
                    split(paths,
                          keyinfo.at("PhotonGroup"),
                          boost::is_any_of(pathDelim));
                    std::string photonGroupName = paths.at(paths.size() - 2);
                    paths.clear();
                    split(paths,
                          keyinfo.at("SegmentGroup"),
                          boost::is_any_of(pathDelim));
                    std::string segmentGroupName = paths.at(paths.size() - 2);
                    // replace photonGroupName with segmentGroupName to get
                    // corresponding segment group'
                    segmentGroup = group;
                    boost::replace_last(
                        segmentGroup, photonGroupName, segmentGroupName);
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
    std::string getSegmentGroupOfPhotonDataset(const std::string &shortName,
                                               const std::string &dataset)
    {
        std::string segmentGroup;

        if (isPhotonDataset(shortName, dataset))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                // shortname matched
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    std::string pathDelim("/");
                    std::vector<std::string> paths;
                    split(paths,
                          keyinfo.at("PhotonDataset"),
                          boost::is_any_of(pathDelim));
                    std::string photonGroup = paths.at(1);
                    paths.clear();
                    split(paths, dataset, boost::is_any_of(pathDelim));
                    std::string photon = paths.at(1);
                    segmentGroup = keyinfo.at("SegmentGroup");
                    boost::replace_last(segmentGroup, photonGroup, photon);
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
    std::string getSwathSegmentGroup(const std::string &shortName,
                                     const std::string &group)
    {
        std::string swathSegmentGroup;

        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find("FreeboardSwathSegmentGroup") != keyinfo.end())
                {
                    swathSegmentGroup =
                        keyinfo.at("FreeboardSwathSegmentGroup");
                }
            }
        }

        return swathSegmentGroup;
    }

    /**
     * get corresponding beam segment group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getFreeboardBeamSegmentGroup(const std::string &shortName,
                                             const std::string &group)
    {
        std::string freeboardBeamSegmentGroup;
        std::vector<std::string> paths;
        std::string pathDelim("/");
        std::string groundTrack;
        std::string groundTrackPattern;

        if ((isHeightSegmentRateGroup(shortName, group) &&
             !isSwathFreeboardGroup(shortName, group)) ||
            isLeadsGroup(shortName, group) ||
            isSwathHeightIndex(shortName, group))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    freeboardBeamSegmentGroup =
                        keyinfo.at("FreeboardBeamSegmentGroup");
                    split(paths, group, boost::is_any_of(pathDelim));
                    groundTrack = paths.at(1);
                    paths.clear();
                    split(paths,
                          freeboardBeamSegmentGroup,
                          boost::is_any_of(pathDelim));
                    groundTrackPattern = paths.at(1);
                    paths.clear();
                    boost::replace_last(freeboardBeamSegmentGroup,
                                        groundTrackPattern,
                                        groundTrack);
                }
            }
        }
        return freeboardBeamSegmentGroup;
    }

    /**
     * get corresponding beam segment group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getBeamFreeboardGroup(const std::string &shortName,
                                      const std::string &group)
    {
        std::string beamFreeboardGroup;
        std::vector<std::string> paths;
        std::string pathDelim("/");
        std::string groundTrack;
        std::string groundTrackPattern;

        if (isHeightSegmentRateGroup(shortName, group) &&
            !isSwathFreeboardGroup(shortName, group))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    beamFreeboardGroup = keyinfo.at("BeamFreeboardGroup");
                    split(paths, group, boost::is_any_of(pathDelim));
                    groundTrack = paths.at(1);
                    paths.clear();
                    split(
                        paths, beamFreeboardGroup, boost::is_any_of(pathDelim));
                    groundTrackPattern = paths.at(1);
                    paths.clear();
                    boost::replace_last(
                        beamFreeboardGroup, groundTrackPattern, groundTrack);
                }
            }
        }
        return beamFreeboardGroup;
    }

    /**
     * get corresponding freeboard segment group for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getFreeboardSegmentGroup(const std::string &shortName,
                                         const std::string &group)
    {
        std::string freeboardSegmentGroup;
        std::vector<std::string> paths;
        std::string pathDelim("/");
        std::string groundTrack;
        std::string groundTrackPattern;

        if (isFreeboardSegmentHeightsGroup(shortName, group) ||
            isFreeboardSegmentGeophysicalGroup(shortName, group) ||
            isSwathHeightIndex(shortName, group))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    freeboardSegmentGroup = keyinfo.at("FreeboardSegmentGroup");
                    split(paths, group, boost::is_any_of(pathDelim));
                    groundTrack = paths.at(1);
                    paths.clear();
                    split(paths,
                          freeboardSegmentGroup,
                          boost::is_any_of(pathDelim));
                    groundTrackPattern = paths.at(1);
                    paths.clear();
                    boost::replace_last(
                        freeboardSegmentGroup, groundTrackPattern, groundTrack);
                }
            }
        }
        return freeboardSegmentGroup;
    }

    /**
     * get corresponding leads group for a given beam freeboard/swath freeboard
     * group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getLeadsGroup(const std::string &shortName,
                              const std::string &group,
                              const std::string &datasetName = "")
    {
        std::string leadsGroup;

        boost::unordered_map<std::string, std::string> groundTrackMap{
            {"gt1", "gt1l"},
            {"gt2", "gt1r"},
            {"gt3", "gt2l"},
            {"gt4", "gt2r"},
            {"gt5", "gt3l"},
            {"gt6", "gt3r"}};

        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                std::string pathDelim("/");
                std::vector<std::string> paths;
                split(paths, group, boost::is_any_of(pathDelim));
                std::string groundTrack;
                if (isSwathFreeboardGroup(shortName, group))
                    groundTrack = paths.at(2);
                else if (isFreeboardBeamSegmentGroup(shortName, group))
                    groundTrack = paths.at(1);
                else if (isReferenceSurfaceSectionGroup(shortName, group))
                    groundTrack = paths.at(1);
                else if (isFreeboardSwathSegmentGroup(shortName, group))
                {
                    paths.clear();
                    std::string pathDelimDash("_");
                    split(paths, datasetName, boost::is_any_of(pathDelimDash));
                    std::string groundTrackPattern = paths.at(paths.size() - 1);
                    if (groundTrackPattern.length() == 3)
                        groundTrack =
                            groundTrackMap.at(paths.at(paths.size() - 1));
                    else
                        groundTrack = paths.at(paths.size() - 1);
                }
                else
                    groundTrack = paths.at(1);
                paths.clear();
                split(paths,
                      keyinfo.at("LeadsGroup"),
                      boost::is_any_of(pathDelim));
                std::string leadsGroupname = paths.at(paths.size() - 2);
                leadsGroup = "/" + groundTrack + "/" + leadsGroupname + "/";
            }
        }
        return leadsGroup;
    }

    /**
     * get corresponding Reference Surface Section for a given group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getReferenceSurfaceSectionGroup(const std::string &shortName,
                                                const std::string &group)
    {
        std::string referenceSurfaceSectionGroup;
        std::vector<std::string> paths;
        std::string pathDelim("/");
        std::string groundTrack;
        std::string groundTrackPattern;

        LOG_DEBUG(
            "Configuration::getReferenceSurfaceSectionGroup(): ENTER group:"
            << group);

        if (isFreeboardRateGroup(shortName, group) ||
            isLeadsGroup(shortName, group) ||
            isSwathHeightIndex(shortName, group))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    referenceSurfaceSectionGroup =
                        keyinfo.at("ReferenceSurfaceSectionGroup");
                    split(paths, group, boost::is_any_of(pathDelim));
                    groundTrack = paths.at(1);
                    paths.clear();
                    split(paths,
                          referenceSurfaceSectionGroup,
                          boost::is_any_of(pathDelim));
                    groundTrackPattern = paths.at(1);
                    paths.clear();
                    boost::replace_last(referenceSurfaceSectionGroup,
                                        groundTrackPattern,
                                        groundTrack);
                }
            }
        }

        LOG_DEBUG("Configuration::getReferenceSurfaceSectionGroup() group:"
                  << group << " referenceSurfaceSectionGroup: "
                  << referenceSurfaceSectionGroup);

        return referenceSurfaceSectionGroup;
    }

    /**
     * get corresponding height segment rate group for a given leads group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getHeightSegmentRateGroup(const std::string &shortName,
                                          const std::string &groupname)
    {

        std::string heightsGroupname;
        std::string pathDelim("/");
        std::vector<std::string> paths;

        if (isLeadsGroup(shortName, groupname))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    heightsGroupname = keyinfo.at("HeightsGroup");
                    split(paths, heightsGroupname, boost::is_any_of(pathDelim));
                    std::string groundTrackPattern = paths.at(1);
                    paths.clear();
                    split(paths, groupname, boost::is_any_of(pathDelim));
                    std::string groundTrack = paths.at(1);
                    boost::replace_last(
                        heightsGroupname, groundTrackPattern, groundTrack);
                }
            }
        }

        return heightsGroupname;
    }

    /**
     * get corresponding freeboard segment heights rate for a given leads group
     * @param shortName product shortname
     * @param group group name
     * @return string corresponding segment group or empty string
     */
    std::string getFreeboardSegmentHeightsGroup(const std::string &shortName,
                                                const std::string &groupname)
    {
        std::string heightsGroupname;
        std::string pathDelim("/");
        std::vector<std::string> paths;

        if (isLeadsGroup(shortName, groupname))
        {
            for (std::map<std::string,
                          std::map<std::string, std::string>>::iterator it =
                     photonSegmentGroups.begin();
                 it != photonSegmentGroups.end();
                 it++)
            {
                std::string shortNamePattern = it->first;
                std::map<std::string, std::string> keyinfo = it->second;
                if (regex_match(shortName, boost::regex(shortNamePattern)))
                {
                    heightsGroupname =
                        keyinfo.at("FreeboardSegmentHeightsGroup");
                    split(paths, heightsGroupname, boost::is_any_of(pathDelim));
                    std::string groundTrackPattern = paths.at(1);
                    paths.clear();
                    split(paths, groupname, boost::is_any_of(pathDelim));
                    std::string groundTrack = paths.at(1);
                    boost::replace_last(
                        heightsGroupname, groundTrackPattern, groundTrack);
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
    void getDatasetNames(const std::string &shortName,
                         const std::string &groupname,
                         std::string &indexBegin,
                         std::string &count)
    {
        LOG_DEBUG(
            "Configuration::getDatasetNames(): ENTER groupname: " << groupname);

        if (isPhotonGroup(shortName, groupname) ||
            isPhotonDataset(shortName, groupname))
        {
            getSegmentDatasetNames(shortName, indexBegin, count);
            if (isPhotonDataset(shortName, groupname))
            {
                getMatchedDatasetNames(groupname, indexBegin, count);
            }
        }
        else if (isHeightSegmentRateGroup(shortName, groupname) ||
                 isFreeboardRateGroup(shortName, groupname))
        {
            getLeadsDatasetNames(shortName, indexBegin, count);
        }
        else if (isLeadsGroup(shortName, groupname))
        {
            if (freeboardSwathSegment)
            {
                getSwathSegmentDatasetNames(
                    shortName, groupname, indexBegin, count, "");
            }
            else
            {
                getBeamSegmentDatasetNames(shortName, indexBegin, count);
            }
        }
    }

    /**
     * get corresponding index begin and count dataset names from given photon
     * dataset
     * @datasetname photon dataset name
     * @indexBegin index begin dataset name
     * @count count dataset name
     */
    void getMatchedDatasetNames(const std::string &datasetname,
                                std::string &indexBegin,
                                std::string &count)
    {
        // return if indexBegin/count dataset names do not require pattern
        // matching
        if (indexBegin.find("[") == std::string::npos)
            return;
        std::string delim("/x");
        std::vector<std::string> paths;
        split(paths, datasetname, boost::is_any_of(delim));
        std::string sub;
        if (datasetname.find("/") == std::string::npos)
            sub = paths.at(0);
        else
            sub = paths.at(paths.size() - 2);
        paths.clear();
        split(paths, indexBegin, boost::is_any_of(delim));
        std::string pattern = paths.at(0);
        paths.clear();
        boost::replace_last(indexBegin, pattern, sub);
        boost::replace_last(count, pattern, sub);
    }

    /**
     * get configured segment photonIndexBegin dataset name
     * @param shortName product shortname
     * @return photonIndexBegin dataset name
     */
    std::string getIndexBeginDatasetName(const std::string &shortName,
                                         const std::string &groupname,
                                         const std::string &datasetName = "",
                                         bool repair = false)
    {
        LOG_DEBUG("Configuration::getIndexBeginDatasetName(): ENTER groupname: "
                  << groupname);

        std::string indexBegin, count;
        if (isSegmentGroup(shortName, groupname))
        {
            getSegmentDatasetNames(shortName, indexBegin, count);
            getMatchedDatasetNames(datasetName, indexBegin, count);
        }
        else if (isLeadsGroup(shortName, groupname))
        {
            getLeadsDatasetNames(shortName, indexBegin, count);
        }
        else if (isBeamFreeboardGroup(shortName, groupname) ||
                 isFreeboardSegmentGroup(shortName, groupname))
        {
            getBeamFreeboardIndexBegin(shortName, indexBegin);
        }
        else if (isFreeboardBeamSegmentGroup(shortName, groupname))
        {
            if (!repair || isSwathHeightIndex(shortName, datasetName))
                getBeamSegmentDatasetNames(shortName, indexBegin);
            else
                getBeamSegmentDatasetNames(shortName, indexBegin, count);
        }
        else if (isReferenceSurfaceSectionGroup(shortName, groupname))
        {
            if (isSwathHeightIndex(shortName, datasetName))
                getBeamSegmentDatasetNames(shortName, indexBegin);
            else
                getBeamSegmentDatasetNames(shortName, indexBegin, count);
        }
        else if (isSwathFreeboardGroup(shortName, groupname))
        {
            getSwathFreeboardIndexName(shortName, indexBegin);
        }
        else if (isFreeboardSwathSegmentGroup(shortName, groupname))
        {
            getSwathSegmentDatasetNames(
                shortName, groupname, indexBegin, count, datasetName);
        }

        LOG_DEBUG("Configuration::getIndexBeginDatasetName() groupname:"
                  << groupname << " datasetName:" << datasetName
                  << " indexBegin:" << indexBegin);

        return indexBegin;
    }

    /**
     * get configured segment segmentPhotonCount dataset name
     * @param shortName product shortname
     * @return segmentPhotonCount dataset name
     */
    std::string getCountDatasetName(const std::string &shortName,
                                    const std::string &groupname,
                                    const std::string &datasetName = "")
    {
        LOG_DEBUG("Configuration::getCountDatasetName(): ENTER groupname: "
                  << groupname);

        std::string indexBegin, count;
        if (isSegmentGroup(shortName, groupname))
        {
            getSegmentDatasetNames(shortName, indexBegin, count);
            getMatchedDatasetNames(datasetName, indexBegin, count);
        }
        else if (isLeadsGroup(shortName, groupname))
        {
            getLeadsDatasetNames(shortName, indexBegin, count);
        }
        else if (isFreeboardSwathSegmentGroup(shortName, groupname))
        {
            getSwathSegmentDatasetNames(
                shortName, groupname, indexBegin, count, datasetName);
        }
        else if (isFreeboardBeamSegmentGroup(shortName, groupname) ||
                 isReferenceSurfaceSectionGroup(shortName, groupname))
        {
            getBeamSegmentDatasetNames(shortName, indexBegin, count);
        }

        LOG_DEBUG("Configuration::getCountDatasetName() groupname:"
                  << groupname << " datasetName:" << datasetName
                  << " indexBegin:" << indexBegin << " count:" << count);

        return count;
    }

    /**
     * get configured segment and photon dataset names
     * @param shortName product shortname
     * @param photonIndexBegin out: photonIndexBegin dataset name
     * @param segmentPhotonCount out: segmentPhotonCount datasetname
     */
    void getSegmentDatasetNames(const std::string &shortName,
                                std::string &photonIndexBegin,
                                std::string &segmentPhotonCount)
    {
        //// first check shortname matches,
        // then get the datasetnames
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            // shortname matched
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find("PhotonIndexBegin") != keyinfo.end() &&
                    keyinfo.find("SegmentPhotonCount") != keyinfo.end())
                {
                    photonIndexBegin = keyinfo.at("PhotonIndexBegin");
                    segmentPhotonCount = keyinfo.at("SegmentPhotonCount");
                    break;
                }
            }
        }
    }

    /**
     * get configured index begin and count dataset names from leads
     * group(ATL10)
     * @param shortName product shortname
     * @param indexBegin out: index begin dataset name
     * @param count out: count datasetname
     */
    void getLeadsDatasetNames(const std::string &shortName,
                              std::string &indexBegin,
                              std::string &count)
    {
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find("LeadsIndex") != keyinfo.end() &&
                    keyinfo.find("LeadsCount") != keyinfo.end())
                {
                    indexBegin = keyinfo.at("LeadsIndex");
                    count = keyinfo.at("LeadsCount");
                    break;
                }
            }
        }
    }

    /**
     * get configured index begin and count dataset names for swath segment
     * group(ATL10)
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name
     * @param count out: count datasetname
     */
    void getSwathSegmentDatasetNames(const std::string &shortName,
                                     const std::string &groupname,
                                     std::string &indexBegin,
                                     std::string &count,
                                     const std::string &datasetName)
    {
        boost::unordered_map<std::string, std::string> groundTrack{
            {"gt1l", "gt1"},
            {"gt1r", "gt2"},
            {"gt2l", "gt3"},
            {"gt2r", "gt4"},
            {"gt3l", "gt5"},
            {"gt3r", "gt6"}};
        std::string pathDelim("_/");
        std::vector<std::string> paths;

        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find("SwathIndex") != keyinfo.end() &&
                    keyinfo.find("SwathCount") != keyinfo.end())
                {
                    indexBegin = keyinfo.at("SwathIndex");
                    count = keyinfo.at("SwathCount");
                    std::string groundTrackNum;
                    split(paths, indexBegin, boost::is_any_of(pathDelim));
                    std::string groundTrackPattern = paths.at(paths.size() - 1);
                    paths.clear();
                    if (!datasetName.empty())
                    {
                        split(paths, datasetName, boost::is_any_of(pathDelim));
                        groundTrackNum = paths.at(paths.size() - 1);
                        paths.clear();
                    }
                    else
                    {
                        split(paths, groupname, boost::is_any_of(pathDelim));
                        groundTrackNum = paths.at(1);
                        paths.clear();
                    }
                    boost::replace_last(
                        indexBegin, groundTrackPattern, groundTrackNum);
                    boost::replace_last(
                        count, groundTrackPattern, groundTrackNum);
                    break;
                }
            }
        }
    }

    void groundTrackRename(std::string groupname,
                           std::string &indexBegin,
                           std::string &count)
    {
        boost::unordered_map<std::string, std::string> groundTrack{
            {"gt1l", "gt1"},
            {"gt1r", "gt2"},
            {"gt2l", "gt3"},
            {"gt2r", "gt4"},
            {"gt3l", "gt5"},
            {"gt3r", "gt6"}};
        std::string pathDelim("_/");
        std::vector<std::string> paths;

        split(paths, groupname, boost::is_any_of(pathDelim));
        std::string groundTrackNum = groundTrack.at(paths.at(1));
        boost::replace_last(indexBegin, paths.at(1), groundTrackNum);
        boost::replace_last(count, paths.at(1), groundTrackNum);
    }

    /**
     * get configured index begin and count dataset names for swath segment
     * group(ATL10)
     * @param shortName product shortname
     * @param groupname: group name
     * @param indexBegin out: index begin dataset name
     */
    void getBeamSegmentDatasetNames(const std::string &shortName,
                                    std::string &indexBegin,
                                    std::string &count)
    {
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find("BeamIndex") != keyinfo.end() &&
                    keyinfo.find("BeamCount") != keyinfo.end())
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
    void getBeamFreeboardIndexBegin(const std::string &shortName,
                                    std::string &indexBegin)
    {
        LOG_DEBUG(
            "Configuration::getBeamFreeboardIndexBegin(): ENTER indexBegin: "
            << indexBegin);

        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
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
    void getBeamSegmentDatasetNames(const std::string &shortName,
                                    std::string &indexBegin)
    {
        LOG_DEBUG(
            "Configuration::getBeamSegmentDatasetNames(): ENTER indexBegin: "
            << indexBegin);

        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
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
    void getSwathFreeboardIndexName(const std::string &shortName,
                                    std::string &indexBegin)
    {
        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
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
    bool isSwathHeightIndex(const std::string &shortName,
                            const std::string datasetName)
    {
        bool matched = false;

        for (std::map<std::string, std::map<std::string, std::string>>::iterator
                 it = photonSegmentGroups.begin();
             it != photonSegmentGroups.end();
             it++)
        {
            std::string shortNamePattern = it->first;
            std::map<std::string, std::string> keyinfo = it->second;
            if (regex_match(shortName, boost::regex(shortNamePattern)))
            {
                if (keyinfo.find("SwathHeightIndex") != keyinfo.end())
                {
                    std::string datasetPattern = keyinfo.at("SwathHeightIndex");
                    if (regex_search(datasetName, boost::regex(datasetPattern)))
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
     * Add shortname, group, and dataset from granule file to cache.
     * @param shortName product shortname
     * @param group: group
     */
    void addShortNameGroupDatasetFromGranuleFile(const std::string &shortName,
                                                 const std::string &group)
    {
        shortNameGroupDatasetFromGranuleFile[shortName].push_back(group);
    }

    /**
     * Check if shortname, group, or dataset exists granule file cache.
     * @param shortName product shortname
     * @param group: group
     * @return Return TRUE if shortname, group, or dataset exists.  FALSE
     * otherwise
     */
    bool isShortNameGroupDatasetFromGranuleFile(const std::string &shortName,
                                                const std::string &group)
    {
        LOG_DEBUG("Configuration::isShortNameGroupDatasetFromGranuleFile(): "
                  "ENTER group: "
                  << group);

        bool patternMatched = false;
        std::string shortNamePattern("");

        for (const auto &keyinfo : shortNameGroupDatasetFromGranuleFile)
        {
            shortNamePattern = keyinfo.first;
            for (const auto &valueinfo : keyinfo.second)
            {
                if (shortNamePattern == shortName && valueinfo == group)
                {
                    LOG_DEBUG("Configuration::"
                              "isShortNameGroupDatasetFromGranuleFile() "
                              "FOUND  shortName: "
                              << shortNamePattern << " group: " << valueinfo);
                    patternMatched = true;
                    break;
                }
            }
        }

        return patternMatched;
    }

    /**
     * Retrieve the shortname+version number from the cache storage of the
     * granule.
     * @param shortName product shortname
     * @return Return shortname/version number string of granule (Ex. ATL10v005)
     */
    std::string getVersionNumber(const std::string &shortName)
    {
        LOG_DEBUG("Configuration::getVersionNumber(): ENTER shortName: "
                  << shortName);

        const std::string atl10v005("/gt[\\w]+/freeboard_beam_segment/");
        const std::string atl10v006("/gt[\\w]+/freeboard_segment/");

        for (const auto &keyinfo : shortNameGroupDatasetFromGranuleFile)
        {
            for (const auto &valueinfo : keyinfo.second)
            {
                if (keyinfo.first == shortName &&
                    regex_match(valueinfo, boost::regex(atl10v005)))
                {
                    LOG_DEBUG(
                        "Configuration::getVersionNumber() FOUND version = 005 "
                        "shortName: "
                        << keyinfo.first << " group: " << valueinfo);
                    return ATL10v005;
                }
                else if (keyinfo.first == shortName &&
                         regex_match(valueinfo, boost::regex(atl10v006)))
                {
                    LOG_DEBUG(
                        "Configuration::getVersionNumber() FOUND version = 006 "
                        "shortName: "
                        << keyinfo.first << " group: " << valueinfo);
                    return ATL10v006;
                }
            }
        }

        LOG_ERROR(
            "Configuration::getVersionNumber() Failed to retrieve short name: "
            << shortName << " version number");

        // Return empty string
        return std::string();
    }

  private:
    // return a value in vectorB which is also in vectorA,
    // an empty string is returned if no matches
    std::string findStringInBothVectors(std::vector<std::string> vectorA,
                                        std::vector<std::string> vectorB)
    {
        std::string returnValue;
        for (std::vector<std::string>::iterator ait = vectorA.begin();
             ait != vectorA.end();
             ait++)
        {
            std::string value = *ait;
            std::vector<std::string>::iterator it =
                find(vectorB.begin(), vectorB.end(), value);
            if (it != vectorB.end())
            {
                returnValue = *it;
            }
        }
        return returnValue;
    }

    // check if a given string matches any regexp on the list, only search if
    // the string has the pattern since the string can have extra information
    // e.g. value: /profile_1/calibration matches the regexpList which has
    // /profile_1
    bool matchRegexpList(std::vector<std::string> regexpList, std::string value)
    {
        bool matchFound = false;
        for (std::vector<std::string>::iterator it = regexpList.begin();
             it != regexpList.end();
             it++)
        {
            if (regex_search(value, boost::regex(*it)))
            {
                matchFound = true;
                break;
            }
        }
        return matchFound;
    }

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
    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        coordinateDatasetNames;

    /**
     * key: shortname pattern, value: epoch time
     * e.g.
     * "ATL[\\d]{2}_SDP": "2005-01-01T00.00.00.000000"
     */
    std::map<std::string, std::string> productEpochs;

    /**
     * subsettable groups
     * key: shortname pattern
     * value: list of group patterns
     */
    std::map<std::string, std::vector<std::string>> subsettableGroups;

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
    std::map<std::string, std::vector<std::string>> unsubsettableGroups;

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
    std::map<std::string, std::map<std::string, std::string>>
        photonSegmentGroups;

    /**
     * shortname attribute path
     * key: mission name
     * value: shortname attribute path
     */
    std::vector<std::string> shortnames;

    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        superCoordinateDatasets;

    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        superGroups;

    std::map<std::string, std::string> missions;

    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        requiredDatasets;

    std::map<std::string, std::map<std::string, std::string>> resolutions;

    std::map<std::string, std::string> projections;

    bool freeboardSwathSegment;

    /**
     * Cache of shortname, group, and dataset from granule file
     * key: shortname pattern
     * value: list of group and dataset patterns
     */
    std::map<std::string, std::vector<std::string>>
        shortNameGroupDatasetFromGranuleFile;

    const std::string ATL10v005 = "ATL10v005";
    const std::string ATL10v006 = "ATL10v006";
};
#endif
