#!/tools/miniconda/bin/python

#This utility is called in the smapl1l2_gdalwarp chain if the output format requested is HDF-EOS5

import sys
import os
import subprocess
from ParameterHandling_3 import ParameterHandling
from Logging_3 import Log

parameters = ParameterHandling("SMAP GeoTIFF Reprojection Reformat to HDF-EOS5 Tool Adapter");
filepath = sys.path[0];
mode = filepath.split('/')[3];
scriptpath = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/geotiff_to_hdfeos5";

inputfile = parameters.getInputfile();
identifier = parameters.getIdentifier();
outputdir = parameters.getOutputdir() + "/" + identifier;
format = parameters.getFormat();
if format is None:
    format = "GeoTIFF";
projection = parameters.getProjection();
if projection is None:
    projection = "";
inputfilename = inputfile.rpartition("/")[2];
metadatafilename = inputfilename.partition(".h5")[0];
metadatafilename = metadatafilename.replace("warped_",""); #should use lreplace?
metadatafilename += ".h5";
if format == "HDF-EOS5":
    outputfile = outputdir + "/" + metadatafilename.rpartition(".")[0] + ".he5";
else:
    outputfile = inputfile.replace("warped_","");
if not os.path.exists(outputdir):
    os.makedirs(outputdir);
groupname = inputfilename.rpartition(".")[0].rpartition(".")[0].partition(".h5_")[2];
fieldname = inputfilename.rpartition(".")[0].rpartition(".")[2];
metadatafilename = outputdir + "/" + metadatafilename;
debug = parameters.getDebug();
log = Log(debug, outputdir);

if format == "HDF-EOS5": 
    cmd = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/geotiff_to_hdfeos5 " + inputfile + " " + outputfile + " " + groupname + " " + fieldname + " " + metadatafilename + " " + projection + " ";
    # used to remove paths for readable logging
    internalCmd = "geotiff_to_hdfeos5 " + os.path.basename(inputfile) + " " + os.path.basename(outputfile) + " " + groupname + " " + fieldname + " " + os.path.basename(metadatafilename) + " " + projection + " ";
else:
    cmd = "/bin/mv " + inputfile + " " + outputfile;
    internalCmd = "/bin/mv " + os.path.basename(inputfile) + " " + os.path.basename(outputfile);

log.writeCommand(cmd);
# invokes the command constructed above
process = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE);
# assigns the stdout stderr from above command to variables
outstr, errorstr = process.communicate();
# wait for process the terminate, and get the return code
exitstatus = process.poll();
sys.stderr.write(str(errorstr));

if (exitstatus == 0 or exitstatus == 256) and os.path.isfile(outputfile):
    #if os.path.dirname(os.path.abspath(inputfile)) == os.path.dirname(os.path.abspath(outputfile)):
    #    os.remove(inputfile);
    download_urls = "\n        <downloadUrl>" + outputfile + "</downloadUrl>";
    log.XMLReturnSuccess(inputfile, outputfile, download_urls, format, internalCmd);
else:
    # display the error message and return code to the end user
    errorCode = "InternalError"
    errorMsg = "GeoTiffToHDFEOS5 failed with exit code"
    log.XMLReturnFailure(exitstatus, errorCode, errorMsg);
