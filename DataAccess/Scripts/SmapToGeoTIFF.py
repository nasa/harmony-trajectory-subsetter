#!/tools/miniconda/bin/python

#This utility is called in the smapl1l2_to_polar chain to generate geotiff files in the polar project
# GDAL doesn't seem to be able to produce this output 
#
# INPUT: an HDF5 file with SMAP data
# OUTPUT: a geotiff file for each of the bands in the input
#
import sys
import os
from Logging_3 import Log
from ParameterHandling_3 import ParameterHandling
from SubsetDataLayers_3 import SubsetDataLayers

filepath = sys.path[0];
mode = filepath.split('/')[3];

# argument processing
parameters = ParameterHandling("SMAP GeoTIFF Reprojection To Polar Stereogrphic Tool Adapter");
inputfile = parameters.getInputfile();
outputdir = parameters.getOutputdir() + "/" + parameters.getIdentifier();
subset_data_layers = parameters.getSubsetDataLayers();
dataLayers = SubsetDataLayers(subset_data_layers);
datasets = dataLayers.getDatasets(inputfile);
bbox = parameters.getBbox();
if bbox is None: bbox = "";
format = parameters.getFormat();

projection = parameters.getProjection(); 
if projection == "NORTH POLAR STEREOGRAPHIC":
    proj = "'+proj=stere +lat_0=90 +lat_ts=90 +lon_0=0 +k_0=1.0 +datum=WGS84 +ellps=WGS84'";
else:
    proj = "'+proj=stere +lat_0=-90 +lat_ts=-90 +lon_0=0 +k_0=1.0 +datum=WGS84 +ellps=WGS84'";

debug = parameters.getDebug();
log = Log(debug, outputdir);

notHDF5 = True; 
exitstatus = 0;
#ignore not HDF5 files
if os.path.splitext(inputfile)[1] == ".h5":
    notHDF5 = False;
    outputdir = outputdir.replace("'","");
    inputfilename = inputfile.rpartition("/")[2];
    if not os.path.exists(outputdir):
        os.makedirs(outputdir);
    download_urls, all_cmds, output_files = "","","";
    for ds in datasets:
        fieldname = ds.rpartition("/")[2];
        groupname = ds.partition("/")[2].replace("/" + fieldname, "");
        # remove ".h5" extension in the middle of the output filename for GeoTIFF format 
        #     (if this is its last step in chaining)
        if (format == "GeoTIFF"):
            outputfile = outputdir + "/" + os.path.splitext(inputfilename)[0] + "_" + groupname + "." + fieldname + ".tif";
        else:
            outputfile = outputdir + "/" + inputfilename + "_" + groupname + "." + fieldname + ".tif";
        cmd = "/usr/ecs/" + mode + "/CUSTOM/bin/DPL/smap_to_geotiff " + inputfile + " " + outputfile + " " + proj + " " + ds + " " + bbox;
        exitstatus = os.system(cmd + " > /dev/null");
        sys.stderr.write(cmd+";");
        # removed paths for readable logging
        all_cmds += "smap_to_geotiff " + os.path.basename(inputfile) + " " + os.path.basename(outputfile) + " " + proj + " " + ds + " " + bbox +";";
        if (exitstatus != 0 and exitstatus != 256):
            break;

    # get the list of .tif files in the output directory
    for item in os.listdir(outputdir):
        if item.endswith(".tif"):
            tifFile = os.path.join(outputdir,item);
            output_files += tifFile + " ";
            download_urls += "\n        <downloadUrl>" + tifFile + "</downloadUrl>";

if notHDF5 == True:
    error_code = "InternalError";
    error_msg = "Input file is not a HDF5 file";
    log.XMLReturnFailure(exitstatus, error_code, error_msg); 
elif (exitstatus == 0 or exitstatus == 256) and len(download_urls)!=0:
    log.XMLReturnSuccess(inputfile, output_files, download_urls, format, all_cmds);
else:
    error_code = "InternalError";
    error_msg = "SmapToGeoTIFF failed with exit code"; 
    log.XMLReturnFailure(exitstatus, error_code, error_msg);
