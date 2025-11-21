#ifndef PROCESSARGUMENTS_H
#define PROCESSARGUMENTS_H

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <time.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include "LogLevel.h"
#include "geobox.h"

namespace program_options = boost::program_options;
namespace property_tree = boost::property_tree;

class ProcessArguments
{
  public:
    static constexpr int PASS = 0;
    static constexpr int ERROR = 1;
    static constexpr int SHOW_HELP_OR_NO_FILENAME = 2;

    int process_args(int argc, char *argv[]);

    std::string getInfilename() { return infilename; }
    std::string getOutfilename() { return outfilename; }
    std::string getConfigFile() { return configFile; }
    std::string getSubsettype() { return subsettype; }
    std::string getBoundingBox() { return bounding_box; }
    std::string getStartString() { return startString; }
    std::string getEndString() { return endString; }
    std::string getOriginalOutputFormat() { return originalOutputFormat; }
    std::string getOutputFormat() { return outputFormat; }
    std::string getDatasetList() { return datasetList; }
    std::string getCollShortName() { return collShortName; }
    std::string getLogLevel() { return logLevel; }
    std::string getLogFile() { return logFile; }
    bool isReproject() { return reproject; }

    std::vector<geobox> *getGeoboxes() { return geoboxes; }
    std::vector<std::string> getDatasetsToInclude()
    {
        return datasetsToInclude;
    }
    boost::property_tree::ptree getBoundingShapePt() { return boundingShapePt; }

  private:
    void setLogLevel(program_options::variables_map variables_map);
    void setSubsettype(program_options::variables_map variables_map);
    void setConfigFile(program_options::variables_map variables_map);
    void setDatasetList(program_options::variables_map variables_map);
    void setReformat(program_options::variables_map variables_map);
    void setCRS(program_options::variables_map variables_map);
    void setCollectionShortname(program_options::variables_map variables_map);

    int showHelpVerifyFilename(program_options::options_description description,
                               program_options::variables_map variables_map);
    int setInFileName(program_options::variables_map variables_map);
    int setOutFileName(program_options::variables_map variables_map);
    int setBoundingBox(program_options::variables_map variables_map);
    int setStartEndTemporalParameters(
        program_options::variables_map variables_map);
    int setBoundingShape(program_options::variables_map variables_map);

    std::string infilename;
    std::string outfilename;
    std::string configFile;
    std::string subsettype;
    std::string bounding_box;
    std::string startString;
    std::string endString;
    std::string originalOutputFormat;
    std::string outputFormat;
    std::string datasetList;
    std::string collShortName;
    std::string logLevel;
    std::string logFile;
    bool reproject;

    std::vector<geobox> *geoboxes =
        nullptr; // Multiple bounding boxes can be specified.
    std::vector<std::string> datasetsToInclude;
    boost::property_tree::ptree boundingShapePt;
};

#endif
