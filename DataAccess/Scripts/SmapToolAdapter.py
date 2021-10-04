#!/tools/miniconda/bin/python

import os
import sys
import subprocess
import re
import json

from datetime import datetime
from Logging_3 import Log
from ParameterHandling_3 import ParameterHandling
from SubsetDataLayers_3 import SubsetDataLayers

noprj = ''
hasNobidings = False
tiferr = ''
hasTifIssue = False

# Tool adapter for SMAP L1L2 data access services
bbox = "";

# # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Functions

'''
  function to execute and external command
  Args:
      cmd(string): command to execute
  Returns:
      command exit code - int
      command standard output - string
      command standard error - string
'''
def executeCommand(cmd):
    global tiferr
    global hasTifIssue
    global unbindpars
    
    # invokes the command
    process = subprocess.Popen([cmd], shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE);

    # gets the standard output and standard error message
    outstr, errorstr = process.communicate();

    # wait for the process to terminate, and get the return code
    exitstatus = process.poll();
    
    unbindpars = ""
    if subsetDataLayers is None and noprj :
        num = 5
        out =outstr.decode("utf-8").split("\n")
        for index, line in enumerate(out):  # enumerate the list and loop through it
            result = re.search(rf"{noprj}\w+", line)
            if result:
                arg = result.group(0)
                unbindpars += "".join(arg+", ")
        unbindpars = unbindpars.rstrip(', ') + " ...more"

    if str(outstr).find("Skipping")  > 0 :
        filestr = re.findall(r'(?<=Skipping) .*.tif', str(outstr))[0]
        tiferr = filestr.split('/')[len(filestr.split('/'))-1]
        hasTifIssue = True

    return exitstatus, str(outstr), str(errorstr);

'''
  return corresponding error code and error message
  Args:
      exit status - int
      sub exit status(designated for reformatting error) - int
  Returns:
      corresponding error code - string
      corresponding error message - string
'''
def getErrorCode(exitstatus, subStatus=0):
    
    # exitstatus = 1 is reserved for invalid parameter error case
    # exitstatus = 2 is reserved for missing parameter error case
    # exitstatus = 3 is reserved for no matching data error case
    # exitstatus = 99 is reserved for reformatting error case
    # all other exitstatus goes to internal error
    errorCode = "InternalError";
    errorMsg = "An internal error has occured.";

    if exitstatus == 1:
        errorCode = "InvalidParameter";
        errorMsg = "Incorrect parameter specified for given dataset(s).";
    elif exitstatus == 2:
        errorCode = "MissingParameter";
        errorMsg = "No parameter value(s) specified for give dataset(s).";
    elif exitstatus == 3:
        errorCode = "NoMatchingData";
        errorMsg = "No data found that matched subset constraints.";
    elif exitstatus == 99:
        errorCode = "ReformattError";
        errorMsg = "EcDlDaRqs.properties doesn't exist";
        if (subStatus == 1): errorMsg = "OPeNDAP and/or gdal configuration is missing in EcDlDaRqs.properties";
    return errorCode, errorMsg;

'''
  spatially subset GeoTIFF files
  Args:
      output directory - string
      parameters - ParameterHandling
      bounding box - string
      log - Logging
  Returns:
      command - string
'''
def spatialSubsetGeoTIFF(outputdir,parameter,bbox,log):

    currentDirectory = sys.path[0];
    mode = currentDirectory.split('/')[3];
    spatialSubsetGeoTIFFScript = "/usr/ecs/" + mode + "/CUSTOM/utilities/SpatialSubsetGeoTIFF.py";

    width = parameter.getWidth();
    height = parameter.getHeight();

    # get all of the GeoTIFF files in the output directory
    tifFiles = [os.path.join(outputdir,f) for f in os.listdir(outputdir) if os.path.isfile(os.path.join(outputdir, f)) and f.rsplit(".", 1)[1] == "tif"];

    # spatially subset each of the GeoTIFF files in the output directory
    for f in tifFiles:
        if "REMOVE_TO_DISABLE_Polar_Projection" not in f:
            newFilename = f + ".precrop.tif";
            os.rename(f, newFilename);
            bbox = bbox.replace(",", " ");
            spatialSubsetGeoTIFFcmd = spatialSubsetGeoTIFFScript + " " + newFilename + " " + f + " " + bbox + " ";
            if width is not None: spatialSubsetGeoTIFFcmd =+ width;
            if height is not None: spatialSubsetGeoTIFFcmd =+ " " + height;
            spatialSubsetGeoTIFFInternalcmd = os.path.basename(spatialSubsetGeoTIFFScript) + " " + os.path.basename(newFilename) + " " + os.path.basename(f) + " " + bbox + " ";
            if width is not None: spatialSubsetGeoTIFFInternalcmd =+ width;
            if height is not None: spatialSubsetGeoTIFFInternalcmd =+ " " + height;
            log.writeCommand(spatialSubsetGeoTIFFcmd);
            exitStatus, stdOut, stdErr = executeCommand(spatialSubsetGeoTIFFcmd);
            if "ERROR" in stdErr or "FAILURE" in stdErr:
                os.rename(newFilename, f);
            else:
                os.remove(newFilename);
    return spatialSubsetGeoTIFFInternalcmd;

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

def readJsonConfigFile():
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

'''
  perform subset
  Args:
      parameters(ParameterHandling): command line parameters
      log - Logging
  Returns:
      exit status of the command executed - int
      command executed - string
      input file name - string
      output file name - string
      corresponding error code - string
      corresponding detailed error message - string
      bounding box - string
'''
def subset(parameters, log):
    global hasNobidings
    global hasTifIssue
    global noprj
    global subsetDataLayers
    global unbindpars


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

    config = readJsonConfigFile();

    cmd = scriptpath + " --configfile " + cfgfile + \
          " --filename " + inputfile + " --outfile " + outputfile;
    internalcmd = os.path.basename(scriptpath) + " --filename " + os.path.basename(inputfile) + " --outfile " + os.path.basename(outputfile);

    subsetDataLayers = parameters.getSubsetDataLayers();
    if subsetDataLayers is not None:
        dataLayers = SubsetDataLayers(subsetDataLayers);
        cmd += " --includedataset " + ",".join(dataLayers.dataLayers);
        internalcmd += " --includedataset " + ",".join(dataLayers.dataLayers);

    boundingShape = parameters.getBoundingShape();
    if boundingShape is not None: cmd += " --boundingshape " + boundingShape;

    bbox = parameters.getBbox();
    noprj = ''
    if bbox is not None: 
        cmd += " --bbox " + bbox;
        internalcmd += " --bbox " + bbox;
        # Define in which hemisphere bbox is
        lat_south = float(bbox.split(',')[1])
        lat_north =float(bbox.split(',')[3])
        if lat_south >= lat_north or float(bbox.split(',')[2]) <= float(bbox.split(',')[0]):
            exitstatus = 1;
            errorCode, errorMsg = getErrorCode(exitstatus);
            errorMsg += " Bad request BBOX parameters: BBOX[" + bbox + "]"
            return exitstatus, "", inputfile, outputfile, errorCode, errorMsg, bbox;
        if lat_north > 0 and lat_south >= 0 and lat_north <= 90:
            noprj = '/South_Polar_Projection/'
            hasNobidings = True
        elif lat_south < 0 and lat_north <= 0 and lat_south >= -90:
            noprj = '/North_Polar_Projection/'
            hasNobidings = True

    projection = parameters.getProjection()
    projectionParams = parameters.getProjectionParameters();

    # exit when there is not reformat but projection is specified
    if projection is not None and format is None:
        exitstatus = 2;
        errorCode, errorMsg = getErrorCode(exitstatus);
        return exitstatus, "", inputfile, outputfile, errorCode, errorMsg, bbox;
    if format == "KML" and projection:
        projection = "";
    if format is not None:
        if format in ["GeoTIFF", "HDF-EOS5", "NetCDF4-CF"]:
            if (projection is None or "POLAR" not in projection):
                cmd += " --reformat GeoTIFF";
                internalcmd += " --reformat GeoTIFF";
        else:
            cmd += " --reformat " + format;
            internalcmd += " --reformat " + format;

    # auto-calculate the bbox for UTM SOUTHERN/NORTHERN HEMISPHERE projections from a 
    # given NZONE when no bbox is specified
    if (bbox is None) and (projectionParams) and (projection is not None and "HEMISPHERE" in projection):
        params = projectionParams.split(",");
        for param in params:
            value = param.split(":");
            if value[0] == "NZone": nzone = value[1];

        if nzone is not None:
            north = 90;
            south = -90;
            west = int(nzone) * 6 - 228;
            east = west + 90;
  
            if west < -180: west = -180;
            if east > 180: east = 180;

        bbox = str(west) + "," + str(south) + "," + str(east) + "," + str(north);
        cmd += " --bbox " + bbox; 
        internalcmd += " --bbox " + bbox;

    # auto-calculate the bbox for NORTH/SOUTH POLAR STEREOGRAPHIC projections when no
    # bbox is specified
    if (bbox is None) and (projection is not None and "TH POLAR STEREOGRAPHIC" in projection):
        north = 90;
        south = -90;
        west = -180;
        east = 180;

        if projection.startswith("NORTH"): south = 0;
        elif projection.startswith("SOUTH"): north = 0;

        bbox = str(west) + "," + str(south) + "," + str(east) + "," + str(north);
        cmd += " --bbox " + bbox;
        internalcmd += " --bbox " + bbox;

    debug = parameters.getDebug();
    log.writeCommand(cmd);
    exitstatus, stdout, stderr = executeCommand(cmd);
    # List of unbinded parameters
    if subsetDataLayers is not None:     
        unbindpars += "".join(s + ',' for s in dataLayers.dataLayers if noprj in s)
        unbindpars = unbindpars.rstrip(', ')
    if not os.listdir(outputdir): exitstatus = 3;
    errorCode = "";
    errorMsg = "";
    if exitstatus != 0: (errorCode, errorMsg) = getErrorCode(exitstatus);

    log.writeStdOutAndStdErr(stdout, stderr);

    # if failed, log last 10 lines of standard out to rqs log
    if exitstatus != 0 and debug != "Y":
        sys.stderr.write(stderr);
        sys.stderr.write(";".join(stdout.split('\n')[-10:]));

    if format == "KML" or (format == "GeoTIFF" and (projection is not None and "POLAR" not in projection)):
        if os.path.exists(outputfile): os.remove (outputfile);

    return exitstatus, internalcmd, inputfile, outputfile, errorCode, errorMsg, bbox;

'''
  get opendap configuration from EcDlDaRqs.properties
  Returns:
      exit status - int
      opendap URL base - string
      opendap root directory - string
      GDAL translate directory - string
      error code - string
      error message - string
'''
def readConfigFile():

    currentDirectory = sys.path[0];
    mode = currentDirectory.split('/')[3];
    configFilePath = "/usr/ecs/" + mode + "/CUSTOM/cfg/EcDlDaRqs.properties";

    exitstatus = 0;
    errorCode = "";
    errorMsg = "";

    if not os.path.isfile(configFilePath):
        exitstatus = 99;
        errorCode, errorMsg = getErrorCode(99);
        return;

    opendapUrlBase = "";
    opendapRootDir = "";
    gdalTranslate = "";

    with open(configFilePath) as cfg:
        for line in cfg:
            line = line.rstrip("\n");
            if line.startswith("opendap.url.base"):
                opendapUrlBase = line.split("=")[1];
            if line.startswith("opendap.root.dir"):
                opendapRootDir = line.split("=")[1];
            if line.startswith("gdal.translate"):
                gdalTranslate = line.split("=")[1];

    if (not opendapUrlBase) or (not opendapRootDir) or (not gdalTranslate):
        exitstatus = 99;
        errorCode, errorMsg = getErrorCode(99,1);

    return exitstatus, opendapUrlBase, opendapRootDir, gdalTranslate, errorCode, errorMsg;

'''
  call tifToKML script to reformat from GeoTIFF to KML
  Args:
     output directory - string
     debug - string (Y or N or empty)
     log - Logging
  Returns:
      exit status - int
      kml file name - string
      error code - string
      error message - string
'''
def reformatTiffToKML(outputdir, debug, log):

    #sys.stderr.write("\nreformatTiffToKML");
    currentDirectory = sys.path[0];
    mode = currentDirectory.split('/')[3];
    tifToKmlScript = "/usr/ecs/" + mode + "/CUSTOM/utilities/tif2kml.sh";

    tifFiles = [os.path.join(outputdir,f) for f in os.listdir(outputdir) if os.path.isfile(os.path.join(outputdir, f)) and f.rsplit(".",1)[1] == "tif"];

    exitstatus = 0;
    kmlfile = "";
    tifToKmlInternalCmd = "";
    errorCode = "";
    errorMsg = "";

    if len(tifFiles) == 0: exitstatus = -1;

    for f in tifFiles:
        kmlfile = f + ".kmz";

        tifToKmlCmd = tifToKmlScript + " " + f + " " + kmlfile; 
        tifToKmlInternalCmd = os.path.basename(tifToKmlScript) + " " + os.path.basename(f) + " " + os.path.basename(kmlfile);

        if debug == "Y":
            tifToKmlCmd = tifToKmlScript + " " + f + " " + kmlfile + " --DEBUG";
            tifToKmlInternalCmd = os.path.basename(tifToKmlScript) + " " + os.path.basename(f) + " " + os.path.basename(kmlfile) + " --DEBUG";

        log.writeCommand(tifToKmlCmd);
        (exitstatus, stdOut, stdErr) = executeCommand(tifToKmlCmd);
        if "ERROR" in stdOut:
            exitstatus = 99;
            
    errorCode = "";
    errorMsg = "";
    if exitstatus != 0: errorCode, errorMsg = getErrorCode(exitstatus);

    return exitstatus, tifToKmlInternalCmd, kmlfile, errorCode, errorMsg;
'''
  file reformatting
  Args:
      input file name - string;
      output file format - string;
      opendap URL base - string
      opendap root directory - string
      GDAL translate directory - string
      log - Log
  Returns:
      exitstatus - int
      reforamt command - string
      input file name - string
      output file name - string
      errorCode - string
      errorMsg - string;
'''
def reformat(inputfile, parameters, opendapUrlBase, opendapRootDir, gdalTranslate, log):

    # construct OPeNDAP url
    #inputfile = os.path.splitext(inputfile)[0];
    opendapUrl = inputfile.replace(opendapRootDir, "");
    opendapUrl = opendapUrlBase + "/" +  opendapUrl;

    extension = "";
    reformatcmd = "";
    outputfile = "";
    nctonc4cmd = ""; 
    errorCode = "";
    errorMsg = "";
    service_error = "";
    resource_error = "";
    reformat_error = "";

    ncFormats = ["NetCDF3", "NetCDF-3", "NetCDF4-classic"];

    curl = "curl --silent --show-error -o ";
    nccopy = "/tools/netcdf/bin/nccopy -k 4 ";

    if (format == "ASCII"):
        extension = ".ascii";
        outputfile = inputfile + extension;
        reformatcmd = curl + outputfile + " " + opendapUrl + extension;
        reformatInternalCmd = curl + os.path.basename(outputfile) + " " + opendapUrl + extension;
    elif format in ncFormats:
        extension = ".nc";
        outputfile = inputfile + extension;
        reformatcmd = curl + outputfile + " " + opendapUrl + extension;
        reformatInternalCmd = curl + os.path.basename(outputfile) + " " + opendapUrl + extension;
        if (format == "NetCDF4-classic"):
            nc4extension = ".nc4";
            nc4Outputfile = inputfile + nc4extension;
            nctonc4cmd = nccopy + nc4Outputfile + " " + outputfile;
            nctonc4InternalCmd = os.path.basename(nccopy) + os.path.basename(nc4Outputfile) + " " + os.path.basename(outputfile);
    elif (format == "KML"):
        exitstatus, reformatInternalCmd, outputfile, errorCode, errorMsg = reformatTiffToKML(outputdir, debug, log);

    if (reformatcmd != ""):
        log.writeCommand(reformatcmd);
        (exitstatus, stdOut, stdErr) = executeCommand(reformatcmd);
        if os.path.exists(outputfile):
            if format in ncFormats:
                status, stdout, stderr = executeCommand("file " + outputfile + " | egrep -o 'NetCDF|Hierarchical'");
                isbinary = str(stdout);
                if not isbinary:
                    exitstatus = 99;
                errorCode, errorMsg = getErrorCode(exitstatus);
            if format not in ncFormats or not isbinary:
                head = "";
                firstFour = "";
                with open(outputfile) as myfile:
                    lineNum = 1;
                    for line in myfile:
                        if lineNum > 20: break;
                        head += line;
                        if lineNum >= 4: firstFour += line;
                        lineNum += 1;
                if "503 Service Temporarily Unavailable" in head:
                    service_error = "503 Service Temporarily Unavailable";
                if "Hyrax: Resource Not Found" in head:
                    resource_error = "Hyrax: Resource Not Found";
                if "Error" in head:
                    if "<title>" in firstFour: reformat_error = re.sub('<[^>]\*>', '', head);
                if service_error or resource_error or reformat_error:
                    exitstatus = 99;
                    errorCode = "ReformatError";
                    errorMsg = "OPeNDAP reponse error message " + reformat_error + " " + service_error + " " + resource_error;

    if exitstatus == 0 and os.path.exists(outputfile):
        newOutputfile = outputfile.replace(".h5", "").replace(".tif", "");
        os.rename(outputfile, newOutputfile);
        outputfile = newOutputfile;
    return exitstatus, reformatInternalCmd, inputfile, outputfile, errorCode, errorMsg;

# # # # # # # # # # # # # # # # # # # # # # # # # # # #
# main work flow

parameters = ParameterHandling("SMAP Tool Adapter");
format = parameters.getFormat();

#commands executed
allcmds = [];

debug = parameters.getDebug();
identifier = parameters.getIdentifier();
outputdir = parameters.getOutputdir() + "/" + identifier;
projection = parameters.getProjection();

log = Log(debug, outputdir);

(exitstatus, cmd, inputfile, outputfile, errorCode, errorMsg, bbox) = subset(parameters, log);
allcmds.append(cmd);
#sys.stderr.write("\nexit: " + str(exitstatus));
formats = ["KML", "ASCII", "NetCDF3", "NetCDF-3", "NetCDF4-classic"];

# if reformatting is required through OPeNDAP services
if (format and format in formats) and exitstatus == 0:
    #sys.stderr.write("\nreformat");
    (exitstatus, opendapUrlBase, opendapRootDir, gdalTranslate, errorCode, errorMsg) = readConfigFile();

    if (exitstatus == 0):
       (exitstatus, cmd, inputfile, outputfile, errorCode, errorMsg) = reformat(outputfile, parameters, opendapUrlBase, opendapRootDir, gdalTranslate, log);
       allcmds.append(cmd);
       if os.path.exists(inputfile): os.remove(inputfile);

outputfiles = [];
#sys.stderr.write("len: " + str(len(os.listdir(outputdir))));
#if format == "GeoTIFF" and exitstatus == 0:
#    if (projection is not None and "POLAR STEREOGRAPHIC" not in projection)  and bbox is not None:
if exitstatus == 0 and \
   format in ["GeoTIFF", "HDF-EOS5", "NetCDF4-CF"] and \
   bbox and (projection and "POLAR STEREOGRAPHIC" not in projection):
        cmd = spatialSubsetGeoTIFF(outputdir, parameters, bbox, log);
        allcmds.append(cmd);

sys.stderr.write("len: " + str(len(os.listdir(outputdir))));
for f in os.listdir(outputdir):
    f = os.path.join(outputdir, f);
    if format == "GeoTIFF" and not projection:
        if f == outputfile and os.path.exists(outputfile) and outputfile.endswith(".h5"):
            os.remove(outputfile);
        else:
            newtif = f.replace(".h5", "");
            if os.path.exists(f):
                os.rename(f, newtif);
                outputfiles.append(newtif);
    elif format == "KML":
        newtif = f.replace(".h5", "").replace(".tif", "");
        os.rename(f, newtif);
        outputfiles.append(newtif);
    else: outputfiles.append(f);

if format == "GeoTIFF" and len(os.listdir(outputdir)) == 0 and exitstatus != 3:
    exitstatus = 1
    errorCode = "InvalidParameter"
    errorMsg = "Skipping output file " + tiferr + ". String data type cannot be output to GeoTIFF."

download_urls = "";
# return success or failure xml to standard out
if (exitstatus == 0):
    unbindings = ""
    if outputfiles:
         for f in outputfiles:
             #download_urls += "\n        <downloadUrl>" + f + "</downloadUrl>";
             if format is not None and (projection is not None and "POLAR STEREOGRAPHIC" not in projection):
                 if f.endswith(".tif"):
                     download_urls += "\n        <downloadUrl>" + f + "</downloadUrl>";
             else:
                 download_urls += "\n        <downloadUrl>" + f + "</downloadUrl>";
    if hasNobidings:
        unbindings += "Your request included spatial subsetting (BBOX[" + parameters.getBbox() + "]) and the following specified parameters have no data found: " + unbindpars
    if hasTifIssue:
        unbindings = "Skipping output file " + tiferr + ". String data type cannot be output to GeoTIFF."

    log.XMLReturnSuccessWithInfo(unbindings, inputfile, outputfile,download_urls,format,';'.join(allcmds));
else:
    # remove empty directory
    if not os.listdir(outputdir): os.rmdir(outputdir);

    log.XMLReturnFailure(exitstatus, errorCode, errorMsg);
