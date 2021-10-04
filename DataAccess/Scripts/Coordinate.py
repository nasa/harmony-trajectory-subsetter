#!/tools/miniconda/bin/python

import sys
import os
import shapefile
import numpy as np
import h5py
import collections
import json
import re

from SubsetDataLayers_3 import SubsetDataLayers

class Coordinate:

    def __init__(self, inputfile, subsetDataLayers):
        self.subsetDataLayers = subsetDataLayers;
        self.subsetDataLayers.expand(inputfile);
        datasets = self.subsetDataLayers.datasets;

        inf = h5py.File(inputfile, 'r');

        currentDirectory = sys.path[0];
        mode = currentDirectory.split('/')[3];
        configfile = "/usr/ecs/" + mode + "/CUSTOM/data/DPL/SubsetterConfig.json";

        # open the config file and get lat/lon names
        cfgfile = open(configfile).read();
        fixed_json = self.removeComments(cfgfile);
        config = json.loads(fixed_json);
        coorDatasetNames = config["CoordinateDatasetNames"];
        self.latNames = [];
        self.lonNames = [];
        for i, (key, value) in enumerate(coorDatasetNames.items()):
            self.latNames += coorDatasetNames[key]["latitude"];
            self.lonNames += coorDatasetNames[key]["longitude"];

        # stores coordinate references and the dataset names that have reference to these coordinate datasets
        #     key: coordinate dataset names(/<groupname>/<latitude>, /<groupname>/<longitude>
        #     value: dataset name
        self.coorDatasets = collections.OrderedDict();

        self.getCoordinates(inf);

    ''' looping through input file to get the coordinates references from datasets
    Args:
        g(Dataset/Group): group/dataset to be processed in order to get the coordinates reference
        groupLat(string): latitude name found in dataset names in the group
        groupLon(string): longitude name found in dataset names in the group
    '''
    def getCoordinates(self, g, groupLat=None, groupLon=None):
        for key, value in g.items():
            name = value.name;
            # if it is a dataset
            if isinstance(value, h5py.Dataset):
                datasetName = name.rpartition("/")[2];
                
                # if dataset itself is a lat/lon dataset
                if datasetName in self.latNames: groupLat = name;
                elif datasetName in self.lonNames: groupLon = name;

                # if the dataset is one of the included datasets
                if self.subsetDataLayers.isDatasetIncluded(name):
                    groupname = name.rsplit("/", 1)[0];
                    coorLat, coorLon = "", "";

                    if "coordinates" in value.attrs:
                        coorAttr = value.attrs["coordinates"];
                        #coors = coorAttr.split(", ");
                        if isinstance(coorAttr, bytes): coorAttr = coorAttr.decode()
                        coors = re.split(",| ", coorAttr);
                        count = coors[0].count("../");
                        for i in range(count):
                            groupname = groupname.rsplit("/", 1)[0];
                        for coor in coors:
                            coor = coor.replace("../", "");
                            path = groupname + "/" + coor;
                            if coor in self.latNames: 
                                coorLat = os.path.abspath(path);
                            elif coor in self.lonNames:
                                coorLon = os.path.abspath(path);

                    # if no latitude found in coordinates attribute but found in dataset names
                    if not coorLat and groupLat:
                        if groupLat.rpartition("/")[0] != groupname: continue;
                        coorLat = groupLat;
                    # if no longitude found in coordinates attribute but found in dataset names
                    if not coorLon and groupLon:
                        if groupLon.rpartition("/")[0] != groupname: continue;
                        coorLon = groupLon;
                
                    # if no latitude/longitude found
                    if not coorLat and not coorLon: continue;
           
                    # if latitude/longitude does not exist
                    if (coorLat not in g) or (coorLon not in g): continue;

                    # construct the key for the dictionary that stores coordinate references and 
                    # datasets that are referencing to these coordinates
                    if coorLat and coorLon: coorDatasetNames = coorLat + ", " + coorLon;

                    # inconsistent dataset size
                    if g[coorLat].shape[0] != value.shape[0]: 
                        print ("Inconsistent dataset size:", name);
                        continue;

                    # add dataset name to the dictionary according to its coordinate reference
                    if coorDatasetNames not in self.coorDatasets: self.coorDatasets[coorDatasetNames] = [];
                    self.coorDatasets[coorDatasetNames].append(str(name));
      
                    # multi-dimensional datasets
                    if g[name].ndim > 1:
                        for i in range(1, g[name].shape[1]):
                            datasetName = name.split("/")[-1];
                            datasetName = datasetName + "_" + str(i);
                            newDatasetName = name.rpartition("/")[0] + "/" + datasetName;
                            self.coorDatasets[coorDatasetNames].append(str(newDatasetName));
            # if it is a group
            elif isinstance(value, h5py.Group):
                # if it is included, get latitude/longitude names from its descendant datasets
                # then recursively calls getCoordinates
                if self.subsetDataLayers.isGroupIncluded(name):
                    groupLat, groupLon = "", "";
                    allDatasetNames = value.keys();
                    for ds in allDatasetNames:
                        if ds in self.latNames: groupLat = name + "/" + ds;
                        elif ds in self.lonNames: groupLon = name + "/" + ds;
                    self.getCoordinates(value, groupLat, groupLon);

    def removeComments(self, text):
        """ remove c-style comments.
            text: blob of text with comments (can include newlines)
            returns: text with comments removed
        """
        pattern = r"""
                            ##  --------- COMMENT ---------
           /\*              ##  Start of /* ... */ comment
           [^*]*\*+         ##  Non-* followed by 1-or-more *'s
           (                ##
             [^/*][^*]*\*+  ##
           )*               ##  0-or-more things which don't start with /
                            ##    but do end with '*'
           /                ##  End of /* ... */ comment
         |                  ##  -OR-  various things which aren't comments:
           (                ## 
                            ##  ------ " ... " STRING ------
             "              ##  Start of " ... " string
             (              ##
               \\.          ##  Escaped char
             |              ##  -OR-
               [^"\\]       ##  Non "\ characters
             )*             ##
             "              ##  End of " ... " string
           |                ##  -OR-
                            ##
                            ##  ------ ' ... ' STRING ------
             '              ##  Start of ' ... ' string
             (              ##
               \\.          ##  Escaped char
             |              ##  -OR-
               [^'\\]       ##  Non '\ characters
             )*             ##
             '              ##  End of ' ... ' string
           |                ##  -OR-
                            ##
                            ##  ------ ANYTHING ELSE -------
             .              ##  Anything other char
             [^/"'\\]*      ##  Chars which doesn't start a comment, string
           )                ##    or escape
        """
        regex = re.compile(pattern, re.VERBOSE|re.MULTILINE|re.DOTALL)
        noncomments = [m.group(2) for m in regex.finditer(text) if m.group(2)]
        return "".join(noncomments)
