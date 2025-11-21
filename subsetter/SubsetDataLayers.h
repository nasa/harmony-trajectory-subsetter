#ifndef SUBSET_DATA_LAYERS_H
#define SUBSET_DATA_LAYERS_H

#include "H5Cpp.h"
#include "LogLevel.h"
#include <H5DataSpace.h>
#include <H5DataType.h>
#include <H5Exception.h>
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace property_tree = boost::property_tree;

class SubsetDataLayers
{

    // data structure that keeps tracks of datasets to be included in the output
    // the vector is ordered by the depth of the subset data layer
    // each set contains strings of specified subset data layers at that depth
    //    Note: when all datasets are included, first set of the vector would
    //    contain "/"
    //          otherwise, emptyset
    //    Examples: second set would be all one-level data layers (/1, /2)
    //              third set would be all two-level data layers(/1/2, /1/3)
    std::vector<std::set<std::string>> datasets;
    bool include_all;

  public:
    std::string json_in_name; // input file name for reading from a json file
    std::vector<std::string>
        dataset_to_include; // contains lists of "includedataset" parameter

    // constructor
    // parse comma-separated lists of "includedataset", and add it to "datasets"
    SubsetDataLayers(std::vector<std::string> dataset_to_include)
        : dataset_to_include(dataset_to_include)
    {
        LOG_DEBUG("SubsetDataLayers::SubsetDataLayers(): ENTER");

        // if "includeddataset" is specified, add each dataset to "datasets"
        if (!dataset_to_include.empty())
        {
            include_all = false;
            for (std::vector<std::string>::iterator it =
                     dataset_to_include.begin();
                 it != dataset_to_include.end();
                 it++)
            {
                std::string dataset;
                boost::char_separator<char> sep(",");
                boost::tokenizer<boost::char_separator<char>> tokens(*it, sep);
                boost::tokenizer<boost::char_separator<char>>::iterator iter =
                    tokens.begin();
                for (; iter != tokens.end(); iter++)
                {
                    dataset = *iter;
                    if (*dataset.rbegin() != '/')
                        dataset.push_back('/');
                    LOG_DEBUG("SubsetDataLayers::SubsetDataLayers(): adding "
                              "dataset to include "
                              << dataset);
                    add_dataset(dataset);
                }
            }
            add_dataset("/Metadata/");
            add_dataset("/METADATA/");
        }
        // else, include all datasets
        else
        {
            LOG_DEBUG(
                "SubsetDataLayers::SubsetDataLayers(): adding / dataset to "
                "include all");
            include_all = true;
            add_dataset("/");
        }
    };

    // constructor
    // read-in specified subset data layers from a json file when it is provided
    SubsetDataLayers(std::string json_in_name) : json_in_name(json_in_name)
    {
        include_all = false;
        read_from_json(json_in_name);
        add_dataset("/Metadata/");
        add_dataset("/METADATA/");
    };

    // add all descendants of groups that are included in the output to
    // "datasets"
    void expand_all(std::string input_filename)
    {
        H5::H5File infile(input_filename, H5F_ACC_RDONLY);
        H5::Group ingroup = infile.openGroup("/");
        expand_group(ingroup, "");
    }

    // check to see if a dataset is one of the datasets to  be included in the
    // output
    bool is_dataset_included(std::string str)
    {
        if (*str.rbegin() != '/')
            str.push_back('/');
        if (include_all)
            return true;

        std::string ancestor("");
        size_t next_slash;
        std::vector<std::set<std::string>>::iterator it = datasets.begin();

        while (it != datasets.end())
        {
            next_slash = str.find('/', ancestor.size());
            if (next_slash != std::string::npos)
            {
                ancestor = str.substr(0, next_slash + 1);
                if (it->find(ancestor) != it->end())
                {
                    return true;
                }
                it++;
            }
            else
            {
                return (it->find(ancestor) != it->end());
            }
        }

        return false;
    }

    // check to see if any of its children are included in the output
    bool is_child_included(std::string str)
    {
        if (*str.rbegin() != '/')
            str.push_back('/');
        if (include_all)
            return true;

        std::vector<std::set<std::string>>::iterator it = datasets.begin();

        while (it != datasets.end())
        {
            std::set<std::string>::iterator set_iter = it->begin();
            while (set_iter != it->end())
            {
                if (set_iter->find(str) == 0)
                    return true;
                set_iter++;
            }
            it++;
        }

        return false;
    }

    // check to see if it is one of the included datasets or any of its children
    // are included in the output
    bool is_included(std::string str)
    {
        return (is_child_included(str) || is_dataset_included(str));
    }

    // prints all included datasets
    void print_datasets()
    {
        LOG_DEBUG("SubsetDataLayers::print_datasets(): printing the subset "
                  "data layers");

        int count = 0;
        std::vector<std::set<std::string>>::iterator it = datasets.begin();

        // loop through sets in the vector
        while (it != datasets.end())
        {
            std::set<std::string> temp_set = *it;
            std::set<std::string>::iterator set_it = temp_set.begin();

            // loop through strings in the set
            while (set_it != temp_set.end())
            {
                LOG_DEBUG(*set_it << ", ");
                set_it++;
            }
            it++;
            count++;
        }
    }

    // writes all included datasets to a json file
    void write_to_json(std::string json_out_name)
    {
        LOG_DEBUG("SubsetDataLayers::write_to_json(): ENTER");

        property_tree::ptree root;
        property_tree::ptree
            data_set_layers; // list(node) stores included datasets

        std::vector<std::set<std::string>>::iterator it = datasets.begin();

        // loop through sets in the vector
        while (it != datasets.end())
        {
            std::set<std::string> temp = *it;
            std::set<std::string>::iterator iter = temp.begin();

            // loop through strings in the set
            while (iter != temp.end())
            {
                // create an unnamed node with value
                property_tree::ptree data_set_layer;
                data_set_layer.put("", *iter);

                // add the node to the list
                data_set_layers.push_back(std::make_pair("", data_set_layer));
                iter++;
            }
            it++;
        }
        // add the list to the root
        root.add_child("data_set_layers", data_set_layers);

        LOG_DEBUG(
            "SubsetDataLayers::write_to_json(): writing to the json file: "
            << json_out_name);

        // write to json
        property_tree::write_json(json_out_name, root);
    }

    std::vector<std::set<std::string>> getDatasets() { return this->datasets; }

    // add descendants of a group that are included in the output to "datasets"
    // ingroup IN: input h5 group
    // groupname IN: group name
    void expand_group(H5::Group ingroup, std::string groupName)
    {
        size_t numObjs = ingroup.getNumObjs();
        // loop through all the objects in the group
        for (int i = 0; i < numObjs; i++)
        {
            std::string obj_name = ingroup.getObjnameByIdx(i);
            std::string full_name = groupName + "/" + obj_name;
            std::string type_name;
            ingroup.getObjTypeByIdx(i, type_name);
            // if the object is a group, and it is one of the group to be
            // included or it is an ancestor of one of the included datasets
            if (type_name == "group" && is_included(full_name))
            {
                // recurse on it, until all of its descendants have been
                // expanded
                expand_group(ingroup.openGroup(obj_name), full_name);
            }
            else if (type_name == "dataset" && is_dataset_included(full_name))
            {
                if (*full_name.rbegin() != '/')
                    full_name.push_back('/');
                add_expanded_dataset(full_name);
            }
        }
        remove_expanded_group(groupName);
    }

  private:
    // Add a dataset to "datasets" only if its ancestor is not already included.
    // Purge lower-level datasets if their ancestor is being added.
    void add_dataset(std::string str)
    {
        std::string ancestor = "";
        std::set<std::string> empty_set;
        size_t next_slash;

        if (datasets.size() == 0)
        {
            datasets.push_back(empty_set);
        }

        std::vector<std::set<std::string>>::iterator it = datasets.begin();

        next_slash = str.find('/', 1);

        // Step through levels of added dataset string, treating levels as
        // ancestors. Step down through levels of datasets and exit if ancestor
        // is found.
        while (next_slash != std::string::npos)
        {
            it++; // Step down into datasets.

            if (it == datasets.end())
            {
                // If no set of variables are found at this level, add an empty
                // set at this level.
                // (insert of dataset follows)
                it = datasets.insert(it, empty_set);
            }

            // Expand ancestor string to include next level.
            ancestor = str.substr(0, next_slash + 1);

            // Move end-marker to next '/', starting at current end-marker.
            // Current next_slash == ancestor.size().
            next_slash = str.find('/', ancestor.size() + 1);
            if (it->find(ancestor) != it->end())
            {
                // Found the ancestor string in the set at this level (it).
                return;
            }
        }

        // If full dataset string is not found at this level, insert dataset.
        if (it->find(str) == it->end())
        {
            it->insert(str);
            LOG_DEBUG(
                "SubsetDataLayers::add_dataset(): inserted dataset: " << str);

            // Skip to the next level, purge any lower-level datasets
            // where the new dataset being added is an ancestor.
            it++;
            while (it != datasets.end())
            {
                // Now start iterating through the datasets within the set
                // at this level.
                std::set<std::string>::iterator set_iter = it->begin();
                while (set_iter != it->end())
                {
                    // Look for the new dataset being added as a prefix in any
                    // of the datasets withing the current set.
                    if (set_iter->find(str) == 0)
                    {
                        // Remove the set-item that has matching prefix.
                        // Compute the parameter set-iter, iterate, and call
                        // erase.
                        it->erase(set_iter++);
                    }
                    else
                    {
                        set_iter++;
                    }
                }
                it++;
            }
        }
    }

    void add_expanded_dataset(std::string str)
    {
        int num_of_slashes = std::count(str.begin(), str.end(), '/');
        std::set<std::string> s = datasets.back();
        std::set<std::string> empty_set;

        if (num_of_slashes <= datasets.size())
        {
            datasets.at(num_of_slashes - 1).insert(str);
        }
        else
        {
            while (num_of_slashes > datasets.size())
            {
                datasets.insert(datasets.end(), empty_set);
            }
            datasets.back().insert(str);
        }
    }

    // removes groups that has already been expanded
    void remove_expanded_group(std::string str)
    {
        if (*str.rbegin() != '/')
            str.push_back('/');
        int num_of_slashes = std::count(str.begin(), str.end(), '/');
        if (datasets.size() > num_of_slashes - 1 && is_dataset_included(str))
        {
            datasets.at(num_of_slashes - 1).erase(str);
        }
    }

    // read datasets to be included in the output from a json file
    void read_from_json(std::string json_in_name)
    {
        LOG_DEBUG(
            "SubsetDataLayers::read_from_json(): reading from the json file: "
            << json_in_name);

        property_tree::ptree root;

        std::string str, dataset;

        // read the json file
        property_tree::read_json(json_in_name, root);
        // for each node in "data_set_layers", add it to "datasets"
        BOOST_FOREACH (property_tree::ptree::value_type &data_layer,
                       root.get_child("data_set_layers"))
        {
            str = data_layer.second.get_value<std::string>();
            boost::char_separator<char> delim(" ,");
            boost::tokenizer<boost::char_separator<char>> datasets(str, delim);
            BOOST_FOREACH (dataset, datasets)
            {
                if (*dataset.rbegin() != '/')
                    dataset.push_back('/');
                add_dataset(dataset);
            }
        }
    }
};

#endif
