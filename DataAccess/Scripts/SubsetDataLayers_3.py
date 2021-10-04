#!/tools/miniconda/bin/python

import os.path
import re
import sys
import h5py

class SubsetDataLayers:
    '''
    Data structure that keeps tracks of data layers to be included in the output.
    Instance attributes:
        dataLayers: list of data layers included based on the input, if both data layer
                and its ancestor are in the input, only the ancestor is added to
                dataLayers list.
        datasets: full list of datasets matching the subsetDataLayers in a h5 file
        groups: full list of groups matching the subsetDataLayers in a h5 file
    '''

    def __init__(self, dataLayers):
        '''
        initialize a SubsetDataLayers class instance
        Args:
            dataLayers(string): list of data layers separated by comma
        '''
        self.dataLayers = [];
        if (dataLayers is not None and len(dataLayers) > 0):
            dataLayerList = dataLayers.split(",");
            for dataLayer in dataLayerList: 
                self.addDataLayer(dataLayer);
        else:
            self.addDataLayer("/");        
        self.datasets = [];
        self.groups = [];

    def addDataLayer(self, dataLayer):
        '''
        add a data layer only if its ancestor is not already included, and remove any
        its children after this data layer is added
        Args:
            dataLayer(string): the data layer to add to list
        ''' 
        # remove trailing / if dataLayer != "/" 
        if (dataLayer != "/"): dataLayer = dataLayer.rstrip('/');

        if len(self.dataLayers) == 0:
            self.dataLayers.append(dataLayer);
        else:
            #if ancestor is NOT already included, add it to dataLayers 
            if not self.__includes(dataLayer):
                self.dataLayers.append(dataLayer);
                # filter out any item which has dataLayer as its ancestor
                self.dataLayers = [item for item in self.dataLayers if item == dataLayer or not item.startswith(dataLayer.rstrip('/') + "/")];

    def __includes(self, s):
        ''' check whether the dataLayers include s, either the list contains s or has its ancestor
        Args:
            s (string): the data layer which is checked against dataLayers
        Returns:
            bool: True if the s is included in the dataLayers
        '''
        return any((s == item or s.startswith(item.rstrip('/')+ "/")) for item in self.dataLayers);

    def __includeschild(self, s):
        ''' check whether the dataLayers has s's child
        Args:
            s (string): the data layer which is checked against dataLayers
        Returns:
            bool: True if the s has child(ren) in the dataLayers list
        ''' 
        return any(item.startswith(s.rstrip('/')+"/") for item in self.dataLayers);

    def isDatasetIncluded(self, s):
        ''' check whether a dataset is included in the dataLayers
        Args:
            s (string): the dataset which is checked against dataLayers
        Returns:
            bool: True if the dataset is included based on dataLayers
        ''' 
        return self.__includes(s);

    def isGroupIncluded(self, s):
        ''' check whether a group is included in the dataLayers
        Args:
            s (string): the group which is checked against dataLayers
        Returns:
            bool: True if the group is included based on dataLayers
        '''
        return self.__includes(s) or self.__includeschild(s);

    def expand(self, inputfile):
        ''' expand the dataLayers for a given h5 file, 
        the results will be saved to the datasets and groups lists
        Args:
            inputfile(string): input h5 file     
        '''
        # clear the datasets and groups to load new list for the given file
        self.datasets[:] = [];
        self.groups[:] = [];
        f = h5py.File(inputfile,'r');
        if (self.isGroupIncluded(f.name)): self.groups.append(f.name);
        self.expandGroup(f);

    def expandGroup(self, g):
        ''' expand a group based on the dataLayers,
        the results will be added to the datasets and groups lists
        Args:
            g (h5py.Group): group to expand based on dataLayers
        '''
        for key, val in g.items():
            name = val.name;
            if isinstance(val, h5py.Dataset):                
                if (self.isDatasetIncluded(name)):
                    self.datasets.append(name);
            elif isinstance(val, h5py.Group):
                if (self.isGroupIncluded(name)):
                    self.groups.append(name);
                    self.expandGroup(val);

    def getDatasets(self, inputfile):
        ''' get the list of datasets matching the dataLayers in a file,
        expand the dataLayers it's not done already
        Args:
            inputfile(string): input h5 file
        ''' 
        if (len(self.groups)+len(self.datasets) == 0):
            self.expand(inputfile);
        return self.datasets;

    def getGroups(self, inputfile):
        ''' get the list of groups matching the dataLayers in a file
        expand the dataLayers it's not done already
        Args:
            inputfile(string): input h5 file
        ''' 
        if (len(self.groups)+len(self.datasets) == 0):
            self.expand(inputfile);
        return self.groups;

    def addDimensionScaleDatasets(self, inputfile):
        ''' for datasets in the dataLayers, add their dimension scale datasets to the
        dataLayers if these scale datasets are not already included
        Args:
            inputfile(string): input h5 file 
        Returns:
            list: datasets added to data layers
        '''
        datasetsAdded = [];
        # expand the subset data layer to get list of datasets
        self.getDatasets(inputfile);

        # read through each dataset, and get the dimension scales it attaches to,
        # add to the dataLayers and datasets list
        f = h5py.File(inputfile,'r');
        for dn in self.datasets:
            ds = f[dn];
            #print "dataset", ds.name; 
            for i in range(0, len(ds.dims)):
                for ss in ds.dims[i].values():
                    #print "dimension", i,ss.name;
                    if not self.__includes(ss.name):
                        self.dataLayers.append(ss.name);
                        datasetsAdded.append(ss.name);
        self.datasets.extend(datasetsAdded);
        return datasetsAdded;

    def addCoordinateDatasets(self, inputfile, includedDatasets = None, excludedDatasets = None):
        ''' for datasets in the dataLayers, add their coordinate datasets to the
        dataLayers if these scale datasets are not already included.
        The filters will be applied when specified. If includedDatasets is specified,
        only datasets listed are included. If excludedDatasets is specified, the
        datasets listed are excluded.
        Args:
            inputfile(string): input h5 file
            includedDatasets(list): if not None, includes only the datasets (names) listed.
            excludedDatasets(list): if not None, excludes the datasets (names) listed.
        Returns:
            list: datasets added to data layers
        '''
        datasetsAdded = [];
        # expand the subset data layer to get list of datasets
        self.getDatasets(inputfile);

        # read through each dataset, and get the coordinate datasets based on the attribute,
        # add to the dataLayers and datasets list
        f = h5py.File(inputfile,'r');
        for dn in self.datasets:
            ds = f[dn];
            dsname = ds.name;
            grppath = os.path.dirname(ds.name);
            #print "dataset", ds.name;
            # coordinates datasets list
            attrVals = ds.attrs.get("coordinates");
            if attrVals is not None and isinstance(attrVals, bytes): attrVals = attrVals.decode('utf-8');
            attrValList = re.split(r'[,\s]+',attrVals) if attrVals is not None else [];
            for attrVal in attrValList:
                # coordinate dataset absolute path
                coordinateds = os.path.abspath(os.path.join(grppath,attrVal));
                #print "coordinates", attrVal, coordinateds;
                if (includedDatasets is not None and os.path.basename(coordinateds) not in includedDatasets):
                    continue;
                if (excludedDatasets is not None and os.path.basename(coordinateds) in excludedDatasets):
                    continue;
                if not self.__includes(coordinateds):
                    self.dataLayers.append(coordinateds);
                    datasetsAdded.append(coordinateds);
        self.datasets.extend(datasetsAdded);
        return datasetsAdded;

    def addCorrespondingCoorDatasets(self, latNames, lonNames, timeNames):
        ''' for latitude/longitude datasets in dataLayers, add its corresponding latitude/
            longitude datasets to dataLayers if not already exist
        Args:
            latNames(list): latitude dataset name candidates retrieved from config file
            lonNames(list): longitude dataset name candidates retrieved from config file
        Returns:
            list: datasets added to data layers
        '''
        datasetsAdded = [];

        # loop through the datasets list
        for ds in self.datasets:
            dn = ds.rpartition("/")[2];
            grouppath = os.path.dirname(ds);
            if dn in timeNames:
                for lat in latNames:
                    coordinateDs =  os.path.abspath(os.path.join(grouppath, lat));
                    if not self.__includes(coordinateDs):
                        self.dataLayers.append(coordinateDs);
                        datasetsAdded.append(coordinateDs);
                for lon in lonNames:
                    coordinateDs =  os.path.abspath(os.path.join(grouppath, lon));
                    if not self.__includes(coordinateDs):
                        self.dataLayers.append(coordinateDs);
                        datasetsAdded.append(coordinateDs);
            if dn in latNames:
                i = latNames.index(dn);
                coordinateDn = lonNames[i];
                coordinateDs = os.path.abspath(os.path.join(grouppath, coordinateDn));
                if not self.__includes(coordinateDs):
                    self.dataLayers.append(coordinateDs);
                    datasetsAdded.append(coordinateDs);
            if dn in lonNames:
                i = lonNames.index(dn);
                coordinateDn = latNames[i];
                coordinateDs = os.path.abspath(os.path.join(grouppath, coordinateDn));
                if not self.__includes(coordinateDs):
                    self.dataLayers.append(coordinateDs);
                    datasetsAdded.append(coordinateDs);
        self.datasets.extend(datasetsAdded);
        return datasetsAdded

    def addRequiredDatasets(self, inputfile, required):
        ''' add required datasets
        Args:
            inputfile(string): inputfile path
            required(dictionary): required datasets from configuration file
        Return:
            list: datasets to be added to the data layers
        '''
        requiredDatasets = [];

        groups = self.getGroups(inputfile);
        
        for i, (shortname, val) in enumerate(required.items()):
            for i, (groupPattern, value) in enumerate(val.items()):
                for group in groups:
                    if re.match(groupPattern, group) is not None:
                        requiredDatasets.append(group+"/"+value);
                        self.dataLayers.append(group+"/"+value);
        self.datasets.extend(requiredDatasets);
        return requiredDatasets

        

# test program for the class
# Usage: SubsetDataLayers.py <input H5 file> <comma separated subset data layers>
# e.g. SubsetDataLayers.py processed_SMAP_L2_SM_P_09881_A_20161207T035804_R14010_002.h5 
#      "/Soil_Moisture_Retrieval_Data/tb_time_utc,/Soil_Moisture_Retrieval_Data,/grouptoo/"
if __name__ == '__main__':
    # retrieve command line parameters and save to global variables
    (scriptName, inputFile, subsetDataLayers) = sys.argv[0:3];
    dataLayers = SubsetDataLayers(subsetDataLayers);
    print ("dataLayers added: ", dataLayers.dataLayers);
    print ("expanding data layer");
    dataLayers.expand(inputFile);
    datasets = dataLayers.getDatasets(inputFile);
    print ("datasets included: ", datasets);
    groups = dataLayers.getGroups(inputFile);
    print ("groups included: ", groups);

    # test adding dimension scales to data layers
    dataLayers = SubsetDataLayers(subsetDataLayers);
    datasetsAdded = dataLayers.addDimensionScaleDatasets(inputFile);
    print ("dataLayers.addDimensionScaleDatasets added: ", datasetsAdded);
    print ("new dataLayers: ", ",".join(dataLayers.dataLayers));

    # test adding coordinate datasets to data layers
    dataLayers = SubsetDataLayers(subsetDataLayers);
    includedDatasets = [];
    includedDatasets.append("latitude");
    includedDatasets.append("longitude");
    excludedDatasets = [];
    excludedDatasets.append("delta_time");
    # includedDatasets and/or includedDatasets can set to None
    # datasetsAdded = dataLayers.addCoordinateDatasets(inputFile, None, excludedDatasets);
    # datasetsAdded = dataLayers.addCoordinateDatasets(inputFile);
    datasetsAdded = dataLayers.addCoordinateDatasets(inputFile, includedDatasets, excludedDatasets);
    print ("dataLayers.addCoordinateDatasets added: ", datasetsAdded);
    print ("updated dataLayers: ", ",".join(dataLayers.dataLayers));

    required = {"/BEAM[\\d]+$": "shot_number"}

    dataLayers.addRequiredDatasets(inputFile, required)

