#ifndef SUBSET_DATA_LAYERS_H
#define SUBSET_DATA_LAYERS_H

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <algorithm>
#include "H5Cpp.h"
#include <H5DataSpace.h>
#include <H5DataType.h>
#include <H5Exception.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace H5;
using namespace boost;
namespace pt = boost::property_tree;

class SubsetDataLayers{
    
    // data structure that keeps tracks of datasets to be included in the output
    // the vector is ordered by the depth of the subset data layer
    // each set contains strings of specified subset data layers at that depth
    //    Note: when all datasets are included, first set of the vector would contain "/"
    //          otherwise, emptyset
    //    Examples: second set would be all one-level data layers (/1, /2)
    //              third set would be all two-level data layers(/1/2, /1/3)
    vector< set <string> > datasets;
    bool include_all;
    
public:
    
    string json_in_name; // input file name for reading from a json file
    vector <string> dataset_to_include; // contains lists of "includedataset" parameter
    
    // constructor
    // parse comma-separated lists of "includedataset", and add it to "datasets"
    SubsetDataLayers(vector <string> dataset_to_include):dataset_to_include(dataset_to_include)
    {
        // if "includeddataset" is specified, add each dataset to "datasets"
        if (!dataset_to_include.empty())
        {
            include_all = false;
            for (vector<string>::iterator it = dataset_to_include.begin(); it != dataset_to_include.end(); it++)
            {
                string dataset;
                char_separator<char> sep(",");
                tokenizer<char_separator<char> > tokens(*it, sep);
                tokenizer<char_separator<char> >::iterator iter = tokens.begin();
                for (; iter != tokens.end(); iter++)
                {
                    dataset=*iter;
                    if (*dataset.rbegin() != '/') dataset.push_back('/');
                    cout << "adding dataset to include " << dataset << endl;
                    add_dataset(dataset);
                }
            }
            add_dataset("/Metadata/");
            add_dataset("/METADATA/");
        }
        // else, include all datasets
        else
        {
            cout << "adding / dataset to include all" << endl;
            include_all = true;
            add_dataset("/");
        }

    };
    
    // constructor
    // read-in specified subset data layers from a json file when it is provided
    SubsetDataLayers(string json_in_name):json_in_name(json_in_name)
    {
        include_all = false;
        read_from_json(json_in_name);
        add_dataset("/Metadata/");
        add_dataset("/METADATA/");
    };
 
    // add all descendants of groups that are included in the output to "datasets"
    void expand_all(string input_filename)
    {                
        //cout << "expand_all" << endl;
        H5File infile(input_filename, H5F_ACC_RDONLY);
        Group ingroup = infile.openGroup("/");
        expand_group(ingroup, "");
    }

    // check to see if a dataset is one of the datasets to  be included in the output
    bool is_dataset_included(string str)
    {        
        if (*str.rbegin() != '/') str.push_back('/');
        if (include_all) return true;
        
        //cout << "str: " << str << endl;
        
        string ancestor("");
        size_t next_slash;
        vector < set <string> >::iterator it = datasets.begin();

        while(it != datasets.end())
        {
            next_slash = str.find('/', ancestor.size());
            //cout << "next_slash: " << next_slash << endl;
            if (next_slash != string::npos)
            {
                ancestor = str.substr(0, next_slash+1);
                //cout << "ancestor: " << ancestor << endl;
                if (it->find(ancestor) != it->end()){
                    //cout << "is dataset included - true" << endl;
                    return true;
                }
                it++;
            }
            else
            {
                //cout << "ancestor: " << ancestor << endl;
                //cout << "(it->find(ancestor) != it->end(): " << (it->find(ancestor) != it->end()) << endl;
                return (it->find(ancestor) != it->end());
            }
        }
        
        return false;
    }
    
    // check to see if any of its children are included in the output
    bool is_child_included(string str)
    {                
        if (*str.rbegin() != '/') str.push_back('/');
        if (include_all) return true;
        
        vector < set <string> >::iterator it = datasets.begin();
        
        while(it != datasets.end())
        {
            set<string>::iterator set_iter = it->begin();
            while(set_iter!=it->end())
            {
                if (set_iter->find(str)==0) return true;
                set_iter++;
            }
            it++;
        }
        
        return false;
    }
    
    // check to see if it is one of the included datasets or any of its children are included in the output
    bool is_included(string str)
    {
        return (is_child_included(str)||is_dataset_included(str));
    }
    
    // prints all included datasets
    void print_datasets()
    {        
        cout << "printing the subset data layers" << endl;;

        int count = 0;
        vector< set <string> >::iterator it = datasets.begin();
        
        // loop through sets in the vector
        while  (it!=datasets.end())
        {
            set<string> temp_set = *it;
            set<string>::iterator set_it = temp_set.begin();
            
            // loop through strings in the set
            while (set_it!=temp_set.end())
            {
                cout << *set_it << ", ";
                set_it++;
            }
            it++;
            count++;
            cout << endl;
        }
    }    
    
    // writes all included datasets to a json file
    void write_to_json(string json_out_name)
    {
        pt::ptree root;
        pt::ptree data_set_layers; // list(node) stores included datasets
        
        vector< set <string> >::iterator it = datasets.begin();
        
        // loop through sets in the vector
        while (it != datasets.end())
        {
            set<string> temp = *it;
            set<string>::iterator iter = temp.begin();
            
            // loop through strings in the set
            while (iter!=temp.end())
            {
                // create an unnamed node with value
                pt::ptree data_set_layer;
                data_set_layer.put("", *iter);
                
                // add the node to the list
                data_set_layers.push_back(std::make_pair("", data_set_layer));
                iter++;
            }
            it++;
        }
        // add the list to the root
        root.add_child("data_set_layers", data_set_layers);
        
        cout << "writing to the json file: " << json_out_name << endl;
        
        //write to json
        pt::write_json(json_out_name, root);
    }
    
    vector < set <string> > getDatasets() { return this->datasets; }
    
    // add descendants of a group that are included in the output to "datasets"
    // ingroup IN: input h5 group
    // groupname IN: group name
    void expand_group(Group ingroup, string groupName)
    {        
        //cout << "expand_group" << endl;
        size_t numObjs = ingroup.getNumObjs();
        // loop through all the objects in the group
        for (int i = 0; i < numObjs; i++)
        {
            string obj_name = ingroup.getObjnameByIdx(i);
            string full_name = groupName + "/"+obj_name;
            string type_name;
            ingroup.getObjTypeByIdx(i, type_name);
            // if the object is a group, and it is one of the group to be included or 
            // it is an ancestor of one of the included datasets
            if (type_name == "group" && is_included(full_name))
            {
                // recurse on it, until all of its descendants have been expanded
                expand_group(ingroup.openGroup(obj_name), full_name);
            }
            else if (type_name == "dataset" && is_dataset_included(full_name))
            {
                if (*full_name.rbegin() != '/') full_name.push_back('/');
                add_expanded_dataset(full_name);
            }        
        }
        remove_expanded_group(groupName);        
    }
    
private:
    
    // add a dataset to "datasets" only if its ancestor is not already included
    void add_dataset(string str)
    {   
        string ancestor = "";
        set<string> empty_set;
        size_t next_slash;

        if (datasets.size() == 0) datasets.push_back(empty_set);
        
        vector< set <string> >::iterator it = datasets.begin();
        
        next_slash = str.find('/',1);
        
        while (next_slash != string::npos)
        {           
            it++;
            if (it == datasets.end()) it = datasets.insert(it, empty_set);
            ancestor = str.substr(0, next_slash+1);
            next_slash = str.find('/', ancestor.size()+1);
            if (it->find(ancestor) != it->end()) return;
        }
        
        if (it->find(str) == it->end())
        {
            it->insert(str);
            cout << "inserted dataset: " << str << endl;
            it++;
            while (it != datasets.end())
            {
                set<string>::iterator set_iter = it->begin();
                while(set_iter!=it->end())
                {
                    if (set_iter->find(str)==0) it->erase(set_iter);
                    set_iter++;
                }
                it++;
            }
        }      
    }
    

    
    void add_expanded_dataset(string str)
    {
        int num_of_slashes = std::count(str.begin(), str.end(), '/');
        set<string> s = datasets.back();
        set<string> empty_set;
        
        if (num_of_slashes <= datasets.size())
        {
            datasets.at(num_of_slashes-1).insert(str);
            //cout << "inserted dataset: " << str << endl;
        }
        else
        {
            while (num_of_slashes > datasets.size())
            {
                datasets.insert(datasets.end(), empty_set);
            }
            datasets.back().insert(str);
            //cout << "inserted dataset: " << str << endl;
        }
    }
    
    // removes groups that has already been expanded
    void remove_expanded_group(string str)
    {
        if (*str.rbegin() != '/') str.push_back('/');
        int num_of_slashes = std::count(str.begin(), str.end(), '/');
        if (datasets.size() > num_of_slashes-1 && is_dataset_included(str))
        {
            datasets.at(num_of_slashes-1).erase(str);
        }

    }
    
    // read datasets to be included in the output from a json file
    void read_from_json(string json_in_name)
    {       
        cout << "reading from the json file: " << json_in_name << endl;
        
        pt::ptree root;
        
        string str, dataset;

        // read the json file
        pt::read_json(json_in_name, root);
        // for each node in "data_set_layers", add it to "datasets"
        BOOST_FOREACH(pt::ptree::value_type &data_layer, root.get_child("data_set_layers"))
        {
            str = data_layer.second.get_value<string>();
            char_separator<char> delim(" ,");
            tokenizer<char_separator<char> > datasets(str, delim);
            BOOST_FOREACH(dataset, datasets)
            {
                if (*dataset.rbegin() != '/') dataset.push_back('/');
                add_dataset(dataset);
            }
        }
    } 

};

#endif

