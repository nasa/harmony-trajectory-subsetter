#include <gtest/gtest.h>
#include <gtest/internal/gtest-filepath.h>

#include <boost/foreach.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include "../../../subsetter/Configuration.h"
#include "../../../subsetter/GeoPolygon.h"
#include "../../../subsetter/geobox.h"
#include <iostream>

#include <regex>
#include <string>

namespace
{
//
class test_ATL10_v005_Configuration : public testing::Test
{
  protected:
    void SetUp()
    {
        std::string configFile;
        std::string currentDir =
            testing::internal::FilePath::GetCurrentDir().c_str();

        configFile =
            std::regex_replace(currentDir,
                               std::regex("tests/unit/gtest/build"),
                               "harmony_service/subsetter_config.json");
        std::cout << "Full path: " << configFile.c_str() << std::endl;

        configuration = std::make_unique<Configuration>(configFile);
    }

    void TearDown() {}

    std::unique_ptr<Configuration> configuration;
    std::string shortName = "ATL10";
};

//
// Start ATL10 v005 Configuration::getReferencedGroupname()
//
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getReferencedGroupname_isFreeboardBeamSegmentGroup)
{
    std::string expectGroupname = "";

    // Test /gt1l/freeboard_beam_segment/beam_freeboard/
    std::string group = "/gt1l/freeboard_beam_segment/";
    std::string groupname =
        configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /freeboard_swath_segment/
    group = "/freeboard_swath_segment/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getReferencedGroupname_isFreeboardBeamSegmentGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/freeboard_beam_segment/abcdefg/";
    std::string groupname =
        configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test geophy does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/geophy/abcdefg/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test height does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/height/abcdefg/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getReferencedGroupname_isHeightSegmentRateGroup)
{
    std::string expectGroupname = "/gt1l/freeboard_beam_segment/";

    // Test /gt1l/freeboard_beam_segment/beam_freeboard/beam_fb_height
    std::string group =
        "/gt1l/freeboard_beam_segment/beam_freeboard/beam_fb_height/";
    std::string groupname =
        configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test/gt1l/freeboard_beam_segment/height_segments/
    group = "/gt1l/freeboard_beam_segment/height_segments/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test/gt1l/freeboard_beam_segment/height_segments/asr_25
    group = "/gt1l/freeboard_beam_segment/height_segments/asr_25/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_beam_segment/geophysical/
    group = "/gt1l/freeboard_beam_segment/geophysical/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_beam_segment/geophysical/latitude/
    group = "/gt1l/freeboard_beam_segment/geophysical/latitude/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_beam_segment/beam_freeboard/
    group = "/gt1l/freeboard_beam_segment/beam_freeboard/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getReferencedGroupname_isHeightSegmentRateGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/freeboard_beam_segment/abcdefg/";
    std::string groupname =
        configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test geophy does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/geophy/abcdefg/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test height does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/height/abcdefg/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration, ATL10_getReferencedGroupname_isLeadsGroup)
{
    std::string expectGroupname = "/gt1l/freeboard_beam_segment/";

    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/freeboard_beam_segment/");
    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/freeboard_beam_segment/height_segments/");
    configuration->addShortNameGroupDatasetFromGranuleFile(shortName,
                                                           "/gt1l/leads/");
    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/leads/ssh_ndx/");

    // Test /gt1l/leads/delta_time
    std::string group = "/gt1l/leads/";
    std::string groupname =
        configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/leads/ssh_ndx
    group = "/gt1l/leads/ssh_ndx/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getReferencedGroupname_isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/lea/";
    std::string groupname =
        configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test geophy does not exist, expect groupname return empty string
    group = "/gt1l/lea/abcdefg/";
    groupname = configuration->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}
//
// End ATL10 v005 Configuration::getReferencedGroupname()
//

//
// Start ATL10 v005 Configuration::getTargetGroupname()
//
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getTargetGroupname_isFreeboardBeamSegmentGroup)
{
    std::string expectGroupname = "";

    // Test /gt1l/freeboard_beam_segment/
    // No TargetGroupName without /gt1l/freeboard_beam_segment/beam_lead_ndx
    std::string group = "/gt1l/freeboard_beam_segment/";
    std::string groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getTargetGroupname_isFreeboardBeamSegmentGroup_isBeamIndex)
{
    std::string expectGroupname = "/gt1l/leads/";

    // Test /gt1l/freeboard_beam_segment/ with datasetName option
    std::string datasetName = "beam_lead_ndx";
    std::string group = "/gt1l/freeboard_beam_segment/";
    std::string groupname =
        configuration->getTargetGroupname(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getTargetGroupname_isFreeboardBeamSegmentGroup_isSwathHeightIndex)
{
    std::string expectGroupname = "/freeboard_swath_segment/";

    // Test /gt1l/freeboard_beam_segment/fbswath_ndx/
    std::string group = "/gt1l/freeboard_beam_segment/fbswath_ndx/";
    std::string datasetName = "fbswath_ndx";
    configuration->setFreeboardSwathSegment(true);

    // Test /gt1l/freeboard_beam_segment/fbswath_ndx/ with datasetName option
    std::string groupname =
        configuration->getTargetGroupname(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_beam_segment/ with datasetName option
    group = "/gt1l/freeboard_beam_segment/";
    groupname =
        configuration->getTargetGroupname(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getTargetGroupname_isFreeboardBeamSegmentGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/freeboard_beam_segment/abcdefg/";
    std::string groupname = configuration->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test geophy does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/geophy/abcdefg/";
    groupname = configuration->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test height does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/height/abcdefg/";
    groupname = configuration->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration, ATL10_getTargetGroupname_isLeadsGroup)
{
    std::string expectGroupname =
        "/gt1l/freeboard_beam_segment/height_segments/";

    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/freeboard_beam_segment/");
    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/freeboard_beam_segment/height_segments/");
    configuration->addShortNameGroupDatasetFromGranuleFile(shortName,
                                                           "/gt1l/leads/");
    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/leads/ssh_ndx/");

    // Test /gt1l/freeboard_beam_segment/beam_freeboard/beam_fb_height
    std::string group = "/gt1l/leads/";
    std::string groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getTargetGroupname_isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell
    std::string group = "/gt1l/lea/";
    std::string groupname = configuration->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getTargetGroupname_isHeightSegmentRateGroup)
{
    std::string expectGroupname = "/gt1l/freeboard_beam_segment/";

    // Test /gt1l/freeboard_beam_segment/beam_freeboard/beam_fb_height
    std::string group =
        "/gt1l/freeboard_beam_segment/beam_freeboard/beam_fb_height/";
    std::string groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test/gt1l/freeboard_beam_segment/height_segments/
    group = "/gt1l/freeboard_beam_segment/height_segments/";
    groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test/gt1l/freeboard_beam_segment/height_segments/asr_25
    group = "/gt1l/freeboard_beam_segment/height_segments/asr_25/";
    groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_beam_segment/geophysical/
    group = "/gt1l/freeboard_beam_segment/geophysical/";
    groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_beam_segment/geophysical/latitude/
    group = "/gt1l/freeboard_beam_segment/geophysical/latitude/";
    groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_beam_segment/beam_freeboard/
    group = "/gt1l/freeboard_beam_segment/beam_freeboard/";
    groupname = configuration->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getTargetGroupname_isHeightSegmentRateGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/freeboard_beam_segment/abcdefg/";
    std::string groupname = configuration->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test geophy does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/geophy/abcdefg/";
    groupname = configuration->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test height does not exist, expect groupname return empty string
    group = "/gt1l/freeboard_beam_segment/height/abcdefg/";
    groupname = configuration->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

//
// Start ATL10 v005 Configuration::getIndexBeginDatasetName()
//
TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isLeadsGroup)
{
    std::string expectGroupname = "ssh_ndx";

    // Add Mock data from H5 file to test leads
    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/freeboard_beam_segment/");
    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/freeboard_beam_segment/height_segments");
    configuration->addShortNameGroupDatasetFromGranuleFile(shortName,
                                                           "/gt1l/leads/ssh_n");
    configuration->addShortNameGroupDatasetFromGranuleFile(
        shortName, "/gt1l/leads/ssh_ndx");

    // Test /gt1l/leads/
    std::string group = "/gt1l/leads/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "lead_height";
    groupname =
        configuration->getIndexBeginDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell gt1l/lea/
    std::string group = "/gt1l/lea/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isBeamFreeboardGroup)
{
    std::string expectGroupname = "beam_refsurf_ndx";

    // Test /gt1l/freeboard_beam_segment/beam_freeboard/
    std::string group = "/gt1l/freeboard_beam_segment/beam_freeboard/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_height";
    groupname =
        configuration->getIndexBeginDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isBeamFreeboardGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell beam_free /gt1l/freeboard_beam_segment/beam_free/
    std::string group = "/gt1l/freeboard_bea/beam_free/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test misspell "freeboard_bea" /gt1l/freeboard_bea/beam_freeboard/
    std::string datasetName = "beam_fb_height";
    group = "/gt1l/freeboard_bea/beam_freeboard/";
    groupname =
        configuration->getIndexBeginDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(
    test_ATL10_v005_Configuration,
    ATL10_getIndexBeginDatasetName_isFreeboardBeamSegmentGroup_isSwathHeightIndex)
{
    std::string expectGroupname = "fbswath_ndx";

    // Test /gt1l/freeboard_beam_segment/
    std::string group = "/gt1l/freeboard_beam_segment/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_length";
    groupname =
        configuration->getIndexBeginDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isFreeboardBeamSegmentGroup_BeamIndex)
{
    std::string expectGroupname = "beam_lead_ndx";

    // Test /gt1l/freeboard_beam_segment/
    std::string group = "/gt1l/freeboard_beam_segment/";
    std::string datasetName("");
    std::string groupname = configuration->getIndexBeginDatasetName(
        shortName, group, datasetName, true);
    EXPECT_EQ(expectGroupname, groupname);

    datasetName = "beam_fb_length";
    groupname = configuration->getIndexBeginDatasetName(
        shortName, group, datasetName, true);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isFreeboardBeamSegmentGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell "freeboard_beam_segm" /gt1l/freeboard_beam_segm/
    std::string group = "/gt1l/freeboard_beam_segm/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_length";
    groupname =
        configuration->getIndexBeginDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isSwathFreeboardGroup)
{
    std::string expectGroupname = "fbswath_ndx";

    // Test /freeboard_swath_segment/gt1l/swath_freeboard/
    std::string group = "/freeboard_swath_segment/gt1l/swath_freeboard/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "fbswath_fb_height";
    groupname =
        configuration->getIndexBeginDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getIndexBeginDatasetName_isSwathFreeboardGroup_Negative)
{
    std::string expectGroupname = "";

    // Test /freeboard_swath_segment/gt1l/swath_/
    std::string group = "/freeboard_swath_segment/gt1l/swath_/";
    std::string groupname =
        configuration->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "fbswath_fb_height";
    groupname =
        configuration->getIndexBeginDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

//
// Start ATL10 v005 Configuration::getCountDatasetName()
//
TEST_F(test_ATL10_v005_Configuration, ATL10_getCountDatasetName_isLeadsGroup)
{
    std::string expectGroupname = "ssh_n";

    // Test /gt1l/leads/
    std::string group = "/gt1l/leads/";
    std::string groupname =
        configuration->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "lead_height";
    groupname =
        configuration->getCountDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getCountDatasetName__isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell gt1l/lea/
    std::string group = "/gt1l/lea/";
    std::string groupname =
        configuration->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getCountDatasetName_isFreeboardBeamSegmentGroup)
{
    std::string expectGroupname = "beam_lead_n";

    // Test /gt1l/freeboard_beam_segment/
    std::string group = "/gt1l/freeboard_beam_segment/";
    std::string groupname =
        configuration->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_length";
    groupname =
        configuration->getCountDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v005_Configuration,
       ATL10_getCountDatasetName_isFreeboardBeamSegmentGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell "freeboard_beam_segm" /gt1l/freeboard_beam_segm/
    std::string group = "/gt1l/freeboard_beam_segm/";
    std::string groupname =
        configuration->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_length";
    groupname =
        configuration->getCountDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

} // namespace
