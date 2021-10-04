import argparse

class ParameterHandling:
    'Handles query parameters'

    def __init__(self, desc):
        parser = argparse.ArgumentParser(description=desc);
        parser.add_argument('--FILE_URLS', type=str, required=True, help='Name of the input file to reformat');
        parser.add_argument('--OUTPUT_DIR', type=str, required=True, help='Name of the output directory to put the output file');
        parser.add_argument('--IDENTIFIER', type=str, help='Identifier of the reuqest used to determine output directory');
        parser.add_argument('--FORMAT', type=str, help='Used to determine output format');
        parser.add_argument('--SUBSET_DATA_LAYERS', type=str, help='Datasets to be included');
        parser.add_argument('--PROJECTION', type=str, help='');
        parser.add_argument('--PROJECTION_PARAMETERS', type=str, help='Specify Nzone used for UTM projections');
        parser.add_argument('--BBOX', type=str, help='Spatial constraints(w,s,e,n)');
        parser.add_argument('--START', type=str, help='Start time for tmporal subset');
        parser.add_argument('--END', type=str, help='End time for temporal subset')
        parser.add_argument('--DEBUG', type=str, default='N', help='Debug mode, Y or N');
        parser.add_argument('--HEIGHT', type=str, help='Height in pixels of map picture, WCS 1.0.0 compatible parameter');
        parser.add_argument('--WIDTH', type=str, help='Width in pixels of map picture, WCS 1.0.0 compatiable parameter');
        parser.add_argument('--CRS', type=str, help='proj4 parameter string');
        parser.add_argument('--ORIGINALFILE', type=str, help='Original input file');
        parser.add_argument('--INCLUDE_META', type=str, help='Indicate whether to include metadata and processing history');
        parser.add_argument('--BOUNDINGSHAPE', type=str, help='GeoJSON content containing polygon information');
        self.args, unknown = parser.parse_known_args();
   
    def getInputfile(self):
        if self.args.FILE_URLS is not None:
            inputfile = self.args.FILE_URLS.replace("'", "");
            return inputfile;
    
    def getOutputdir(self):
        if self.args.OUTPUT_DIR is not None:
            outputdir = self.args.OUTPUT_DIR.replace("'", "");
            return outputdir;

    def getIdentifier(self):
        if self.args.IDENTIFIER is not None:
            identifier = self.args.IDENTIFIER.replace("'", "");
            return identifier;

    def getSubsetDataLayers(self):
        if self.args.SUBSET_DATA_LAYERS is not None:
            subset_data_layers = self.args.SUBSET_DATA_LAYERS.replace("'", "");
            return subset_data_layers;

    def getFormat(self):
        if self.args.FORMAT is not None:
            format = self.args.FORMAT.replace("'", "");
            return format;

    def getProjection(self):
        if self.args.PROJECTION is not None:
            projection = self.args.PROJECTION.replace("'", "");
            return projection;

    def getProjectionParameters(self):
        if self.args.PROJECTION_PARAMETERS is not None:
            projection_parameters = self.args.PROJECTION_PARAMETERS.replace("'", "");
            return projection_parameters;

    def getBbox(self):
        if self.args.BBOX is not None:
            bbox = self.args.BBOX.replace("'", "");
            return bbox;

    def getStart(self):
        if self.args.START is not None:
            start = self.args.START.replace("'", "");
            return start;

    def getEnd(self):
        if self.args.END is not None:
            end = self.args.END.replace("'", "");
            return end;

    def getDebug(self):
        if self.args.DEBUG is not None:
            debug = self.args.DEBUG.replace("'", "");
            return debug;

    def getHeight(self):
        if self.args.HEIGHT is not None:
            height = self.args.HEIGHT.replace("'", "");
            return height;

    def getWidth(self):
        if self.args.WIDTH is not None:
            width = self.args.WIDTH.replace("'", "");
            return width;

    def getCRS(self):
        if self.args.CRS is not None:
            crs = self.args.CRS.replace("'", "");
            return crs;

    def isMetadataIncluded(self):
        if self.args.INCLUDE_META is not None and self.args.INCLUDE_META.replace("'", "").upper() == "Y":
            return True;
        else:
            return False;

    def getBoundingShape(self):
        if self.args.BOUNDINGSHAPE is not None:
            boundingShape = self.args.BOUNDINGSHAPE;
            return boundingShape;
