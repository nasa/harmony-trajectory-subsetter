#!/tools/miniconda/bin/python
import os
import sys
import subprocess
import json
import re
import h5py
from datetime import datetime
from Logging_3 import Log
from ParameterHandling_3 import ParameterHandling
from SubsetDataLayers_3 import SubsetDataLayers

# Tool adapter for ICESAT2 data access service

# # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Functions
pars = ""
hasUnsubsettables = False

def executeCommand(cmd):
    '''Function to execute an external command
    Args:
        cmd(string): command to execute
    Returns:
        command exit code int
        command standard output string
        command standard error string
    '''
    notsubsStr = "/ is not subsettable \n"
    global pars
    global hasUnsubsettables

    # invokes the command constructed above
    process = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE);
    while True:
        # Looping messages from subset
        par = "";
        line = process.stdout.readline().decode('UTF-8')
        if not line:
            break;
        if line.find(notsubsStr, 16) != -1:
            par = re.sub(notsubsStr, "", line);
            if par not in pars: 
                pars += par + ', ';
            hasUnsubsettables = True
            
    # assigns the stdout stderr from above command to variables
    outstr, errorstr = process.communicate();
    
    # wait for process the terminate, and get the return code
    exitstatus = process.poll();
    return exitstatus, str(outstr), str(errorstr), str(pars);

def readConfigFile():
    ''' Function to read the configuration file
    Returns:
        json object that contains config file information
    '''
    currentDirectory = sys.path[0];
    mode = currentDirectory.split('/')[3];
    cfgfile = "/usr/ecs/" + mode + "/CUSTOM/data/DPL/SubsetterConfig.json";
    configString = open(cfgfile).read();
    configStringWoComments = removeComments(configString);
    config = json.loads(configStringWoComments);

    return config;

def subset(parameters,config):
    '''Perform subset
    Args:
        parameters(ParameterHandling): command line parameters
    Returns:
        command exit code int
        command executed string 
        inputfile input file for subsetting string
        outputfile output file string
        errorCode error code of operation string 
        errorMsg error message of the command execution string
    '''
    inputfile = parameters.getInputfile();
    identifier = parameters.getIdentifier();
    outputdir = parameters.getOutputdir() + "/" + identifier;
    outputfile = outputdir + "/processed_" + os.path.basename(inputfile);
    if not os.path.exists(outputdir):
        os.makedirs(outputdir);
    format = parameters.getFormat();

    currentDirectory = sys.path[0];
    mode = currentDirectory.split('/')[3];
    scriptpath = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/subset";
    cfgfile = "/usr/ecs/" + mode + "/CUSTOM/data/DPL/SubsetterConfig.json";

    cmd = scriptpath + " --configfile " + cfgfile + \
          " --filename " + inputfile + " --outfile " + outputfile;

    subsetDataLayers = parameters.getSubsetDataLayers();
    if subsetDataLayers is not None:
        dataLayers = SubsetDataLayers(subsetDataLayers);
        requiredDatasets = config["RequiredDatasets"]
        dataLayers.addRequiredDatasets(inputfile, requiredDatasets)
        # If the format is NetCDF or Shapefile, additional datasets have 
        # to be added to subset data layers.
        if format in netcdfFormats: 
            dataLayers.addDimensionScaleDatasets(inputfile);
        elif format in shapefileFormats: 
            #configString = open(cfgfile).read();
            #configStringWOComment = removeComments(configString);
            #config = json.loads(configStringWOComment);
            #config = readConfigFile();
            coordinateDatasetNames = config["CoordinateDatasetNames"];
            latNames = []
            lonNames = []
            timeNames = []
            for i, (key, value) in enumerate(coordinateDatasetNames.items()):
                latNames += coordinateDatasetNames[key]["latitude"];
                lonNames += coordinateDatasetNames[key]["longitude"];
                if "time" in coordinateDatasetNames[key]:
                    timeNames += coordinateDatasetNames[key]["time"];
            dataLayers.addCoordinateDatasets(inputfile);
            dataLayers.addCorrespondingCoorDatasets(latNames, lonNames, timeNames);
        # use the updated data layers
        cmd += " --includedataset " + ",".join(dataLayers.dataLayers);

    bbox = parameters.getBbox();
    if bbox is not None: cmd += " --bbox " + bbox;

    startTime = parameters.getStart();
    if startTime is not None: cmd += " --start " + startTime;

    endTime = parameters.getEnd();
    if endTime is not None:
        cmd += " --end " + endTime;
    elif startTime is not None:
        # use current time
        cmd += " --end " + datetime.utcnow().strftime("%Y-%m-%dT%T");

    boundingShape = parameters.getBoundingShape();
    if boundingShape is not None: cmd += " --boundingshape " + boundingShape;

    debug = parameters.getDebug();
    log = Log(debug,outputdir);
    log.writeCommand(cmd);
    exitstatus, stdout, stderr, pars = executeCommand(cmd);
    log.writeStdOutAndStdErr(stdout, stderr);

    # if failed, log last 10 lines of standard out to rqs log
    if exitstatus != 0 and debug != 'Y':
        sys.stderr.write(stderr);
        sys.stderr.write(";".join(stdout.split('\n')[-10:]));
   
    # if failed, return error code and message
    # exitstatus = 1 is reserved for invalid parameter error case
    # exitstatus = 2 is reserved for missing parameter error case
    # exitstatus = 3 is reserved for no matching data error case
    # all other exitstatus goes to internal error
    errorCode, errorMsg = None, None;
    if exitstatus != 0 or not os.path.isfile(outputfile):
        errorCode = "InternalError";
        errorMsg = "An internal error occured.";

        if exitstatus == 1:
            errorCode = "InvalidParameter";
            errorMsg = "Incorrect parameter specified for given dataset(s).";
        elif exitstatus == 2:
            errorCode = "MissingParameter";
            errorMsg = "No parameter value(s) specified for given dataset(s).";
        elif exitstatus == 3:
            errorCode = "NoMatchingData";
            errorMsg = "No data found that matched subset constraints.";
        elif exitstatus == 6:
            errorCode = "NoPolygonFound";
            errorMsg  = "No polygon found in the given GeoJSON/KML/Shapefile.";
        
    return exitstatus, cmd, inputfile, outputfile, errorCode, errorMsg;

def convertToNetCDF(parameters, inputfile):
    '''convert HDF5 file to NetCDF
    Args:
        parameters(ParameterHandling): command line parameters
        inputfile(string): input H5 file
    Returns:
        command exit code int
        command executed string
        inputfile input H5 file string
        outputfile output file string
        errorCode error code of operation string 
        errorMsg error message of the command execution string
    '''
    identifier = parameters.getIdentifier();
    outputdir = parameters.getOutputdir() + "/" + identifier;
    outputfile = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0] + ".nc";
    if not os.path.exists(outputdir):
        os.makedirs(outputdir);
    format = parameters.getFormat();
    cmd, exitstatus, errorCode, errorMsg = None, 0, None, None;
    
    # if the format is NetCDF-3, then estimate the output NetCDF-3 size.
    # if the estimated size is exceeding 2GB, then return error
    if (format in ["NetCDF3", "NetCDF-3"]):
        (fileSize, uncompressedSize, numOfGroups, numOfDatasets) = getHDF5FileInfo(inputfile);
        if float(uncompressedSize)/1024/1024/1024 >= 2:
            exitstatus = -1;
            errorCode = "ExceedsFileSizeLimit";
            errorMsg = "Estimated NetCDF-3 size is " + "{:.2f}".format(uncompressedSize/1024/1024/1024) + \
                       "GB which exceeds the 2GB size limit of the NetCDF-3 format. " + \
                       "Please select fewer parameters and/or a smaller subset area.";

    # continue to convert data if no errors
    if (exitstatus == 0):
        currentDirectory = sys.path[0];
        mode = currentDirectory.split('/')[3];
        scriptpath = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/H5ToNetCDFConverter";
        cmd = scriptpath + " " + format + " " + inputfile + " " + outputfile;
        debug = parameters.getDebug();
        log = Log(debug,outputdir);
        log.writeCommand(cmd);
        exitstatus, stdout, stderr, pars = executeCommand(cmd);    
        log.writeStdOutAndStdErr(stdout, stderr);

        # if failed, log last 10 lines of standard out to rqs log
        if exitstatus != 0 and debug != 'Y':
            sys.stderr.write(stderr);
            sys.stderr.write(";".join(stdout.split('\n')[-10:]));
    
    # if failed, return error code and message
    if exitstatus != 0 or not os.path.isfile(outputfile):
        # -62 NetCDF3 error for exceeding 2GB file size limit
        netcdf3SizeError = -62;
        if (exitstatus-256 == netcdf3SizeError):
            exitstatus = netcdf3SizeError;
            errorCode = "ExceedsFileSizeLimit";
            errorMsg = "Exceeds the 2GB size limit of the NetCDF-3 format. " + \
                       "Please select fewer parameters and/or a smaller subset area.";
        elif errorCode is None and errorMsg is None:
            errorCode = "InternalError";
            errorMsg = "H5ToNetCDFConverter failed";

    return exitstatus, cmd, inputfile, outputfile, errorCode, errorMsg;

    
def convertToShapefile(parameters, inputfile):
    '''convert HDF5 file to Shapefile
    Args:
        parameters(ParameterHandling): command line parameters
        inputfile(string): input file
    Returns:
        command exit code int
        command executed string
        inputfile input H5 file string
        outputfile output file string
        errorCode error code of operation string 
        errorMsg error message of the command execution string
    '''    
    identifier = parameters.getIdentifier();
    outputdir = parameters.getOutputdir() + "/" + identifier;
    outputfile = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0] + ".tar.gz";
    if not os.path.exists(outputdir):
        os.makedirs(outputdir);
    subsetDataLayers = parameters.getSubsetDataLayers();
    format = parameters.getFormat();

    currentDirectory = sys.path[0];
    mode = currentDirectory.split('/')[3];
    scriptpath = "/usr/ecs/" + mode + "/CUSTOM/utilities/H5ToShapefile.py";
    cmd = scriptpath + " --FILE_URLS " + inputfile + " --OUTPUT_DIR " + outputdir + " --FORMAT " + format;
    if subsetDataLayers is not None:
          cmd += " --SUBSET_DATA_LAYERS " + subsetDataLayers;
    debug = parameters.getDebug();
    log = Log(debug,outputdir);
    log.writeCommand(cmd);
    exitstatus, stdout, stderr, pars = executeCommand(cmd); 
    log.writeStdOutAndStdErr(stdout, stderr);

    # if failed, log last 10 lines of standard out to rqs log
    if exitstatus != 0 and debug != 'Y':
        sys.stderr.write(";".join(stdout.split('\n')[-10:]));
    
    # if failed, return error code and message
    errorCode, errorMsg = None, None;
    if exitstatus != 0 or not os.path.isfile(outputfile):
        if exitstatus == 4:
            errorCode = "No output generated";
            errorMsg = "No coordinate references found for selected area";
        elif exitstatus == 5:
            errorCode = "NumberOfFieldsOutOfRange";
            errorMsg = "Number of fields exceeded 2046(max fields allowed)";
        else:
            errorCode = "InternalError";
            errorMsg = "H5ToShapefile.py failed";

    return exitstatus, cmd, inputfile, outputfile, errorCode, errorMsg;

def removeComments(text):
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

def getHDF5FileInfo(inputfile):
    '''get HDF5 file information
    Args:
        inputfile(string): input h5 file
    Returns:
        fileSize file size int
        uncompressedSize uncompressed size based on calculated dataset memory size int,
                         this doesn't include the attributes and other memory usages
        numOfGroups number of groups int
        numOfDatasets number of datasets int
    '''
    fileSize = os.path.getsize(inputfile);
    inf = h5py.File(inputfile,'r');
    (uncompressedSize, numOfGroups, numOfDatasets) = getGroupDatasetSizeInfo(inf);
    return fileSize, uncompressedSize, numOfGroups, numOfDatasets;

def getGroupDatasetSizeInfo(g):
    '''get group and dataset size information of a given HDF5 group
    Args:
        g(Group): input group
    Returns:
        size uncompressed size of the group based on calculated dataset memory size int,
                         this doesn't include the attributes and other memory usages
        numOfGroups number of groups in the group int
        numOfDatasets number of datasets in the group int
    '''
    (size, numOfGroups, numOfDatasets) = (0,0,0);
    for key, val in g.items():
       name = val.name;
       if isinstance(val, h5py.Dataset):
           size += val.size * val.dtype.itemsize;
           numOfDatasets+=1;
       elif isinstance(val, h5py.Group):
           numOfGroups += 1;
           (rsize, rnumOfGroups, rnumOfDatasets) = getGroupDatasetSizeInfo(val);
           size += rsize;
           numOfGroups += rnumOfGroups;
           numOfDatasets += rnumOfDatasets;
    return size, numOfGroups, numOfDatasets;

# # # # # # # # # # # # # # # # # # # # # # # # # # # #
# main work flow

parameters = ParameterHandling("ICESat-2 Tool Adapter");
format = parameters.getFormat();
originalInputfile = parameters.getInputfile();
# commands executed
allcmds = [];

global netcdfFormats, shapefileFormats;
netcdfFormats = ["NetCDF4-CF", "NetCDF-4", "NetCDF3", "NetCDF-3", "NetCDF4-classic"];
shapefileFormats = ["Shapefile", "Shapefile-Combined"];
tabularAsciiFormats = ["TABULAR_ASCII"];

config = readConfigFile();

(exitstatus, cmd, inputfile, outputfile, errorCode, errorMsg) = subset(parameters,config);
allcmds.append(cmd);
debug = parameters.getDebug();
identifier = parameters.getIdentifier();
outputdir = parameters.getOutputdir() + "/" + identifier;
log = Log(debug,outputdir);

# if tabular ASCII format, estimate the output file size
# if the estimated output file size exceeds limit set in the config file
# (2GB by default), return error
if (exitstatus == 0) and (format in tabularAsciiFormats):
        (fileSize, uncompressedSize, numOfGroups, numOfDatasets) = getHDF5FileInfo(outputfile);
        fileSizeLimit = config["FileSizeLimit"];
        outputFileSizeLimit = fileSizeLimit["ASCII"];
        # use 10xuncompressed h5 file size to estimate the size of ASCII file
        if float(uncompressedSize)/1024/1024/1024*10 >= float(outputFileSizeLimit):
            exitstatus = -1;
            errorCode = "ExceedsFileSizeLimit";
            errorMsg = "Estimated ASCII size is " + "{:.2f}".format(float(uncompressedSize)/1024/1024/1024*10) + \
                       "GB which exceeds the " + str(outputFileSizeLimit) + "GB size limit of the ASCII format. " + \
                       "Please select fewer parameters and/or a smaller subset area.";

# process request, reformat as requested 
if exitstatus == 0 and os.path.isfile(outputfile):
    if (format is not None):
        if format in netcdfFormats:
            (exitstatus,cmd,inputfile, outputfile, errorCode, errorMsg) = \
                convertToNetCDF(parameters, outputfile);
            allcmds.append(cmd); 
        elif format in shapefileFormats:
            (exitstatus,cmd, inputfile, outputfile, errorCode, errorMsg) = \
                convertToShapefile(parameters, outputfile);
            allcmds.append(cmd);
        files = os.listdir(outputdir);
        if (debug != 'Y') and (format not in tabularAsciiFormats):
            for f in files:
                if f.endswith(".h5"): os.remove(os.path.join(outputdir, f));

# return success or failure xml to standard out
if exitstatus == 0 and os.path.isfile(outputfile):
    unsubsettables = ""
    download_urls = "\n        <downloadUrl>" + outputfile + "</downloadUrl>";
    if hasUnsubsettables:
        unsubsettables = "Your request included spatial or temporal subsetting, but some of the parameters are not subsettable"
    log.XMLReturnSuccessWithInfo(unsubsettables, originalInputfile, outputfile, download_urls, format, ';'.join(allcmds));
else:
    # remove output files when we encounter a failure and debug mode is off
    if os.path.isdir(outputdir) and debug != 'Y':
        for f in os.listdir(outputdir):
            os.remove(os.path.join(outputdir, f));

    # remove empty directory
    if not os.listdir(outputdir): os.rmdir(outputdir);

    log.XMLReturnFailure(exitstatus, errorCode, errorMsg);
