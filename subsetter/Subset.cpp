/*
*  This program subsets an HDF5 file.
*/

#include <time.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>

#include "ProcessArguments.h"
#include "Configuration.h"
#include "geobox.h"
#include "SubsetDataLayers.h"
#include "Subsetter.h"
#include "IcesatSubsetter.h"
#include "SuperGroupSubsetter.h"
#include "Temporal.h"
#include "LogLevel.h"


/**
 * Trajectory Subsetter main function.
 */
int main(int argc, char* argv[])
{
    // If process_args() returns a non-zero value,
    // then the arguments were unable to be processed.
    std::shared_ptr<ProcessArguments> processArgs = std::make_shared<ProcessArguments>();
    int processArgsErrorCode = processArgs->process_args(argc, argv);
    if (processArgsErrorCode != ProcessArguments::PASS)
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
        Configuration* config = new Configuration(processArgs->getConfigFile());

        // Data structure for requested datasets.
        SubsetDataLayers* subsetDataLayers;

        // Read in a json file if one is provided for the
        // requested datasets.
        if (processArgs->getDatasetList().find("json") != std::string::npos)
        {
            subsetDataLayers = new SubsetDataLayers(processArgs->getDatasetList());
        }
        // Otherwise, read in the datasets specified in the command
        // line request via --includedatasets.
        else
        {
            std::string dataset;
            boost::char_separator<char> delim(" ,");
            boost::tokenizer<boost::char_separator<char> > datasets(processArgs->getDatasetList(), delim);
            BOOST_FOREACH(dataset, datasets)
            {
                processArgs->getDatasetsToInclude().push_back(dataset);
            }
            subsetDataLayers = new SubsetDataLayers(processArgs->getDatasetsToInclude());
        }

        // Construct test to check if the input start and end parameters
        // are valid, if they are specified.
        Temporal* temporal = (!processArgs->getStartString().empty() && !processArgs->getEndString().empty())? 
                             new Temporal(processArgs->getStartString(), processArgs->getEndString()) : NULL;

        // Construct data structure to hold bounding shape info, if specified.
        GeoPolygon* geoPolygon = (!processArgs->getBoundingShapePt().empty())? 
                                 new GeoPolygon(processArgs->getBoundingShapePt()) : NULL;

        // If a bounding shape is provided but the constructed GeoPolygon
        // polygon contains no data, return an error.
        if (geoPolygon != NULL and geoPolygon->isEmpty())
        {
            LOG_ERROR("Subset::main(): ERROR: no polygon found for the given GeoJSON/KML/Shapefile");
            return 6;
        }

        // Extract the granule mission by passing the short name returned by
        // a Subsetter class function into a Configuration instance function.
        Subsetter* getMission = new Subsetter(subsetDataLayers, processArgs->getGeoboxes(),
                                              temporal, geoPolygon, config, processArgs->getOutputFormat());
        H5::H5File infile = H5::H5File(processArgs->getInfilename(),H5F_ACC_RDONLY);

        std::string shortname = getMission->retrieveShortName(infile);
        if(shortname.empty() && !processArgs->getCollShortName().empty())
        {
            shortname = processArgs->getCollShortName();
            LOG_INFO("Subset::main(): shortname: " << shortname);
        }
        else if (shortname.empty() && processArgs->getCollShortName().empty())
        {
            LOG_ERROR("Subset::main(): ERROR: The short name could not be retrieved \
                        from the collection or was not defined in the command line arguments");
        }

        std::string mission = config->getMissionFromShortName(shortname);
        delete getMission;

        // Select which subsetter is needed for the mission.
        Subsetter* subsetter = nullptr;
        if (mission == "ICESAT")
        {
            subsetter = new IcesatSubsetter(subsetDataLayers, processArgs->getGeoboxes(), temporal, geoPolygon, config);
        }
        else if (mission == "GEDI")
        {
            subsetter = new SuperGroupSubsetter(subsetDataLayers, processArgs->getGeoboxes(), temporal, geoPolygon, config);
        }
        else // Use the base Subsetter if the mission isn't GEDI or ICESAT.
        {
            subsetter = new Subsetter(subsetDataLayers, processArgs->getGeoboxes(), temporal, geoPolygon, config, processArgs->getOutputFormat());
        }
        ErrorCode = subsetter->subset(processArgs->getInfilename(), processArgs->getOutfilename(), shortname);
        if (ErrorCode == 0)
            LOG_INFO("Subset::main(): subset SUCCESS");
        else
            LOG_ERROR("Subset::main(): subset FAILED return code: " << ErrorCode);

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
            argvStream << argv[i] <<  " ";
        }
        LOG_INFO(argvStream.str());

        endTime=clock();
        double runTime = (double) (endTime - startTime) / CLOCKS_PER_SEC;
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
        LOG_ERROR("\nSubset::main(): ERROR: caught std::exception " << e.what());
        return -1;
    }
    catch (...)
    {
        LOG_ERROR("\nSubset::main(): ERROR: unknown exception occurred");
        return -1;
    }
    return ErrorCode;
 }

