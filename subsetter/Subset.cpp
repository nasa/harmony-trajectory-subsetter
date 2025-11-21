/*
 *  This program subsets an HDF5 file.
 */

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <time.h>

#include "Configuration.h"
#include "IcesatSubsetter.h"
#include "LogLevel.h"
#include "ProcessArguments.h"
#include "SubsetDataLayers.h"
#include "Subsetter.h"
#include "SuperGroupSubsetter.h"
#include "Temporal.h"
#include "geobox.h"

#include "H5Cpp.h"

/**
 * Trajectory Subsetter main function.
 */
int main(int argc, char *argv[])
{
    clock_t startTime, endTime;
    startTime = clock();

    // This is the error code returned by Subsetter::subset()
    // if an exception is not thrown.
    int ErrorCode = 0;

    try
    {
        // If process_args() returns a non-zero value,
        // then the arguments were unable to be processed.
        std::shared_ptr<ProcessArguments> processArgs =
            std::make_shared<ProcessArguments>();
        int processArgsErrorCode = processArgs->process_args(argc, argv);
        if (processArgsErrorCode != ProcessArguments::PASS)
        {
            return processArgsErrorCode;
        }

        std::string startString = processArgs->getStartString();
        std::string endString = processArgs->getEndString();
        std::string datasetList = processArgs->getDatasetList();
        std::vector<std::string> datasetsToInclude =
            processArgs->getDatasetsToInclude();
        boost::property_tree::ptree boundingShapePt =
            processArgs->getBoundingShapePt();
        std::vector<geobox> *geoboxes = processArgs->getGeoboxes();
        std::string infilename = processArgs->getInfilename();
        std::string outfilename = processArgs->getOutfilename();
        std::string outputFormat = processArgs->getOutputFormat();
        std::string collShortName = processArgs->getCollShortName();

        Configuration *config = new Configuration(processArgs->getConfigFile());

        // Data structure for requested datasets.
        SubsetDataLayers *subsetDataLayers;

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
            boost::tokenizer<boost::char_separator<char>> datasets(datasetList,
                                                                   delim);
            BOOST_FOREACH (dataset, datasets)
            {
                datasetsToInclude.push_back(dataset);
            }
            subsetDataLayers = new SubsetDataLayers(datasetsToInclude);
        }

        // Construct test to check if the input start and end parameters
        // are valid, if they are specified.
        Temporal *temporal = (!startString.empty() && !endString.empty())
                                 ? new Temporal(startString, endString)
                                 : NULL;

        // Construct data structure to hold bounding shape info, if specified.
        GeoPolygon *geoPolygon =
            (!boundingShapePt.empty()) ? new GeoPolygon(boundingShapePt) : NULL;

        // If a bounding shape is provided but the constructed GeoPolygon
        // polygon contains no data, return an error.
        if (geoPolygon != NULL and geoPolygon->isEmpty())
        {
            LOG_ERROR("Subset::main(): ERROR: no polygon found for the given "
                      "GeoJSON/KML/Shapefile");
            return 6;
        }

        // Extract the granule mission by passing the short name returned by
        // a Subsetter class function into a Configuration instance function.
        Subsetter *getMission = new Subsetter(subsetDataLayers,
                                              geoboxes,
                                              temporal,
                                              geoPolygon,
                                              config,
                                              outputFormat);
        H5::H5File infile = H5::H5File(infilename, H5F_ACC_RDONLY);

        std::string shortname = getMission->retrieveShortName(infile);

        if (shortname.empty() && !collShortName.empty())
        {
            shortname = collShortName;
            LOG_INFO("Subset::main(): shortname: " << shortname);
        }
        else if (shortname.empty() && collShortName.empty())
        {
            LOG_ERROR(
                "Subset::main(): ERROR: The short name could not be retrieved \
                        from the collection or was not defined in the command line arguments");
        }

        std::string mission = config->getMissionFromShortName(shortname);
        delete getMission;

        // Select which subsetter is needed for the mission.
        Subsetter *subsetter = nullptr;
        if (mission == "ICESAT")
        {
            subsetter = new IcesatSubsetter(
                subsetDataLayers, geoboxes, temporal, geoPolygon, config);
        }
        else if (mission == "GEDI")
        {
            subsetter = new SuperGroupSubsetter(
                subsetDataLayers, geoboxes, temporal, geoPolygon, config);
        }
        else // Use the base Subsetter if the mission isn't GEDI or ICESAT.
        {
            subsetter = new Subsetter(subsetDataLayers,
                                      geoboxes,
                                      temporal,
                                      geoPolygon,
                                      config,
                                      outputFormat);
        }
        ErrorCode = subsetter->subset(infilename, outfilename, shortname);
        if (ErrorCode == 0)
            LOG_INFO("Subset::main(): subset SUCCESS");
        else
            LOG_ERROR(
                "Subset::main(): subset FAILED return code: " << ErrorCode);

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

        std::stringstream argvStream;
        for (int i = 0; i < argc; i++)
        {
            argvStream << argv[i] << " ";
        }
        LOG_INFO(argvStream.str());

        endTime = clock();
        double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
        LOG_INFO(" execution time: " << runTime << " seconds");
    }
    catch (H5::Exception e)
    {
        std::string msg = e.getDetailMsg();
        LOG_ERROR("\nSubset::main(): ERROR: caught H5 Exception: " << msg);

        if (msg.find("H5Fcreate") != std::string::npos)
        {
            LOG_ERROR("Output file could not be created, check if it's "
                      << "currently open in another application.\n");
        }
        else if (msg.find("H5Gopen") != std::string::npos)
        {
            LOG_ERROR("HDF5 Group could not be opened.\n");
        }
        return -1;
    }
    catch (std::exception &e)
    {
        LOG_ERROR("\nSubset::main(): ERROR: caught std::exception "
                  << e.what());
        return -1;
    }
    catch (...)
    {
        LOG_ERROR("\nSubset::main(): ERROR: unknown exception occurred");
        return -1;
    }
    return ErrorCode;
}
