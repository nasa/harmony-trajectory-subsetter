/*
*  This program subsets an HDF5 file.
*/

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
#include <boost/algorithm/string.hpp>

#include "Configuration.h"
#include "geobox.h"
#include "SubsetDataLayers.h"
#include "Subsetter.h"
#include "IcesatSubsetter.h"
#include "SuperGroupSubsetter.h"
#include "Temporal.h"

namespace program_options = boost::program_options;
namespace property_tree = boost::property_tree;

std::string infilename;
std::string outfilename;
std::string configFile;
std::string subsettype;
std::string bounding_box;
std::string startString;
std::string endString;
std::string boundingShape;
std::string originalOutputFormat;
std::string outputFormat;
std::string datasetList;
std::string collShortName;
bool reproject;

std::vector<geobox>* geoboxes = NULL; // Multiple bounding boxes can be specified.
std::vector<std::string> datasetsToInclude;
property_tree::ptree boundingShapePt;

/**
 * @brief Process the input arguments of the subset request.
 *
 * @param argc The number of input arguments.
 * @param argv The vector of input arguments.
 * @return Error code (0 - success, 1 - fail)
 */
int process_args(int argc, char* argv[])
{
    program_options::options_description description("Available Options");
    description.add_options()
            ("help,h", "Display this help message")
            ("filename,f", program_options::value<std::string>(), "Name of file to subset")
            ("outfile,o", program_options::value<std::string>(), "Name of output file")
            ("configfile,c", program_options::value<std::string>(), "Configure file")
            ("subsettype,t", program_options::value<std::string>(), "Subset type (ICESAT, SMAP, GLAS)")
            ("bbox,b", program_options::value<std::vector<std::string> >(), "Bounding Boxes (West,South,East,North) degrees")
            ("start,s", program_options::value<std::string>(), "Temporal search start")
            ("end,e", program_options::value<std::string>(), "Temporal search end")
            ("includedataset,i", program_options::value<std::string>(), "Only include the specified datasets to include in output product")
            ("boundingshape,p", program_options::value<std::string>(), "Bounding shape(polygon)")
            ("reformat,r", program_options::value<std::string>(), "Change the output format (-r GeoTIFF)")
            ("crs,j", program_options::value<std::string>(), "Reproject to the coordinate reference system (e.g. EPSG:4326")
            ("shortname,n", program_options::value<std::string>(), "The collection shortName for granules that do not contain a shortName variable (ATL24)");

    program_options::variables_map variables_map;
    program_options::store(program_options::command_line_parser(argc, argv).options(description).run(), variables_map);
    program_options::notify(variables_map);

    // Print out the defined command options when the user either types
    // "help" or does not include a filename.
    if (variables_map.count("help") || !variables_map.count("filename"))
    {
        std::cout << description;
        return 2;
    }

    // Access filename from the input command, if specified.
    infilename = variables_map["filename"].as<std::string>();
    std::cout << "Subset::process_args(): filename: " << infilename << std::endl;
    if (!std::ifstream(infilename.c_str()))
    {
        std::cout << "Subset::process_args(): ERROR: Could not open input file " << infilename << std::endl;
        return 1;
    }

    // Access output file from the input command, if specfied.
    if (variables_map.count("outfile")) outfilename = variables_map["outfile"].as<std::string>();
    if (outfilename.find("--") == 0 || !std::ofstream(outfilename.c_str()))
    {
        std::cout << "Subset::process_args(): ERROR: Could not open output file " << outfilename << std::endl;
        return 1;
    }

    // If either the subset type and/or config file are specified, pull them from the
    // variable map, otherwise assign these variables to an empty string.
    subsettype = (variables_map.count("subsettype"))? variables_map["subsettype"].as<std::string>() : "";
    configFile = (variables_map.count("configfile"))? variables_map["configfile"].as<std::string>() : "";

    // Access bounding box from the input command, if specified.
    boost::match_results<std::string::const_iterator> regex_results;
    if (variables_map.count("bbox"))
    {
        std::vector<std::string> boxes = variables_map["bbox"].as<std::vector<std::string> >();
        boost::regex bbox_format("[']?[-]?[.\\d]+,[-]?[.\\d]+,[-]?[.\\d]+,[-]?[.\\d]+[']?");

        for (std::vector<std::string>::iterator it = boxes.begin(); it != boxes.end(); it++)
        {
            if (!regex_match((*it), regex_results, bbox_format))
            {
                std::cout << "Subset::process_args(): ERROR: Invalid bounding box: " << *it << std::endl;
                return 1;
            }

            std::cout << "Subset::process_args(): Adding bounding box " << *it << std::endl;
            bounding_box += *it;
            boost::char_separator<char> sep(",");
            boost::tokenizer<boost::char_separator<char> > tokens(*it, sep);
            boost::tokenizer<boost::char_separator<char> >::iterator iter = tokens.begin();
            double w = atof(iter->c_str());
            double s = atof((++iter)->c_str());
            double e = atof((++iter)->c_str());
            double n = atof((++iter)->c_str());
            if (geoboxes == NULL)
            {
                geoboxes = new std::vector<geobox>();
            }
            geoboxes->push_back(geobox(w, s, e, n));
        }
    }

    // Access start and end temporal parameters, if specified.
    // Either both or none of the parameteres are expected when included.
    if (variables_map.count("start") && variables_map.count("end"))
    {
        startString = variables_map["start"].as<std::string>();
        endString = variables_map["end"].as<std::string>();
        boost::erase_all(startString, "'");
        boost::erase_all(endString, "'");
        boost::regex date_format("[']?[\\d]{4}-[\\d]{2}-[\\d]{2}(T[\\d]{2}:[\\d]{2}:[\\d]{2}[\\.[\\d]*]?)?[']?");
        if (!regex_match(startString, regex_results, date_format) || !regex_match(endString, regex_results, date_format))
        {
            std::cout << "Subset::process_args(): ERROR: Invalid start or end parameter " << std::endl;
            return 1;
        }
    }
    else if (variables_map.count("start") || variables_map.count("end"))
    {
        std::cout << "Subset::process_args(): ERROR: Invalid temporal parameters, must pass in "
                  << "both --start and --end parameters" << std::endl;
        return 1;
    }

    // Access included dataset(s), if specified.
    if (variables_map.count("includedataset"))
    {
        datasetList = variables_map["includedataset"].as<std::string>();
        std::cout << "Subset::process_args(): includedataset" << std::endl;
    }

    // Access bounding shape, if specfied.
    if (variables_map.count("boundingshape"))
    {
        boundingShape = variables_map["boundingshape"].as<std::string>();
        if (boundingShape.find("geojson") != std::string::npos)
        {
            property_tree::read_json(boundingShape, boundingShapePt);
        }
        else
        {
            std::stringstream ss;
            ss << boundingShape;
            property_tree::read_json(ss, boundingShapePt);
        }
    }

    // Access reformatting, if specified.
    if (variables_map.count("reformat"))
    {
        std::cout << "Subset::process_args(): reformat" << std::endl;
        originalOutputFormat = outputFormat = variables_map["reformat"].as<std::string>();
        if (outputFormat == "GeoTIFF" || outputFormat == "GTiff" || outputFormat == "GEO" || outputFormat=="KML")
        {
            outputFormat = "GeoTIFF";
        }
        else if (outputFormat == "netCDF3" || outputFormat == "NetCDF3" || outputFormat == "NetCDF-3")
        {
            outputFormat = "NetCDF-3";
        }
        std::cout << "Subset::process_args(): Reformatting to " << originalOutputFormat << std::endl;
    }

    // Access coordinate reference system / reprojection, if specified.
    if (variables_map.count("crs"))
    {
        reproject=true;
    }
    else
    {
        reproject=false;
    }

    // Access shortname from the input command, otherwise assign these variables to an empty string
    collShortName = (variables_map.count("shortname"))? variables_map["shortname"].as<std::string>() : "";
    boost::trim_right(collShortName);

    return 0;
}

/**
 * Trajectory Subsetter main function.
 */
int main(int argc, char* argv[])
{
    // If process_args() returns a non-zero value,
    // then the arguments were unable to be processed.
    if (int processArgsErrorCode = process_args(argc, argv))
    {
        return processArgsErrorCode;
    }
    clock_t startTime, endTime;
    startTime=clock();

    // This is the error code returned by Subsetter::subset()
    // if an exception is not thrown.
    int ErrorCode = 0;

    try
    {
        Configuration* config = new Configuration(configFile);

        // Data structure for requested datasets.
        SubsetDataLayers* subsetDataLayers;

        // Read in a json file if one is provided for the
        // requested datasets.
        if (datasetList.find("json") != std::string::npos)
        {
            subsetDataLayers = new SubsetDataLayers(datasetList);
        }
        // Otherwise, read in the datasets specified in the command
        // line request via --includedatasets.
        else
        {
            std::string dataset;
            boost::char_separator<char> delim(" ,");
            boost::tokenizer<boost::char_separator<char> > datasets(datasetList, delim);
            BOOST_FOREACH(dataset, datasets)
            {
                datasetsToInclude.push_back(dataset);
            }
            subsetDataLayers = new SubsetDataLayers(datasetsToInclude);
        }

        // Construct test to check if the input start and end parameters
        // are valid, if they are specified.
        Temporal* temporal = (!startString.empty() && !endString.empty())? new Temporal(startString, endString) : NULL;

        // Construct data structure to hold bounding shape info, if specified.
        GeoPolygon* geoPolygon = (!boundingShapePt.empty())? new GeoPolygon(boundingShapePt) : NULL;

        // If a bounding shape is provided but the constructed GeoPolygon
        // polygon contains no data, return an error.
        if (geoPolygon != NULL and geoPolygon->isEmpty())
        {
            std::cout << "Subset::main(): ERROR: no polygon found for the given GeoJSON/KML/Shapefile" << std::endl;
            return 6;
        }

        // Extract the granule mission by passing the short name returned by
        // a Subsetter class function into a Configuration instance function.
        Subsetter* getMission = new Subsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, config, outputFormat);
        H5::H5File infile = H5::H5File(infilename,H5F_ACC_RDONLY);

        std::string shortname = getMission->retrieveShortName(infile);
        if(shortname.empty() && !collShortName.empty())
        {
            shortname = collShortName;
            std::cout << "Subset::main(): shortname: " << shortname << std::endl;
        }
        else if (shortname.empty() && collShortName.empty())
        {
            std::cout << "Subset::main(): ERROR: The short name could not be retrieved \
                        from the collection or was not defined in the command line arguments" 
            << std::endl;
        }

        std::string mission = config->getMissionFromShortName(shortname);
        delete getMission;

        // Select which subsetter is needed for the mission.
        Subsetter* subsetter = nullptr;
        if (mission == "ICESAT")
        {
            subsetter = new IcesatSubsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, config);
        }
        else if (mission == "GEDI")
        {
            subsetter = new SuperGroupSubsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, config);
        }
        else // Use the base Subsetter if the mission isn't GEDI or ICESAT.
        {
            subsetter = new Subsetter(subsetDataLayers, geoboxes, temporal, geoPolygon, config, outputFormat);
        }
        ErrorCode = subsetter->subset(infilename, outfilename, shortname);

        // Release dynamic memory.
        delete config;
        delete subsetDataLayers;
        if (temporal != NULL)
        {
            delete temporal;
        }
        if (geoPolygon != NULL)
        {
           delete geoPolygon;
        }
        delete subsetter;

        endTime=clock();
        double runTime = (double) (endTime - startTime) / CLOCKS_PER_SEC;
        for (int i = 0; i < argc; i++)
        {
            std::cout << argv[i] <<  " ";
        }
        std::cout << " execution time: " << runTime << " seconds" << std::endl;
    }
    catch (H5::Exception e)
    {
        std::string msg = e.getDetailMsg();
        std::cerr << "\nSubset::main(): ERROR: caught H5 Exception: " << msg << std::endl;

        if (msg.find("H5Fcreate") != std::string::npos)
        {
            std::cerr << "Output file could not be created, check if it's "
                      << "currently open in another application.\n" << std::endl;
        }
        else if (msg.find("H5Gopen") != std::string::npos)
        {
            std::cerr << "HDF5 Group could not be opened.\n" << std::endl;
        }
        return -1;
    }
    catch (std::exception &e)
    {
        std::cerr << "\nSubset::main(): ERROR: caught std::exception " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "\nSubset::main(): ERROR: unknown exception occurred";
        return -1;
    }
    return ErrorCode;
 }




