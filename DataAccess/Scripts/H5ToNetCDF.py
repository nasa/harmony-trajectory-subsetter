#!/tools/miniconda/bin/python

import argparse
import sys
import os
import subprocess
from shutil import copyfile
from ParameterHandling_3 import ParameterHandling
from Logging_3 import Log

parameters = ParameterHandling("H5 Reformat to NetCDF4-CF");
filepath = sys.path[0];
mode = filepath.split('/')[3];
#scriptpath = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/h5_to_netcdf";
scriptpath = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/H5ToNetCDFConverter";

inputfile = parameters.getInputfile();
identifier = parameters.getIdentifier();
outputdir = parameters.getOutputdir() + "/" + identifier;
if not os.path.exists(outputdir):
    os.makedirs(outputdir);
debug = parameters.getDebug();
log = Log(debug,outputdir);

(outputfile, internalCmd, exitstatus) = (None, None, 0);

# if the inputfile is xml file and INCLUDE_META = Y, simply copy the xml file over 
if inputfile.lower().endswith(".xml") and parameters.isMetadataIncluded():
    outputfile = outputdir + "/" + os.path.basename(inputfile);
    if (inputfile != outputfile) and os.path.isfile(inputfile):
        copyfile(inputfile, outputfile);
        internalCmd = "cp " + inputfile + " " + outputfile;
else:
    # remove ".h5" or .he5 extension in the middle of the output filename
    outputfile = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0] + ".nc";

    cmd = scriptpath + " NetCDF4-CF " + inputfile + " " + outputfile; 
    # used to remove paths for readable logging
    internalCmd = os.path.basename(scriptpath) + " NetCDF4-CF " + os.path.basename(inputfile) + \
                  " " + os.path.basename(outputfile);

    log.writeCommand(cmd);
    # invokes the command constructed above
    process = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE);
    # assigns the stdout stderr from above command to variables
    outstr, errorstr = process.communicate();
    # wait for process the terminate, and get the return code
    exitstatus = process.poll();
    sys.stderr.write(str(errorstr));

# return success or failure xml to standard out
if (exitstatus == 0) and os.path.isfile(outputfile):
    download_urls = "\n        <downloadUrl>" + outputfile + "</downloadUrl>";
    log.XMLReturnSuccess(inputfile, outputfile,download_urls,"NetCDF-4",internalCmd);
else:
    errorCode = "InternalError"
    errorMsg = "H5ToNetCDF failed with exit code"
    if (exitstatus == 3) and not os.path.isfile(outputfile):
        errorCode="NoMatchingData";
        errorMsg = "No matching data in requested fields, no output file was produced, subset area not found. Exit code";
    log.XMLReturnFailure(exitstatus, errorCode, errorMsg);
