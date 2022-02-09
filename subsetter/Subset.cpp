#include <time.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

#include "Configuration.h"
#include "geobox.h"
#include "SubsetDataLayers.h"
#include "Subsetter.h"
#include "IcesatSubsetter.h"
#include "SuperGroupSubsetter.h"
#include "Temporal.h"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
using namespace boost;

// This program subsets HDF5 file

string infilename;
string outfilename;
string configFile; 
string subsettype;
string bounding_box;
string startString;
string endString;
string boundingShape;
string originalOutputFormat;
string outputFormat;
string datasetList;
bool reproject;

int ErrorCode = 0;  //error code to be returned to shell
vector<geobox>* geoboxes = NULL; // multiple bounding boxes can be specified allow any data points that are in any of the them
vector<string> datasetsToInclude;
pt::ptree boundingShapePt;

int process_args(int argc, char* argv[])
{
    po::options_description description("Available Options");
    description.add_options()
            ("help,h", "Display this help message")
            ("filename,f", po::value<std::string>(), "Name of file to subset")
            ("outfile,o", po::value<std::string>(), "Name of output file")
            ("configfile,c", po::value<std::string>(), "Configure file")
            ("subsettype,t", po::value<std::string>(), "Subset type (ICESAT, SMAP, GLAS)")
            ("bbox,b", po::value<vector<string> >(), "Bounding Boxes (West,South,East,North) degrees")
            ("start,s", po::value<std::string>(), "Temporal search start")
            ("end,e", po::value<std::string>(), "Temporal search end")
            ("includedataset,i", po::value<std::string>(), "Only include the specified datasets to include in output product")
            ("boundingshape,p", po::value<string>(), "Bounding shape(polygon)")
            ("reformat,r", po::value<string>(), "Change the output format (-r GeoTIFF)")
            ("crs,j", po::value<string>(), "Reproject to the coordinate reference system (e.g. EPSG:4326");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
    po::notify(vm);

    if (vm.count("help") || !vm.count("filename"))
    {
        cout << description;
        return 2;
    }

    infilename = vm["filename"].as<std::string>();
    cout << "filename: " << infilename << endl;
    if (!ifstream(infilename.c_str()))
    {
        cout << "ERROR: Could not open input file " << infilename << endl;
        return 1;
    }

    if (vm.count("outfile")) outfilename = vm["outfile"].as<std::string>();
    if (outfilename.find("--") == 0 || !ofstream(outfilename.c_str()))
    {
        cout << "ERROR: Could not open output file " << outfilename << endl;
        return 1;
    }

    subsettype = (vm.count("subsettype"))? vm["subsettype"].as<std::string>() : "";
    configFile = (vm.count("configfile"))? vm["configfile"].as<std::string>() : "";

    match_results<std::string::const_iterator> regex_results;
    if (vm.count("bbox"))
    {
        vector<string> boxes = vm["bbox"].as<vector<string> >();
        regex bbox_format("[']?[-]?[.\\d]+,[-]?[.\\d]+,[-]?[.\\d]+,[-]?[.\\d]+[']?");

        for (vector<string>::iterator it = boxes.begin(); it != boxes.end(); it++)
        {
            if (!regex_match((*it), regex_results, bbox_format))
            {
                cout << "ERROR: Invalid bounding box: " << *it << endl;
                return 1;
            }

            cout << "Adding bounding box " << *it << endl;
            bounding_box += *it;
            char_separator<char> sep(",");
            tokenizer<char_separator<char> > tokens(*it, sep);
            tokenizer<char_separator<char> >::iterator iter = tokens.begin();
            double w = atof(iter->c_str());
            double s = atof((++iter)->c_str());
            double e = atof((++iter)->c_str());
            double n = atof((++iter)->c_str());
            if (geoboxes == NULL)
            {
                geoboxes = new vector<geobox>();
            }
            geoboxes->push_back(geobox(w, s, e, n));
        }
    }

    // expect to have both start and end parameters
    if (vm.count("start") && vm.count("end"))
    {
        startString = vm["start"].as<std::string>();
        endString = vm["end"].as<std::string>();
        //regex date_format("[']?[\\d]{4}-[\\d]{2}-[\\d]{2}T[\\d]{2}:[\\d]{2}:[\\d]{2}[']?[\\.[\\d]*]?");
        regex date_format("[']?[\\d]{4}-[\\d]{2}-[\\d]{2}(T[\\d]{2}:[\\d]{2}:[\\d]{2}[\\.[\\d]*]?)?[']?");
        if (!regex_match(startString, regex_results, date_format) || !regex_match(endString, regex_results, date_format))
        {
            cout << "ERROR: Invalid start or end parameter " << endl;
            return 1;
        }
    }
    else if (vm.count("start") || vm.count("end"))
    {
        cout << "ERROR: Invalid temporal parameters, must pass in both --start and --end parameters" << endl;
        return 1;
    }
    
    if (vm.count("includedataset"))
    {
        datasetList = vm["includedataset"].as<std::string>();
        cout << "includedataset" << endl;
    }

    if (vm.count("boundingshape"))
    {
        boundingShape = vm["boundingshape"].as<std::string>();
        if (boundingShape.find("geojson") != string::npos)
        {
            pt::read_json(boundingShape, boundingShapePt);
        }
        else
        {
            std::stringstream ss;
            ss << boundingShape;
            pt::read_json(ss, boundingShapePt);
        }
    }
    
    if (vm.count("reformat"))
    {
        cout << "reformat" << endl;
        originalOutputFormat = outputFormat = vm["reformat"].as<std::string>();
        if (outputFormat == "GeoTIFF" || outputFormat == "GTiff" || outputFormat == "GEO" || outputFormat=="KML")
            outputFormat = "GeoTIFF";
        else if (outputFormat == "netCDF3" || outputFormat == "NetCDF3" || outputFormat == "NetCDF-3") outputFormat = "NetCDF-3";
        cout << "Reformatting to " << originalOutputFormat << endl;
    }
    
    if (vm.count("crs")) reproject=true;
    else reproject=false;

    return 0;
}

int main(int argc, char* argv[])
{
    if (ErrorCode = process_args(argc, argv)) return ErrorCode;
    clock_t startTime, endTime;
    startTime=clock();
    
    int ErrorCode = 0;

    try
    {
        // initialize the configuration
        if (!configFile.empty())
        {
            Configuration* config = Configuration::getInstance();
            config->initialize(configFile);
        }

        // subset criteria
        SubsetDataLayers* subsetDataLayers;
        if (datasetList.find("json") != string::npos)
        {
            subsetDataLayers = new SubsetDataLayers(datasetList);
        }
        else
        {
            string dataset;
            char_separator<char> delim(" ,");
            tokenizer<char_separator<char> > datasets(datasetList, delim);
            BOOST_FOREACH(dataset, datasets)
            {
                datasetsToInclude.push_back(dataset);
            }
            subsetDataLayers = new SubsetDataLayers(datasetsToInclude);
        }
        Temporal* temporal = (!startString.empty() && !endString.empty())? new Temporal(startString, endString) : NULL;
        GeoPolygon* geoPolygon = (!boundingShapePt.empty())? new GeoPolygon(boundingShapePt) : NULL;
        // if no polygon found
        if (geoPolygon != NULL and geoPolygon->isEmpty())
        {
            cout << "ERROR: no polygon found for the given GeoJSON/KML/Shapefile" << endl;
            ErrorCode = 6;
            return ErrorCode;
        }
        Subsetter* g = new Subsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, outputFormat);
        H5File infile = H5File(infilename,H5F_ACC_RDONLY);
        string shortname = g->retrieveShortName(infile);
        string mission = Configuration::getInstance()->getMissionFromShortName(shortname);
        if (mission == "ICESAT")
        {
            g = new IcesatSubsetter(subsetDataLayers, geoboxes, temporal, geoPolygon);
        }
        else if (mission == "GEDI")
        {
            g = new SuperGroupSubsetter(subsetDataLayers, geoboxes, temporal, geoPolygon);
        }
        ErrorCode = g->subset(infilename, outfilename);
        
        delete subsetDataLayers;
        if (temporal != NULL) delete temporal;
        if (geoPolygon != NULL) delete geoPolygon;
        delete g;        
        Configuration::destroyInstance();

        endTime=clock();
        double runTime = (double) (endTime - startTime) / CLOCKS_PER_SEC;
        for (int i = 0; i < argc; i++) cout << argv[i] <<  " ";
        cout << " execution time: " << runTime << " seconds" << endl;
    }
    catch (Exception e)
    {
        cerr << "ERROR: caught H5 Exception " << e.getDetailMsg() << endl;
        e.printErrorStack();
        ErrorCode = -1;
    }
    catch (std::exception &e)
    {
        cerr << "ERROR: caught std::exception " << e.what() << endl;
        ErrorCode = -1;
    }
    catch (...)
    {
        cerr << "ERROR: unknown exception occurred";
        ErrorCode = -1;
    }
    return ErrorCode;
}


