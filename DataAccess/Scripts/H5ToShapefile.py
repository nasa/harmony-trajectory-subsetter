#!/tools/miniconda/bin/python

import sys
import os
import shapefile
import numpy as np
import h5py
import collections
import time
import tarfile

from ParameterHandling_3 import ParameterHandling
from SubsetDataLayers_3 import SubsetDataLayers
from Coordinate import Coordinate

''' writes field name in shapefile to dataset name in HDF5 file mapping to a readme file
Args:
    readMeFile(string): readMe filename
    inputfile(string): input file name
    fieldNames(OrderedDictionary): key - fieldname
                                   value - dataset name
'''
def writeReadMe(readMeFile,inputfile,fieldNames):
    f = open(readMeFile, 'w');
    f.write("-------------------------------------------------------------------------------------------------\n");
    f.write("This file contains mappings from field names in the shapefile to dataset names in the HDF5 file.\n");
    f.write("HDF5 file: ");
    f.write(os.path.basename(inputfile));
    f.write("\n-------------------------------------------------------------------------------------------------\n");
    for i,(key,value) in enumerate(fieldNames.items()): f.write(key + ": " + value + "\n");

''' construct an unique fieldname for each datset
    Shapefile automatically truncates field name after 11 characters, construct a fieldname 
    that is 11 characters long and has proper index appended to them to distinguish datasets
    among different groups for absolute path of the datasets refer to the readMe file that is 
    generated along with the shapefile
Args:
    allDatasetNames(list): subset data layers list
    coorDatasets(OrderedDictionary): key - coordinate dataset names
                                     value - list of dataset names that are referencing to the 
                                             coordinate dataset in key
    inf(h5 object): input
Return:
    revisedFieldName(list): list of field names
    fieldNames(dict): key - field names
                      value - full path of the dataset
    datasets(dict): key - dataset names
                    value - dataset references
    
'''
def getFieldNames(allDatasetNames, coorDatasets, inf):
    revisedFieldNames = [];
    fieldNames = collections.OrderedDict();
    InUseFieldNames = collections.OrderedDict();
    datasets = collections.OrderedDict();
    index = "";

    for i, datasetName in enumerate(allDatasetNames):
        if any(datasetName in s for s in coorDatasets.values()):
            # reverse the dataset name and group name path
            # e.g., /group1/group2/dataset would show up as dataset/group2/group1/
            # in shapefile
            fieldName = datasetName.split("/");
            fieldName.reverse();
            fieldName = "/".join(fieldName);

            # construct field names for two-dimensional datasets
            # e.g., no occurence of "abcdefghijk" found in previous datasets: abcdefghi_#, 
            #       occurence of "abcdefghijk" was found in previous datasets: abcdefg_#_#
            if inf[datasetName].ndim > 1:
                multiDatasets = [];
                # take first 11 characters of the dataset name
                fieldName = fieldName[:11];
                # if the first 11 characters match any of the former datasets, append an index to it
                if fieldName in fieldNames.keys():
                    if fieldName not in InUseFieldNames.keys(): InUseFieldNames[fieldName] = 1;
                    else: InUseFieldNames[fieldName] = InUseFieldNames[fieldName]+1;
                    index = "_" + str(InUseFieldNames[fieldName]);

                for ds in coorDatasets.values():
                    for dataset in ds:
                        if dataset.startswith(datasetName):
                            if (dataset.rpartition("_")[2].isdigit() or dataset == datasetName):
                                multiDatasets.append(dataset);

                # append the index of 2nd dimension to the field name,
                # adjust if the length exceeds 11 characters
                # e.g., abcdefg_1_10 (12 characters) to abcdef_1_10 (11 characters)
                for i, d in enumerate(multiDatasets):
                    datasets[d] = inf[datasetName][:,i];
                    multiDimIndex = index + "_" + str(i);
                    multiDimFieldName = fieldName[0:11-len(multiDimIndex)] + multiDimIndex;
                    fieldNames[multiDimFieldName] = d;
                    revisedFieldNames.append(multiDimFieldName);

            # one-dimensional datasets, if the dataset name matches any of the former datasets
            # append an index to it
            else:
                datasets[datasetName] = inf[datasetName][()];
                # get first 11 characters of the dataset name
                fieldName = fieldName[:11]; 
                # if it matches any of the former field names, add the index to it, "_#"
                if fieldName in fieldNames.keys():
                    if fieldName not in InUseFieldNames.keys(): InUseFieldNames[fieldName] = 1;
                    else: InUseFieldNames[fieldName] = InUseFieldNames[fieldName]+1;
                    index = "_" + str(InUseFieldNames[fieldName]);
                    fieldName = fieldName[0:11-len(index)] + index;
                fieldNames[fieldName] = datasetName;
                revisedFieldNames.append(fieldName);

    return revisedFieldNames, fieldNames, datasets;

''' store dataset values in an ordered dictionary
Args:
    coorDatasets(OrderedDictionary): key - coordinate dataset names
                                     value - list of dataset names that are referencing to the 
                                             coordinate dataset in key
    inf(h5 object): input
    datasets(OrderedDictionary): key - dataset name
                                 value - dataset value
Returns:
    allValues(numpy array): stores all dataset values
    finalLatitude(numpy array): stores all latitude values
    finalLongitude(numpy array): stores all longitude values
'''
def storeDatasetValue(coorDatasets, inf, datasets):

    finalLatitude = np.empty(0);
    finalLongitude = np.empty(0);
    numOfDatasets = len(datasets);
    allValues = np.empty(0);
    oneRecord = np.empty(0);

    # get latitude and longitude datasets, join all latitudes into one, and
    # and join all longitudes into one
    for i, (key, value) in enumerate(coorDatasets.items()):
        coordinates = key.split(", ");
        for coordinate in coordinates:
            if "lat" in coordinate:
                if not finalLatitude.size: finalLatitude = inf[coordinate][()];
                else:
                    latitude = inf[coordinate][()];
                    finalLatitude = np.concatenate([finalLatitude, latitude]);
            elif "lon" in coordinate:
                if not finalLongitude.size: finalLongitude = inf[coordinate][()];
                else:
                    longitude = inf[coordinate][()];
                    finalLongitude = np.concatenate([finalLongitude, longitude]);

    # total dataset length of dataset from each coordinate reference that have already been added to the array 
    totalDatasetSize = 0;

    # loop datasets by coordinate references
    # construct a numpy array that contains all dataset values
    # add np.nan as placeholders for datasets that have different coordinate reference
    # e.g., [[datasetValue0, np.nan, np.nan], [np.nan, datasetValue1, np.nan], [np.nan, np.nan, datasetValue2]]
    for i, (key, value) in enumerate(coorDatasets.items()):
        for j, (key2, value2) in enumerate(datasets.items()):
            if key2 in value:
                dataset = datasets[key2];
                dataTypeSize = len(dataset);
                if len(dataset) != len(finalLatitude):
                    # datasets that references the first coordinate reference
                    # append np.nan to the array
                    if i == 0:
                        fillArraySize = len(finalLatitude) - len(dataset);
                        fillArray = np.empty(fillArraySize);
                        fillArray.fill(np.nan);
                        dataset = np.concatenate([dataset, fillArray]);
                    # datasets that references the last coordinate reference
                    # prepend np.nan to the array
                    elif i == (len(coorDatasets) - 1):
                        fillArraySize = len(finalLatitude) - len(dataset);
                        fillArray = np.empty(fillArraySize);
                        fillArray.fill(np.nan);
                        dataset = np.concatenate([fillArray, dataset]);
                    # datasets that references middle coordinate references
                    # append and prepend np.nan
                    else:
                        fillArray = np.empty(totalDatasetSize);
                        fillArray.fill(np.nan);
                        dataset = np.concatenate([fillArray, dataset]);
                        fillArraySize = len(finalLatitude) - len(dataset);
                        fillArray = np.empty(fillArraySize);
                        fillArray.fill(np.nan);
                        dataset = np.concatenate([dataset, fillArray]);

                if not allValues.size:
                    allValues = dataset;
                else:
                    allValues = np.vstack((allValues, dataset));

        totalDatasetSize += dataTypeSize;

    # add latitude, longitude values to the array
    # e.g., [[datasetValue0, np.nan, np.nan], [np.nan, datasetValue1, np.nan], 
    #        [np.nan, np.nan, datasetValue2], finalLongitude, finalLatitude]
    allValues = np.vstack((allValues, finalLongitude));
    allValues = np.vstack((allValues, finalLatitude));

    # transpose the array, now the array would have the following:
    #   [[datasetValue0, np.nan, np.nan, finalLongitude0, finalLatitude0], 
    #    [np.nan, datasetValue1, np.nan, finalLongitude1, finalLatitude1], 
    #    [np.nan, np.nan, datasetValue2, finalLongitude2, finalLatitude2]]
    allValues = np.transpose(allValues);

    return allValues, finalLatitude, finalLongitude;

'''read dataset values from inputfile
Argsi:
    allDatasetNames(list): list of dataset names
    coorDatasets(dictionary): key - coordinate dataset names
                              value - list of dataset names that are referencing to the 
                                      coordinate dataset in key
    inf(h5 object): input
Returns:
    datasets(dictionary): key - dataset names
                          value - dataset value
'''
def readDatasets(allDatasetNames, coorDatasets, inf):

    datasets = collections.OrderedDict();

    for index, datasetName in enumerate(allDatasetNames):
        if any(datasetName in s for s in coorDatasets.values()):
            datasets[datasetName] = inf[datasetName][()];

    return datasets;

''' get the size of the dataset from its datatype
Args:
    dataset(H5.Dataset)
Returns:
    dataset size
'''
def getDataTypeSize(dataset):

    # get data type
    dataType = dataset.dtype;

    if dataType == "int8": return 6;
    elif dataType == "int16": return 8;
    elif dataType == "int32": return 12;
    elif dataType == "int64": return 22;
    else: return 50;

''' get the number of decimal places from its datatype
Args:
    dataset(H5.Dataset)
Returns:
    number of decimal places
'''
def getDecimalPlaces(dataset):

    dataType = dataset.dtype;

    if dataType == "float64": return 16;
    if dataType == "float32": return 7;
    return 0;

''' writes everything to one shapefile
Args:
    parameters(ParameterHandling): command line parameters
''' 
def allInOneShapefile(parameters, startTime):

    # argument processing
    inputfile = parameters.getInputfile();
    outputdir = parameters.getOutputdir();
    shapefileDir = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0];
    outputfile = shapefileDir + "/" + os.path.splitext(os.path.basename(inputfile))[0];
    readMeFile = shapefileDir + "/ReadMe.txt";

    subsetDataLayers = parameters.getSubsetDataLayers();
    subsetDataLayers = SubsetDataLayers(subsetDataLayers);
    datasetNames = subsetDataLayers.dataLayers;
    subsetDataLayers.expand(inputfile);
    allDatasetNames = subsetDataLayers.datasets;
    
    # get coordinates information
    coor = Coordinate(inputfile, subsetDataLayers);

    # if no coordinate references found, return
    if not coor.coorDatasets.keys(): sys.exit(4);
  
    # if number of fields exceeds maximum, return
    count = 0;
    for i, (key, value) in enumerate(coor.coorDatasets.items()):
        count += len(value);

    if (count > 2046): sys.exit(5);

    if not os.path.exists(shapefileDir):
        os.makedirs(shapefileDir);

    inf = h5py.File(inputfile, 'r');

    (allFieldNames, fieldNames, datasets) = getFieldNames(allDatasetNames, coor.coorDatasets, inf);
    writeReadMe(readMeFile, inputfile, fieldNames);
    targetfile=inputfile
    shp_writer = shapefile.Writer(targetfile);
    # create fields
    for i, (key, value) in enumerate(fieldNames.items()):
        dataTypeSize = getDataTypeSize(datasets[value]);
        decPlaces = getDecimalPlaces(datasets[value]);
        shp_writer.field(str(key), fieldType='N', size=dataTypeSize, decimal=decPlaces);

    (allValues, latitude, longitude) = storeDatasetValue(coor.coorDatasets, inf, datasets);

    lonDataTypeSize=getDataTypeSize(longitude);
    latDataTypeSize=getDataTypeSize(latitude);
    lonDecPlaces = getDecimalPlaces(longitude);
    latDecPlaces = getDecimalPlaces(latitude);
    shp_writer.field("longitude", fieldType='N', size=lonDataTypeSize, decimal=lonDecPlaces);
    shp_writer.field("latitude", fieldType='N', size=latDataTypeSize, decimal=latDecPlaces);

    # write shapefile
    for i in range(len(latitude)):
        shp_writer.point(longitude[i], latitude[i]);
        record = allValues[i];
        shp_writer.record(*record);

''' creates one shapefile for each dataset
Args:
    parameters(ParameterHandling): command line parameters
'''
def oneDatasetPerShapefile(parameters, startTime):

    # argument processing
    inputfile = parameters.getInputfile();
    outputdir = parameters.getOutputdir();
    shapefileDir = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0];

    subsetDataLayers = parameters.getSubsetDataLayers();
    subsetDataLayers = SubsetDataLayers(subsetDataLayers);
    datasetNames = subsetDataLayers.dataLayers;
    subsetDataLayers.expand(inputfile);
    allDatasetNames = subsetDataLayers.datasets;
    coor = Coordinate(inputfile, subsetDataLayers);

    # if no coordinate references found, return
    if not coor.coorDatasets.keys(): sys.exit(4);

    if not os.path.exists(shapefileDir):
        os.makedirs(shapefileDir);

    inf = h5py.File(inputfile, 'r');

    # read all datasets into 'datasets'(dictionary, key(dataset names), value(dataset))
    datasets = readDatasets(allDatasetNames, coor.coorDatasets, inf);

    # iterate through coorDatasets(dictionary: key(coordinate dataset names), value(list of dataset
    # names that are referencing to these coordiante datasets
    for i, (key, value) in enumerate(coor.coorDatasets.items()):
        # get lat/lon dataset names
        coordinates = key.split(", ");
        for coordinate in coordinates:
            if "lat" in coordinate: latitude = inf[coordinate][()];
            elif "lon" in coordinate: longitude = inf[coordinate][()];

        # loop through list of datasets that are referencing to coordinate datasets in "key"
        for ds in value:
            if ds in allDatasetNames:
                allValues = np.empty(0);
                oneRecord = np.empty(0);
                dataset = datasets[ds];
                targetfile = shapefileDir + "/"  + ds[1:].replace("/", "_");
                shp_writer = shapefile.Writer(targetfile);
                shp_writer.shapeType = 1;
               # shp_writer = shapefile.Writer(shapeType=1);
               # shp_writer = shapefile.Writer(shapeType=3);
                # multi-dimensional datasets
                if dataset.ndim > 1:
                    multiDatasets = collections.OrderedDict();
                    for i in range(dataset.shape[1]):
                        if not allValues.size: allValues = dataset[:,i];
                        else: allValues = np.vstack((allValues, dataset[:,i]));
                        dataTypeSize = getDataTypeSize(dataset[:,i]);
                        decPlaces = getDecimalPlaces(dataset[:,i]);
                        shp_writer.field(str(i), fieldType='N',size=dataTypeSize, decimal=decPlaces);
                # one-dimensional datasets
                else:
                    dataTypeSize = getDataTypeSize(dataset);
                    decPlaces = getDecimalPlaces(dataset);
                    shp_writer.field(str(ds.rpartition("/")[2]), fieldType='N', size=dataTypeSize, decimal=decPlaces);
                    allValues = dataset;

                # add longitude and latitude to the array and transpose the array                 
                allValues = np.vstack((allValues, longitude));
                allValues = np.vstack((allValues, latitude));
                allValues = np.transpose(allValues);

                lonDataTypeSize = getDataTypeSize(longitude);
                latDataTypeSize = getDataTypeSize(latitude);
                lonDecPlaces = getDecimalPlaces(longitude);
                latDecPlaces = getDecimalPlaces(latitude);
                shp_writer.field("longitude", fieldType='N', size=lonDataTypeSize, decimal=lonDecPlaces);
                shp_writer.field("latitude", fieldType='N', size=latDataTypeSize, decimal=latDecPlaces);

                for i in range(len(dataset)):
                    shp_writer.point(longitude[i], latitude[i]);
                    record = allValues[i];
                    shp_writer.record(*record);


# # # # # # # # # # # # # # # # # # # # # # # # # # # # 

#  main work flow

# # # # # # # # # # # # # # # # # # # # # # # # # # # # 

startTime = time.time();

parameters = ParameterHandling("HDF5 to Shapefile converter");
outputdir = parameters.getOutputdir();
inputfile = parameters.getInputfile();
format = parameters.getFormat();
shapefileDir = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0];

if format == "Shapefile-Combined": 
    allInOneShapefile(parameters,startTime);
else:
    oneDatasetPerShapefile(parameters,startTime);

with tarfile.open(shapefileDir+".tar.gz", "w:gz") as tar:
    tar.add(shapefileDir, arcname=os.path.basename(shapefileDir));

files = os.listdir(shapefileDir);

for file in files:
    if not file.endswith(".tar.gz"): os.remove(os.path.join(shapefileDir, file));

if not os.listdir(shapefileDir): os.rmdir(shapefileDir);

print("execution time:", time.time() - startTime);
