#!/tools/miniconda/bin/python

import argparse
import sys
import os
import subprocess
from ParameterHandling_3 import ParameterHandling
from Logging_3 import Log

parameters = ParameterHandling('SMAP HDF5 Reformat to HDF-EOS5 Tool Adapter');
filepath = sys.path[0];
mode = filepath.split('/')[3];
scriptpath = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/h5_to_hdfeos5";

inputfile = parameters.getInputfile();
identifier = parameters.getIdentifier();
outputdir = parameters.getOutputdir() + "/" + identifier;
if not os.path.exists(outputdir):
    os.makedirs(outputdir);
outputfile = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0] + ".he5";
debug = parameters.getDebug();
log = Log(debug, outputdir);

cmd = scriptpath + " " + inputfile + " " + outputfile;
# used to remove paths for readable logging
internalCmd = "h5_to_hdfeos5 " + os.path.basename(inputfile) + " " + os.path.basename(outputfile);

log.writeCommand(cmd);
# invokes the command constructed above
process = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE);
# assigns the stdout stderr from above command to variables
outstr, errorstr = process.communicate();
# wait for process the terminate, and get the return code
exitstatus = process.poll();
sys.stderr.write(str(errorstr));

if (exitstatus == 0) and os.path.isfile(outputfile):
    #if os.path.dirname(os.path.abspath(inputfile)) == os.path.dirname(os.path.abspath(outputfile)):
    #    os.remove(inputfile);
    download_urls = "\n        <downloadUrl>" + outputfile + "</downloadUrl>";
    log.XMLReturnSuccess(inputfile, outputfile,download_urls,"HDF-EOS5",internalCmd);
else:
    errorCode = "InternalError";
    errorMsg = "SmapToHDFEOS5 failed with exit code";
    if (exitstatus == 3) and not os.path.isfile(outputfile):
        errorCode="NoMatchingData";
        errorMsg = "No matching data in requested fields, no output file was produced, subset area not found. Exit code";
    log.XMLReturnFailure(exitstatus, errorCode, errorMsg);
