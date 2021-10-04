#!/tools/miniconda/bin/python

'''
SpatialSubsetGeoTIFF.py
This program subsets a geotiff file given a bounding box, it currently only works for
SMAP CEA and LAEA datasets
Input parameters:
    GeoTIFF input file name
    GeoTIFF output file name
    west south east north coordinates
    height, width of grid in pixel-count (optional)
e.g. SpatialSubsetGeoTIFF.py <inputfile> <outputfile> <west> <south> <east> <north> [<height> <width>]
'''
import os
import sys
import subprocess
import math
from pyproj import Proj 

# gdal tools
gdalinfo = "/tools/gdal/bin/gdalinfo";
gdaltranslate = "/tools/gdal/bin/gdal_translate"

# # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Functions

def executeCommand(cmd):
    '''Function to execute an external command
    Args:
        cmd: command to execute
    Returns:
        command exit code,
        command standard output string
        command standard error string
    '''
    print(cmd);
    # invokes the command constructed above
    process = subprocess.Popen([cmd], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE);

    # assigns the stdout stderr from above command to variables
    outstr, errorstr = process.communicate();
    
    # wait for process the terminate, and get the return code
    exitstatus = process.poll();
    return exitstatus, outstr.decode('utf-8'), errorstr.decode('utf-8');

def getGeoInfo(fileName):
    '''Function to get information from geotiff file
       We can use georasters module if available to retrieve information
    Args:
        fileName: the geotiff file name 
    Returns:
        string(exitstatus) command exit status,
        int(xdim) x dimension size,
        int(ydim) y dimension size, 
        string(projstr) proj string, 
        float(xorigin) x origin, 
        float(yorigin) y origin, 
        float(xpixelSize) x pixel size, 
        float(ypixelSize) y pixel size     
    '''
    #Get information from a raster data set
    command = gdalinfo + " -proj4 " + fileName;
    exitstatus, outstr, errorstr = executeCommand(command);
    if (exitstatus != 0):
        return exitstatus, None, None, None, None, None,None,None;

    lines = iter(outstr.splitlines());
    for line in lines:
        line = line.strip();
        # get dimension size
        if (line.startswith("Size is")):
            (xdim, ydim) = line.replace("Size is","",1).replace(" ", "").split(",");
        elif (line.startswith("PROJ.4 string is")):
            projstr = next(lines).strip().strip("'"); 
        elif (line.startswith("Origin =")):
            (xorigin, yorigin) = line.replace("Origin =","",1).replace("(","",1).replace(")","",1).replace(" ", "").split(",");
        elif (line.startswith("Pixel Size =")):
            (xpixelSize, ypixelSize) = line.replace("Pixel Size =","",1).replace("(","",1).replace(")","",1).replace(" ", "").split(",");

    print(exitstatus, int(xdim), int(ydim), projstr, float(xorigin), float(yorigin), float(xpixelSize), float(ypixelSize));
    return exitstatus, int(xdim), int(ydim), projstr, float(xorigin), float(yorigin), float(xpixelSize), float(ypixelSize);


def computeGridExtent(west,south,east,north,projstr):
    '''Function to compute the Grid extension in meters
    Args:
        west(float),south(float),east(float),north(float): bounding box
        projstr(string): proj string
    Returns:
        float(xmin), float(ymin), float(xmax), float(ymax) min, max x y of subsetted area 
    '''
    if (projstr.count("+proj=cea +lon_0=0") > 0):
        return computeGridExtentForCea(west,south,east,north,projstr);
    elif (projstr.count("+proj=laea +lat_0=90 +lon_0=0 +x_0=0 +y_0=0") > 0 or 
          projstr.count("+proj=laea +lat_0=-90 +lon_0=0 +x_0=0 +y_0=0") > 0):
        return computeGridExtentForLaea(west,south,east,north,projstr);

def computeGridExtentForCea(west, south, east, north, projstr):
    '''Function to compute the Grid extension in meters for CEA
    Args:
        west(float),south(float),east(float),north(float): bounding box
        projstr(string): proj string
    Returns:
        float(xmin), float(ymin), float(xmax), float(ymax) min, max x y of subsetted area
    '''
    # restrict the longitude range for cea
    if (north > 85.0445): north = 85.0445;
    if (south < -85.0445): south = -85.0445;

    # note subsetting with west>east or south>north isn't really working with gdal_translate,
    # but keep the logic from the original bash script
    if (west > east): projstr = projstr.replace("+lon_0=0", "+lon_0=180", 1);
    if (south > north): projstr = projstr.replace("+lat_0=0", "+lat_0=90",1);

    pj = Proj(projstr);
    (xmin, ymax) = pj(west, north);
    (xmax, ymin) = pj(east, south); 

    print("computeGridExtentForCea ", xmin, ymin, xmax, ymax);
    return xmin, ymin, xmax, ymax;

# Input parameters are points along arc in degrees
def arcIncludes(start, end, point):
    '''Function to figure out if the longitude point is between the start and end longitudes
       of the bounding box
    Args:
        start(float), end(float) start and end longitudes along arc in degrees
        point(float): the point in degree to check 
    Returns:
        bool: True if point is between start and end, False otherwise 
    '''

    # Reduce to 0-360 degrees (wrap-around on circle)
    start = math.fmod(start, 360.0);
    end = math.fmod(end, 360.0);
    point = math.fmod(point, 360.0);

    # Address wrap-around (start > end) on circle
    #  Adding 360 brings end around to same point on circle, but is > start
    if (start > end): end += 360.0; 
    # Coerce point to be > start
    #  Adding 360 brings point around to same location, but is > start
    if (point < start): point += 360.0;
    # for our application we want an open interval (the corner points are already tested)
    if (point < end): return True;
    return False;

def computeGridExtentForLaea(west,south,east,north,projstr):
    '''Function to compute the Grid extension in meters for LAEA 
    Args:
        west(float),south(float),east(float),north(float): bounding box
        projstr(string): proj string
    Returns:
        float(xmin), float(ymin), float(xmax), float(ymax) min, max x y of subsetted area
    '''
    # 0 longitude points up for south, down for north, -90 is left for both
    if (projstr.count("+proj=laea +lat_0=90 +lon_0=0 +x_0=0 +y_0=0") > 0): 
        npp = 1;
        # determine latitude bound closest to the equator
        if (south < 0): south = 0;
        lat_near_equator = south;
    if (projstr.count("+proj=laea +lat_0=-90 +lon_0=0 +x_0=0 +y_0=0") > 0):
        npp = 0;
        if (north > 0): north = 0;
        lat_near_equator = north;

    pj = Proj(projstr);
    # get min max x y based on the four corner points of the bbox
    lons = [west,west,east,east];
    lats = [north,south,north,south];
    # convert lon/lats to geo-coordinates (projected meters), all points at once
    xs,ys = pj(lons,lats);
    print(lons, lats);
    print(xs, ys);
    (xmin, xmax, ymin, ymax) = (min(xs), max(xs), min(ys), max(ys));
    print("computeGridExtentForLaea min max x y based on four corners ", xmin, ymin, xmax, ymax);

    # consider intersections of the arc-sector with the axes of the grid(due North, South, East, West from the center pole.)
    # (1) These points of intersection with the axes of the grid will have the latitude taken from the bounding-box, 
    # the inner and outer latitude values or min/max North-South latitude values.
    # (2) The longitude will have the value 0, 90, 180, or -90, depending on which axis is crossed by the arc-sector.
    # Note that the arc-sector can cross multiple axes.

    # clear the lon/lats to load new points 
    lons[:] = [];
    lats[:] = []; 
    # check if arc crosses date line (180), gmt (0), east axis, west axis
    if (arcIncludes(west, east, 180.0)): lons.append(180.0);
    if (arcIncludes(west, east, 0.0)): lons.append(0.0);
    if (arcIncludes(west, east, 90.0)): lons.append(90.0);
    if (arcIncludes(west, east, -90.0)): lons.append(-90.0);

    if (len(lons) > 0):
        # latitude is always lat_near_equator
        for i in lons: lats.append(lat_near_equator);
        xs,ys = pj(lons,lats);
        print(lons, lats);
        print(xs, ys);
        if (xmax < max(xs)): xmax = max(xs);
        if (xmin > min(xs)): xmin = min(xs);
        if (ymax < max(ys)): ymax = max(ys);
        if (ymin > min(ys)): ymin = min(ys);
        print("adding intersections of axes min max x y ", xmin, ymin, xmax, ymax);

    return xmin, ymin, xmax, ymax; 


def computeExtendedGridExtent(xmin,ymin,xmax,ymax,xorigin,yorigin, xpixelSize, ypixelSize, xdim, ydim):
    '''Function to extend the grid extent to include the partially intersected cells
    Args:
        xmin(float),ymin(float),xmax(float),ymax(float): min, max x y of current grid extent,
        xorigin(float),yorigin(float): origin point in meters of input file
        xpixelSize(float), ypixelSize(float): x y pixel size
        xdim(int), ydim(int): x y dimension size of input file 
    Returns:
        int(mincol),int(minrow), int(maxcol), int(maxrow): extended grid in pixel locations
    '''
    # xorigin + mincol*pixelSizeX <= xmin
    mincol = math.floor((xmin - xorigin)/xpixelSize);
    if (mincol < 0): mincol = 0;
    # yorigin + minrow*pixelSizeY >= ymax
    minrow = math.floor((ymax-yorigin)/ypixelSize);
    if (minrow < 0): minrow = 0;
    # xorigin + maxcol*pixelSizeX >= xmax
    maxcol = math.ceil((xmax-xorigin)/xpixelSize)-1;
    if (maxcol > xdim-1): maxcol = xdim-1; 
    # yorigin + maxrow*pixelSizeY <= ymin
    maxrow = math.ceil((ymin-yorigin)/ypixelSize)-1;
    if (maxrow > ydim-1): maxrow = ydim-1; 

    print(mincol, minrow, maxcol, maxrow);
    return int(mincol), int(minrow), int(maxcol), int(maxrow);

# # # # # # # # # # # # # # # # # # # # # # # # # # # #
# main work flow

if (len(sys.argv) < 7):
    print("Usage: SpatialSubsetGeoTIFF.py <inputfile> <outputfile> <west> <south> <east> <north> [<height> <width>]");
    sys.exit(1);
sys.stderr.write("\nSpatialSubsetGeoTIFF");
# retrieve command line parameters and save to global variables
(scriptName, inputFile, outputFile) = sys.argv[0:3];
(west, south, east, north) = (float(sys.argv[3]), float(sys.argv[4]), float(sys.argv[5]), float(sys.argv[6]));
height, width = None, None;
if (len(sys.argv) == 9): (height, width) = (int(sys.argv[7]), int(sys.argv[8]));

# get geo information from tif file 
(exitstatus, xdim, ydim, projstr, xorigin, yorigin, xpixelSize, ypixelSize) = getGeoInfo(inputFile);
# compute grid extent based on the bounding box 
(xmin, ymin, xmax, ymax) = computeGridExtent(west,south,east,north,projstr);
# extend the grid to include partially intersected cells 
(mincol, minrow, maxcol, maxrow) = computeExtendedGridExtent(xmin, ymin, xmax, ymax,
                                   xorigin, yorigin, xpixelSize, ypixelSize, xdim, ydim);

# build and execute subset command
subsetCmd = gdaltranslate + " -co COMPRESS=DEFLATE";
# -srcwin xoff yoff xsize ysize
subsetCmd += " -srcwin " + str(mincol) + " " + str(minrow) + " " + str(maxcol-mincol+1) + " " + str(maxrow-minrow+1);
if height is not None: subsetCmd += " -outsize " + str(height) + " " + str(width);
subsetCmd += " " + inputFile + " " + outputFile
(exitstatus, outstr, errstr) = executeCommand(subsetCmd);
print(outstr);
if (exitstatus != 0): print(sys.stderr, errstr); 
sys.exit(exitstatus);

