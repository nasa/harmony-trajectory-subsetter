#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "H5Cpp.h"
#include <H5DataSpace.h>
#include <H5DataType.h>
#include <H5Exception.h>
using namespace H5;

//libgeotiff
#include "tiff.h"
#include "geotiffio.h"
#include "xtiffio.h"

//GCTP
extern "C" {
#ifdef SDPS
#include "HE5_GctpFunc.h"
#elif defined HARMONY
#include <gctp_prototypes.h>
#include <cproj.h>
#endif
}

#include <iostream>
#include <vector>
#include <map>
using namespace std;

#include <boost/lexical_cast.hpp>

#define N(a) (sizeof(a) / sizeof (a[0]))
#define TIFFTAG_UNITS 65000

    static const TIFFFieldInfo xtiffFieldInfo[] = {
         { TIFFTAG_UNITS, 1, 1, TIFF_ASCII, FIELD_CUSTOM, 0, 0, const_cast<char*>("units") }
    };

    static TIFFExtendProc parent_extender = NULL;

    static void registerCustomTIFFTags(TIFF *tif)
    {
       /* Install the extended Tag field info */
       TIFFMergeFieldInfo(tif, xtiffFieldInfo, N(xtiffFieldInfo));

       if (parent_extender)
          (*parent_extender)(tif);
    }

    static void augment_libtiff_with_custom_tags() {
        static bool first_time = true;
        if (!first_time) return;
        first_time = false;
        parent_extender = TIFFSetTagExtender(registerCustomTIFFTags);
    }




class geotiff_converter
{
public: 

    void convert(short* row_data, short* col_data, float* lat_dat, float* lon_data)
    {
        get_min_max(row_data, row_min, row_max);
        get_min_max(col_data, col_min, col_max);
        num_rows = row_max - row_min + 1;
        num_cols = col_max - col_min + 1;
    }
    
    geotiff_converter(string outfilename, string shortName, Group& outgroup, SubsetDataLayers* subsetDataLayers)
    {
        SubsetDataLayers* osub = new SubsetDataLayers(vector<string>());
        osub->expand_group(outgroup, "");
        vector<set<string>> datasets = osub->getDatasets();
        string tifFileName, datasetName, groupname, fileDatasetName;
        string rowName, colName, latName, lonName;
        short resolution;
        string projection;
        bool crop = false;
        string outputFormat = "GeoTIFF";

        for (vector<set<string>>::iterator set_it = datasets.begin(); set_it != datasets.end(); set_it++)
        {
            for (set<string>::iterator it = set_it->begin(); it != set_it->end(); it++)
            {
                datasetName = *it;
                cout << "converting " << datasetName << " to GeoTIFF" << endl;
                
                if (subsetDataLayers->is_dataset_included(datasetName))
                {
                    // generating tif file name
                    // replace '/' in the dataset name with '.'
                    groupname = datasetName.substr(0, datasetName.size()-1);
                    groupname = groupname.substr(0, groupname.find_last_of("/\\")+1);
                    //datasetName = datasetName.substr(1);
                    //std::replace(datasetName.minIndexStart(), datasetName.end(), '/', '.');
                    tifFileName = outfilename + "_";
                    fileDatasetName = datasetName.substr(1);
                    std::replace(fileDatasetName.begin(), fileDatasetName.end(), '/', '.');
                    //std::replace(tifFileName.find_last_of("/\\"), tifFileName.end(), '/', '.');
                    cout << "tifFileName: " << tifFileName << endl;
                    cout << "fileDatasetName: " << fileDatasetName << endl;
                    tifFileName += fileDatasetName + "tif";

                    // ToDo: need to refactor this to get lat/lon/row/col and resolution once for the whole group
                    //       instead of getting it for each dataset
                    Configuration::getInstance()->getRequiredDatasetsAndResolution(outputFormat, groupname, shortName, rowName, colName, latName, lonName, resolution);
                    if (rowName.find("/") == std::string::npos)
                    {
                        rowName = groupname + rowName;
                        colName = groupname + colName;
                        latName = groupname + latName;
                        lonName = groupname + lonName;
                    }
                    DataSet row = outgroup.openDataSet(rowName);
                    DataSet col = outgroup.openDataSet(colName);
                    DataSet lat = outgroup.openDataSet(latName);
                    DataSet lon = outgroup.openDataSet(lonName);
                    DataSet ds = outgroup.openDataSet(datasetName);
                    projection = Configuration::getInstance()->getProjection(groupname);
                    convert_to_geotiff(tifFileName.c_str(), "not used", row, col, ds, outputFormat.c_str(), projection.c_str(), resolution, crop, &lat, &lon);
                }
            }
        }
        
    }
    
    void convert_to_geotiff(const char* outfilename, const char* ds_name, DataSet& row, DataSet& col, DataSet& ds, 
        const char* outputformat, const char* projection, short resolution=36, bool crop = false, DataSet* lat=0, DataSet* lon=0)
    {
        vector<TIFF*> tifs;
        vector<GTIF*> gtifs;
        
        if (get_data_type(ds)==0 || get_size(row)==0)
        {
            cout << "Skipping " << outfilename << " because it is a type that can not be output to GeoTIFF or it has no matching data" << endl;
            return;  //no data
        }

        //get the number of output geotiff files
        int numfiles = get_numfiles(ds);

        for (int i=0;i<numfiles;i++)
        {
            string filename = outfilename;
            // multiple bands 
            if (numfiles > 1) filename = filename.erase(filename.length()-4) + "_" + boost::lexical_cast<string>(i+1) + ".tif";
            cout << "geotiff_converter processing : " << filename << endl;
            tifs.push_back(XTIFFOpen(filename.c_str(),"w"));
            gtifs.push_back(GTIFNew(tifs[i]));
            assert(gtifs.back());
        }
        hsize_t data_type_size = get_data_type_size(ds);
        void* fill_value = get_fill_value(ds, outputformat);
        void* off_earth_fill_value = get_off_earth_fill_value(ds, outputformat);

        // Tag the custom field as "ECS data units" along with the value
        string units = "ECS data units: " + get_units(ds);
        short* row_data = new short[get_size(row)];
        row.read(row_data, row.getDataType());
        short* col_data = new short[get_size(col)];
        col.read(col_data, col.getDataType());
        float* lat_data = new float[get_size(*lat)];
        lat->read(lat_data, lat->getDataType());

        convert(row_data, col_data, lat_data, 0);

        // set defaults for CEA projection
        double lon_pixel_size = 9000.5141/9.0;
        double lat_pixel_size = 9014.9864/9.0;

        short num_row_pixels = 406 * 36 / resolution;
        short num_col_pixels = 964 * 36 / resolution;
        
        // recalculate row/col for polar bands
        if (string(outfilename).find("Polar") != std::string::npos)
        {
            num_row_pixels = 500 * 36 / resolution;
            num_col_pixels = 500 * 36 / resolution;
        }
        
        double ul_lat_meters, ul_lon_meters, lr_lat_meters, lr_lon_meters;
#ifdef SDPS
        ceaforint(r_major, r_minor, 0, 30.*pi/180., 0., 0.);

        ceafor(-180.*pi/180., 85.0445664*pi/180., &ul_lon_meters, &ul_lat_meters);
        ceafor(180.*pi/180., -85.0445664*pi/180., &lr_lon_meters, &lr_lat_meters);
#endif
        lat_pixel_size = fabs((ul_lat_meters - lr_lat_meters) / num_row_pixels);
        lon_pixel_size = fabs((ul_lon_meters - lr_lon_meters) / num_col_pixels);

        if (!crop) //use full grid
        {
            num_rows = num_row_pixels;
            num_cols = num_col_pixels;
            row_min=0;
            col_min=0;
        }
        double geotiepoints[24] = {0.0, 0.0, 0.0,
                                   ul_lon_meters + col_min*lon_pixel_size,
                                   ul_lat_meters - row_min*lat_pixel_size, 0.0};
        double pixelscale[3] = {lon_pixel_size, lat_pixel_size, 0.0};

        laea=false;
        if (strcmp(projection, "GEO")==0) {
            //geographic projection is required for KML conversion

            assert(crop==false);
            map<short, double> lat_resolution_ratio; //number of rows in geographic projection vs. EASE-Grid2
            lat_resolution_ratio[36] = 586.0/406.0;
            lat_resolution_ratio[9] = 2382.0/1624.0;
            lat_resolution_ratio[3] = 7198.0/4872.0;

            num_rows = num_rows*lat_resolution_ratio[resolution];
            num_cols = col_max - col_min + 1;
            //cout << "Using num rows: " << num_rows << endl;
            lat_pixel_size = fabs((ul_lat_meters - lr_lat_meters) / num_rows);
            // Corner-Points of Geographic Projection are just the lon/lat values
            geotiepoints[3]=-180.0;
            geotiepoints[4]=85.0445664;
            pixelscale[0]=360.0/num_cols;
            pixelscale[1]=85.0445664*2.0/num_rows;

        }
        else if (strcmp(projection, "CEA")) {
            //not GEO, not CEA => LAEA projection

            // Corner points of laea projection, full polar grid are projection-meters,
            // equivalent to upper-center point for y and left-center point for x.
            // Corner point extents = faction of grid-extent based on row-min and
            // col-min, normalized to 500 row/col size for 36K resolution, scaled by
            // actual resolution, and relative to grid center point.
            // Using EASE-2 Standard Polar Grid - has constant corner-point value.
            laea=true;
            double grid_size = 18000000; // 18'000'000  18_000_000
            geotiepoints[3] = grid_size * (col_min/36.0*resolution-250) / 500;  //-9'000'000 for full grid
            geotiepoints[4] = -grid_size * (row_min/36.0*resolution-250) / 500; // 9'000'000 for full gridpy
            pixelscale[0] = pixelscale[1] = grid_size / (500*36.0/resolution);
            if (!crop)
            {
                num_rows = num_cols = 500 * 36 / resolution;
            }
        }

        for (int i=0;i < tifs.size() ;i++)
        {
            set_tiff_keys(tifs[i], data_type_size, get_data_type(ds), num_rows, num_cols, geotiepoints, pixelscale, units);
            set_geotiff_keys(gtifs[i], projection);
        }

        int** array_index_map = get_array_index_map(row_data, col_data);

        delete [] row_data;
        delete [] col_data;
        delete [] lat_data;
        //DataSpace space(ds.getSpace());
        //DataSpace outspace(dimnum, dims, maxdims);

        void* dataset_data = (void*)malloc(get_dataset_size(ds)*data_type_size);
        ds.read(dataset_data,ds.getDataType()); //), outspace, space);

        void* output_line = (void*)malloc(data_type_size*(num_cols));
        for (int i=0;i<num_rows;i++)
        {
            for (int l=0;l<numfiles;l++)
            {
                for (int j=0;j<num_cols;j++)
                {
                    if (laea &&
                        // normalize to 500 row/column grid size, row/column starts from 0-499
                        sqrt(pow((float)i/36.0*resolution-249.5, 2) + pow((float)j/36.0*resolution-249.5, 2)) > 250)
                    {
                        memcpy((char*)output_line + j*data_type_size, off_earth_fill_value, data_type_size);
                    }
                    else if (array_index_map[i][j] == coordinate_size)
                        memcpy((char*)output_line + j*data_type_size, fill_value, data_type_size);
                    else
                    {
                        void* val = ((char*)dataset_data + array_index_map[i][j] * data_type_size * tifs.size() + l*data_type_size);
                        memcpy((char*)output_line + j*data_type_size, val, data_type_size);
                    }
                }
                TIFFWriteScanline(tifs[l], output_line, i, 0);
            }
        }
        free(output_line);
        free(dataset_data);
        free_array_index_map(array_index_map);
        for (int i=0;i<numfiles;i++)
        {
            GTIFFree(gtifs[i]);
            XTIFFClose(tifs[i]);
        }
    }

    hsize_t get_size(DataSet& ds)
    {
        coordinate_size = 1;
        int ndims = ds.getSpace().getSimpleExtentNdims();
        hsize_t dims[ndims];
        ds.getSpace().getSimpleExtentDims(dims);
        while (ndims--) coordinate_size *= dims[ndims];
        return coordinate_size;
    }


    // fill values are dependent on the size of the datatype
    //   This fill value is used for off-earth points outside the swath
    void* get_off_earth_fill_value(DataSet& ds, const char* outputformat)
    {
        size_t size = ds.getDataType().getSize();
        void* buf = malloc(size);
        if (ds.attrExists("_FillValue"))
        {
            Attribute attr = ds.openAttribute("_FillValue");
            attr.read(attr.getDataType(), buf);
        } else
        {
            cout << "Fill Value not found in dataset setting to 0" << endl;
            memset(buf, 0, size);
        }

        if (strcmp(outputformat, "KML")==0) //for KML use the standard fill value
            return buf;

        // For GeoTIFF files we put fillvalue - 2 for
        //  the grid cells that are not defined in cell_row/cell_column
        if (size == 8)
        {
            double fill = -9997;
            memcpy(buf, &fill, size);
        }
        else if (size == 4)
        {
            float val = *(float*)buf;
            if (val == -9999 || val == 0) //is a float (val=0 means no fill (assume lat/lon))
            {
                float fill = -9997;
                memcpy(buf, &fill, size);
            }
            else if (val == 4294967294)
            {
                unsigned long fill = 4294967294-2;
                memcpy(buf, &fill, size);
            }
        }
        else if (size == 2)
        {
            unsigned short fill = 65532;
            memcpy(buf, &fill, size);
        }
        else if (size == 1)
        {
            unsigned char fill = 252;
            memcpy(buf, &fill, size);
        }
        else
        {
            cout << "Unknown data type size when calculating off earth fill value. Defaulting to float";
            float fill = -9997;
            memcpy(buf, &fill, size);
        }
        return buf;
    }

    // fill values are dependent on the size of the datatype
    //   This fill value is used for on-earth points outside the swath
    void* get_fill_value(DataSet& ds, const char* outputformat)
    {
        size_t size = ds.getDataType().getSize();
        void* buf = malloc(size);

        // Get Default Fill Value
        if (ds.attrExists("_FillValue"))
        {
            Attribute attr = ds.openAttribute("_FillValue");
            attr.read(attr.getDataType(), buf);
        }
        else if (size == 4) //cell_lat and cell_lon don't have _FillValue defined, so default to -9999
        {
            float fill = -9999;
            memcpy(buf, &fill, size);
        }


        if (strcmp(outputformat, "KML")==0) //for KML use the standard fill value
            return buf;

        // For GeoTIFF files we put fillvalue - 1 for
        //  the grid cells that are not defined in cell_row/cell_column

        if (size == 8)
        {
            double fill = -9998;
            memcpy(buf, &fill, size);
        }
        else if (size == 4)
        {
            if (*(float*)buf == -9999) //is a float
            {
                float fill = -9998;
                memcpy(buf, &fill, size);
            }
            else if (*(int32_t*)buf == 4294967294)
            {
                unsigned long fill = 4294967294-1;
                memcpy(buf, &fill, size);
            }
        }
        else if (size == 2)
        {
            unsigned short fill = 65533;
            memcpy(buf, &fill, size);
        }
        else if (size == 1)
        {
            unsigned char fill = 253;
            memcpy(buf, &fill, size);
        }
        else
        {
            cout << "Unknown data type size when calculating fill value defaulting to flaot";
            float fill = -9998;
            memcpy(buf, &fill, size);
        }
        return buf;
    }

    void get_min_max(short* data, int& min, int& max)
    {
        min = max = data[0];
        for (int i=1;i < coordinate_size;i++)
        {
            if (data[i] > max) max=data[i];
            if (data[i] < min) min=data[i];
        }
        return;
    }

    float get_float(DataSet& ds, hsize_t index)
    {
        hsize_t count[1];
        count[0]=1;
        hsize_t offset[1];
        offset[0]=index;

        DataSpace dataspace = ds.getSpace();
        dataspace.selectHyperslab( H5S_SELECT_SET, count, offset);
        hsize_t dimsm[1];
        dimsm[0]=get_size(ds);
        DataSpace memspace(1, dimsm);
        memspace.selectHyperslab(H5S_SELECT_SET, count, offset);

        float data_out[dimsm[0]]; //need to fix so we only need a single float
        ds.read(data_out, PredType::IEEE_F32LE, memspace, dataspace);
        return data_out[offset[0]];
    }
    int get_numfiles(DataSet& ds)
    {
        int numfiles = 1;
        int dimnum = ds.getSpace().getSimpleExtentNdims();
        hsize_t dims[dimnum];
        hsize_t maxdims[dimnum];
        ds.getSpace().getSimpleExtentDims(dims, maxdims);

        for (int k=1;k<dimnum;k++)
            numfiles *= dims[k];
        return numfiles;
    }
    hsize_t get_data_type_size(DataSet& ds)
    {
        hid_t mem_type_id = H5Dget_type(ds.getId());
        hid_t native_type = H5Tget_native_type(mem_type_id, H5T_DIR_DEFAULT);
        return H5Tget_size(native_type);
    }
    size_t get_dataset_size(DataSet& ds)
    {
        int dataset_size=1;
        int dimnum = ds.getSpace().getSimpleExtentNdims();
        hsize_t dims[dimnum];
        hsize_t maxdims[dimnum];
        ds.getSpace().getSimpleExtentDims(dims, maxdims);
        for (int i=0;i<dimnum;i++) dataset_size *= dims[i];
        return dataset_size;
    }
    int get_data_type(DataSet& ds)
    {
        hid_t mem_type_id = H5Dget_type(ds.getId());
        hid_t native_type = H5Tget_native_type(mem_type_id, H5T_DIR_DEFAULT);

        if (H5Tequal(native_type, H5T_NATIVE_UCHAR) ||
            H5Tequal(native_type, H5T_NATIVE_USHORT) ||
            H5Tequal(native_type, H5T_NATIVE_UINT) ||
            H5Tequal(native_type, H5T_NATIVE_ULONG))
        {
            return 1;
        }
        else
        if (H5Tequal(native_type, H5T_NATIVE_CHAR) ||
            H5Tequal(native_type, H5T_NATIVE_SHORT) ||
            H5Tequal(native_type, H5T_NATIVE_INT) ||
            H5Tequal(native_type, H5T_NATIVE_LONG))
        {
            return 2;
        }
        else
        if (H5Tequal(native_type, H5T_NATIVE_FLOAT) ||
            H5Tequal(native_type, H5T_NATIVE_DOUBLE))
        {
            return 3;
        }
        else
        {
            assert("invalid type for dataset to convert");
            return 0;
        }
    }

    string get_units(DataSet& ds)
    {
        string buf;
        if (ds.attrExists("units"))
        {
            Attribute attr = ds.openAttribute("units");
            attr.read(attr.getDataType(), buf);
            cout << "Attribute units exists:" << buf << endl;
            return buf;
        }
    }

    void set_tiff_keys(TIFF* tif, size_t data_type_size, int data_type, int num_rows, int num_cols, double* geotiepoints, double* pixelscale, string units)
    {
        TIFFSetField(tif, TIFFTAG_SOFTWARE,  "EED custom geotiff conversion for SMAP version 1.0");
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, data_type_size*8);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, data_type);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, num_rows);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1); //should make it so we have 8K strips
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, num_cols);
        TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, geotiepoints);
        TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixelscale);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
//        TIFFSetField(tif, TIFFTAG_GDAL_NODATA, ); //=42113
        augment_libtiff_with_custom_tags();

        registerCustomTIFFTags(tif);;
        TIFFSetField(tif, TIFFTAG_UNITS, (const char*) units.c_str());
    }
    void set_geotiff_keys(GTIF* gtif, const char* projection)
    {
        if (strcmp(projection, "GEO")==0)
        {
            GTIFKeySet(gtif, GTModelTypeGeoKey, TYPE_SHORT, 1, ModelTypeGeographic);
            GTIFKeySet(gtif, GTCitationGeoKey, TYPE_ASCII, 0, "Geographic (WGS84)");
            GTIFKeySet(gtif, GeogSemiMajorAxisGeoKey, TYPE_DOUBLE, 1, r_major);
            GTIFKeySet(gtif, GeogSemiMinorAxisGeoKey, TYPE_DOUBLE, 1, r_minor);

            GTIFKeySet(gtif, GeogGeodeticDatumGeoKey, TYPE_SHORT, 1, DatumE_WGS84);
            GTIFKeySet(gtif, GeographicTypeGeoKey, TYPE_SHORT, 1, GCS_WGS_84);
            GTIFKeySet(gtif, GeogEllipsoidGeoKey, TYPE_SHORT, 1, Ellipse_WGS_84);

            GTIFKeySet(gtif, GeogAngularUnitsGeoKey, TYPE_SHORT, 1, Angular_Degree);
            GTIFWriteKeys(gtif);
            return;
        }

        GTIFKeySet(gtif, GTModelTypeGeoKey, TYPE_SHORT, 1, ModelTypeProjected);
        GTIFKeySet(gtif, GeogSemiMajorAxisGeoKey, TYPE_DOUBLE, 1, r_major);
        GTIFKeySet(gtif, GeogSemiMinorAxisGeoKey, TYPE_DOUBLE, 1, r_minor);

        GTIFKeySet(gtif, GeogGeodeticDatumGeoKey, TYPE_SHORT, 1, Datum_WGS84);
        GTIFKeySet(gtif, GeographicTypeGeoKey, TYPE_SHORT, 1, GCS_WGS_84);
        GTIFKeySet(gtif, GeogEllipsoidGeoKey, TYPE_SHORT, 1, Ellipse_WGS_84);

        GTIFKeySet(gtif, GeogLinearUnitsGeoKey, TYPE_SHORT, 1, Linear_Meter);
        GTIFKeySet(gtif, GeogAngularUnitsGeoKey, TYPE_SHORT, 1, Angular_Degree);

        GTIFKeySet(gtif, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, KvUserDefined);
        GTIFKeySet(gtif, ProjectionGeoKey, TYPE_SHORT, 1, KvUserDefined);
        GTIFKeySet(gtif, ProjLinearUnitsGeoKey, TYPE_SHORT, 1, Linear_Meter);
        GTIFKeySet(gtif, ProjLinearUnitSizeGeoKey, TYPE_DOUBLE, 1, 1.0);

        if (!strcmp(projection, "CEA"))
        {
            GTIFKeySet(gtif, GTRasterTypeGeoKey, TYPE_SHORT, 1, RasterPixelIsArea);
            GTIFKeySet(gtif, ProjCoordTransGeoKey, TYPE_SHORT, 1, CT_CylindricalEqualArea);
            GTIFKeySet(gtif, GTCitationGeoKey, TYPE_ASCII, 0, "Cylindrical Equal Area (WGS84)");
            GTIFKeySet(gtif, ProjStdParallelGeoKey, TYPE_DOUBLE, 1, 30.0);
        }
        else
        {
            GTIFKeySet(gtif, GTRasterTypeGeoKey, TYPE_SHORT, 1, RasterPixelIsArea);
            GTIFKeySet(gtif, ProjCoordTransGeoKey, TYPE_SHORT, 1, CT_LambertAzimEqualArea);
            GTIFKeySet(gtif, ProjCenterLongGeoKey, TYPE_DOUBLE, 1, 0.0);
            if (!strcmp(projection, "NLAEA"))
            {
                GTIFKeySet(gtif, GTCitationGeoKey, TYPE_ASCII, 0, "Lambert Azimuthal Equal Area - North Polar Projection - (WGS84)");
                GTIFKeySet(gtif, ProjCenterLatGeoKey, TYPE_DOUBLE, 1, 90.0);
                GTIFKeySet(gtif, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1, 0.0);
                GTIFKeySet(gtif, ProjFalseOriginLongGeoKey, TYPE_DOUBLE, 1, 0.0);
            }
            else
            {
                GTIFKeySet(gtif, GTCitationGeoKey, TYPE_ASCII, 0, "Lambert Azimuthal Equal Area - South Polar Projection - (WGS84)");
                GTIFKeySet(gtif, ProjCenterLatGeoKey, TYPE_DOUBLE, 1, -90.0);
                GTIFKeySet(gtif, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1, 0.0);
                GTIFKeySet(gtif, ProjFalseOriginLongGeoKey, TYPE_DOUBLE, 1, 0.0);
            }
        }
        GTIFWriteKeys(gtif);
    }

    int**  get_array_index_map(short* row_data, short* col_data)
    {
        int** array_index_map = new int*[num_rows];
        for (int i=0;i<num_rows;i++)
        {
            array_index_map[i]=new int[num_cols];
            for (int j=0;j<num_cols;j++)
                array_index_map[i][j] = coordinate_size;
        }
        for (int i=0;i<coordinate_size;i++)
        {
            if ((row_data[i] >= row_min && col_data[i] >= col_min) &&
                (row_data[i] - row_min <= row_max && col_data[i] - col_min <= col_max))
            {
                //cout << row_data[i] << "," << row_min << ", " << col_data[i] << ", " << col_min << endl;
                array_index_map[row_data[i]-row_min][col_data[i]-col_min] = i;
            }
        }
        return array_index_map;
    }
    void free_array_index_map(int** array_index_map)
    {
        for (int i=0;i<num_rows;i++) delete [] array_index_map[i];
        delete [] array_index_map;
    }    
    protected:
        /*vector<TIFF*> tifs;
        vector<GTIF*> gtifs;*/
        int coordinate_size;
        int row_min;
        int row_max;
        int col_min;
        int col_max;
        int num_rows;
        int num_cols;
        bool laea;
        static constexpr double r_major = 6378137.;
        static constexpr double r_minor = 6356752.31424;
        static constexpr double pi = 3.141592653589793;  
};

constexpr double geotiff_converter::r_major; 

