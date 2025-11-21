#include "ProcessArguments.h"

/**
 * @brief Process the input arguments of the subset request.
 *
 * @param argc The number of input arguments.
 * @param argv The vector of input arguments.
 * @return Error code (0 - success, 1 - error, 2 - show help or no filename)
 */
int ProcessArguments::process_args(int argc, char *argv[])
{
    program_options::options_description description("Available Options");
    description.add_options()("help,h", "Display this help message")(
        "filename,f",
        program_options::value<std::string>(),
        "Name of file to subset")("outfile,o",
                                  program_options::value<std::string>(),
                                  "Name of output file")(
        "configfile,c",
        program_options::value<std::string>(),
        "Configure file")("subsettype,t",
                          program_options::value<std::string>(),
                          "Subset type (ICESAT, SMAP, GLAS)")(
        "bbox,b",
        program_options::value<std::vector<std::string>>(),
        "Bounding Boxes (West,South,East,North) degrees")(
        "start,s",
        program_options::value<std::string>(),
        "Temporal search start")(
        "end,e", program_options::value<std::string>(), "Temporal search end")(
        "includedataset,i",
        program_options::value<std::string>(),
        "Only include the specified datasets to include in output product")(
        "boundingshape,p",
        program_options::value<std::string>(),
        "Bounding shape(polygon) or .geojson file")(
        "reformat,r",
        program_options::value<std::string>(),
        "Change the output format (-r GeoTIFF)")(
        "crs,j",
        program_options::value<std::string>(),
        "Reproject to the coordinate reference system (e.g. EPSG:4326")(
        "shortname,n",
        program_options::value<std::string>(),
        "The collection shortName for granules that do not contain a shortName "
        "variable (ATL24)")(
        "loglevel,l",
        program_options::value<std::string>(),
        "The log level can be DEBUG, INFO, WARNING, ERROR, or CRITICAL)")(
        "logfile,g",
        program_options::value<std::string>(),
        "Name of log output file");

    program_options::variables_map variables_map;
    program_options::store(program_options::command_line_parser(argc, argv)
                               .options(description)
                               .run(),
                           variables_map);
    program_options::notify(variables_map);

    setLogLevel(variables_map);

    if (showHelpVerifyFilename(description, variables_map) ==
        SHOW_HELP_OR_NO_FILENAME)
        return SHOW_HELP_OR_NO_FILENAME;
    if (setInFileName(variables_map) == ERROR)
        return ERROR;
    if (setOutFileName(variables_map) == ERROR)
        return ERROR;
    if (setBoundingBox(variables_map) == ERROR)
        return ERROR;
    if (setStartEndTemporalParameters(variables_map) == ERROR)
        return ERROR;
    if (setBoundingShape(variables_map) == ERROR)
        return ERROR;

    setSubsettype(variables_map);
    setConfigFile(variables_map);
    setDatasetList(variables_map);
    setReformat(variables_map);
    setCRS(variables_map);
    setCollectionShortname(variables_map);

    return PASS;
}

void ProcessArguments::setLogLevel(program_options::variables_map variables_map)
{
    // Access loglevel from the input command, otherwise assign these log level
    // to "INFO"
    logLevel = (variables_map.count("loglevel"))
                   ? variables_map["loglevel"].as<std::string>()
                   : "INFO";
    SET_LOG_LEVEL(logLevel);

    // Access log file, if specified.
    if (variables_map.count("logfile"))
    {
        logFile = variables_map["logfile"].as<std::string>();
        OPEN_LOG_FILE(logFile);
    }
}

void ProcessArguments::setSubsettype(
    program_options::variables_map variables_map)
{
    // If either the subset type are specified, pull them from the
    // variable map, otherwise assign these variables to an empty string.
    subsettype = (variables_map.count("subsettype"))
                     ? variables_map["subsettype"].as<std::string>()
                     : "";
}

void ProcessArguments::setConfigFile(
    program_options::variables_map variables_map)
{
    // If either the  config file are specified, pull them from the
    // variable map, otherwise assign these variables to an empty string.
    configFile = (variables_map.count("configfile"))
                     ? variables_map["configfile"].as<std::string>()
                     : "";
}

void ProcessArguments::setDatasetList(
    program_options::variables_map variables_map)
{
    // Access included dataset(s), if specified.
    if (variables_map.count("includedataset"))
    {
        datasetList = variables_map["includedataset"].as<std::string>();
        LOG_INFO("Subset::process_args(): includedataset: " << datasetList);
    }
}

void ProcessArguments::setReformat(program_options::variables_map variables_map)
{
    // Access reformatting, if specified.
    if (variables_map.count("reformat"))
    {
        LOG_INFO("Subset::process_args(): reformat");
        originalOutputFormat = outputFormat =
            variables_map["reformat"].as<std::string>();
        if (outputFormat == "GeoTIFF" || outputFormat == "GTiff" ||
            outputFormat == "GEO" || outputFormat == "KML")
        {
            outputFormat = "GeoTIFF";
        }
        else if (outputFormat == "netCDF3" || outputFormat == "NetCDF3" ||
                 outputFormat == "NetCDF-3")
        {
            outputFormat = "NetCDF-3";
        }
        LOG_INFO("Subset::process_args(): Reformatting to "
                 << originalOutputFormat);
    }
}

void ProcessArguments::setCRS(program_options::variables_map variables_map)
{
    // Access coordinate reference system / reprojection, if specified.
    if (variables_map.count("crs"))
        reproject = true;
    else
        reproject = false;
}

void ProcessArguments::setCollectionShortname(
    program_options::variables_map variables_map)
{
    // Access shortname from the input command, otherwise assign these variables
    // to an empty string
    collShortName = (variables_map.count("shortname"))
                        ? variables_map["shortname"].as<std::string>()
                        : "";
    boost::trim_right(collShortName);
}

int ProcessArguments::showHelpVerifyFilename(
    program_options::options_description description,
    program_options::variables_map variables_map)
{
    // Print out the defined command options when the user either types
    // "help" or does not include a filename.
    if (variables_map.count("help") || !variables_map.count("filename"))
    {
        LOG_ERROR(description);
        return SHOW_HELP_OR_NO_FILENAME;
    }

    return PASS;
}

int ProcessArguments::setInFileName(
    program_options::variables_map variables_map)
{
    // Access filename from the input command, if specified.
    infilename = variables_map["filename"].as<std::string>();
    LOG_INFO("Subset::process_args(): filename: " << infilename);
    if (!std::ifstream(infilename.c_str()))
    {
        LOG_ERROR("Subset::setInFileName(): ERROR: Could not open input file "
                  << infilename);
        return ERROR;
    }

    return PASS;
}

int ProcessArguments::setOutFileName(
    program_options::variables_map variables_map)
{
    // Access output file from the input command, if specfied.
    if (variables_map.count("outfile"))
        outfilename = variables_map["outfile"].as<std::string>();
    if (outfilename.find("--") == 0 || !std::ofstream(outfilename.c_str()))
    {
        LOG_ERROR("Subset::setOutFileName(): ERROR: Could not open output file "
                  << outfilename);
        return ERROR;
    }

    return PASS;
}

int ProcessArguments::setBoundingBox(
    program_options::variables_map variables_map)
{
    // Access bounding box from the input command, if specified.
    boost::match_results<std::string::const_iterator> regex_results;
    if (variables_map.count("bbox"))
    {
        std::vector<std::string> boxes =
            variables_map["bbox"].as<std::vector<std::string>>();
        boost::regex bbox_format(
            "[']?[-]?[.\\d]+,[-]?[.\\d]+,[-]?[.\\d]+,[-]?[.\\d]+[']?");

        for (std::vector<std::string>::iterator it = boxes.begin();
             it != boxes.end();
             it++)
        {
            if (!regex_match((*it), regex_results, bbox_format))
            {
                LOG_ERROR(
                    "Subset::process_args(): ERROR: Invalid bounding box: "
                    << *it);
                return ERROR;
            }

            LOG_INFO("Subset::process_args(): Adding bounding box " << *it);
            bounding_box += *it;
            boost::char_separator<char> sep(",");
            boost::tokenizer<boost::char_separator<char>> tokens(*it, sep);
            boost::tokenizer<boost::char_separator<char>>::iterator iter =
                tokens.begin();
            double w = atof(iter->c_str());
            double s = atof((++iter)->c_str());
            double e = atof((++iter)->c_str());
            double n = atof((++iter)->c_str());
            if (geoboxes == nullptr)
            {
                geoboxes = new std::vector<geobox>();
            }
            geoboxes->push_back(geobox(w, s, e, n));
        }
    }

    return PASS;
}

int ProcessArguments::setStartEndTemporalParameters(
    program_options::variables_map variables_map)
{
    boost::match_results<std::string::const_iterator> regex_results;

    // Access start and end temporal parameters, if specified.
    // Either both or none of the parameteres are expected when included.
    if (variables_map.count("start") && variables_map.count("end"))
    {
        startString = variables_map["start"].as<std::string>();
        endString = variables_map["end"].as<std::string>();
        boost::erase_all(startString, "'");
        boost::erase_all(endString, "'");
        boost::regex date_format(
            "[']?[\\d]{4}-[\\d]{2}-[\\d]{2}(T[\\d]{2}:[\\d]{2}"
            ":[\\d]{2}[\\.[\\d]*]?)?[']?");
        if (!regex_match(startString, regex_results, date_format) ||
            !regex_match(endString, regex_results, date_format))
        {
            LOG_ERROR("Subset::process_args(): ERROR: Invalid start or end "
                      "parameter ");
            return ERROR;
        }
    }
    else if (variables_map.count("start") || variables_map.count("end"))
    {
        LOG_ERROR("Subset::process_args(): ERROR: Invalid temporal parameters, "
                  "must pass in "
                  << "both --start and --end parameters");
        return ERROR;
    }

    return PASS;
}

int ProcessArguments::setBoundingShape(
    program_options::variables_map variables_map)
{
    // Access bounding shape, if specfied.
    if (variables_map.count("boundingshape"))
    {
        std::string boundingShape =
            variables_map["boundingshape"].as<std::string>();
        bool isGeoJson = boundingShape.find("geojson") != std::string::npos;

        try
        {
            if (isGeoJson && std::ifstream(boundingShape).good())
            {
                property_tree::read_json(boundingShape, boundingShapePt);
            }
            else
            {
                std::stringstream ss(boundingShape);
                property_tree::read_json(ss, boundingShapePt);
            }
        }
        catch (const boost::property_tree::json_parser::json_parser_error &e)
        {
            LOG_ERROR(
                "Subset::process_args(): JSON parsing error: " << e.what());
            return ERROR;
        }
        catch (const std::ios_base::failure &e)
        {
            LOG_ERROR("Subset::process_args(): File I/O error: " << e.what());
            return ERROR;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("Subset::process_args(): An unexpected error occurred: "
                      << e.what());
            return ERROR;
        }
    }

    return PASS;
}
