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
class test_ATL10_v006_Configuration : public testing::Test
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

        configuration_a = std::make_unique<Configuration>(configFile);

        // Setup canned ATL10 v006 groups and dataset parsing H5 file
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/METADATA/Lineage/Control/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(shortName,
                                                                 "/gt1l/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/beam_fb_confidence/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/beam_fb_height/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/beam_fb_quality_flag/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/beam_fb_unc/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/beam_refsurf_ndx/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/delta_time/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/geophysical/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/geoseg_beg");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/geoseg_end/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/height_segment_id/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/freeboard_segment/heights/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/leads/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/leads/ssh_n/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/leads/ssh_ndx/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/reference_surface_section/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/reference_surface_section/beam_fb_refsurf/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/reference_surface_section/beam_lead_n/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/reference_surface_section/beam_lead_ndx/");
        configuration_a->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1l/reference_surface_section/fbswath_ndx/");

        configuration_b = std::make_unique<Configuration>(configFile);
        configuration_b->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/METADATA/Lineage/Control/");
        configuration_b->addShortNameGroupDatasetFromGranuleFile(shortName,
                                                                 "/gt1r/");
        configuration_b->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1r/freeboard_segment/");
        configuration_b->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/gt1r/freeboard_segment/beam_fb_confidence/");

        configuration_c = std::make_unique<Configuration>(configFile);
        configuration_c->addShortNameGroupDatasetFromGranuleFile(
            shortName, "/METADATA/Lineage/Control/");
    }

    void TearDown() {}

    std::unique_ptr<Configuration> configuration_a;
    std::unique_ptr<Configuration> configuration_b;
    std::unique_ptr<Configuration> configuration_c;

    std::string shortName = "ATL10";
};

//
// Start ATL10 v006 Configuration::getReferencedGroupname()
//
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getReferencedGroupname_isReferenceSurfaceSectionGroup)
{
    std::string expectGroupname = "";

    // Test /gt1l/reference_surface_section
    std::string group = "/gt1l/reference_surface_section/";
    std::string groupname =
        configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /reference_surface_section/beam_fb_refsurf
    group = "/gt1l/reference_surface_section/beam_fb_refsurf";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getReferencedGroupname_isReferenceSurfaceSectionGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/reference_surface_section/abcdefg/";
    std::string groupname =
        configuration_a->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test geophy does not exist, expect groupname return empty string
    group = "/gt1l/reference_surface_section/geophy/abcdefg/";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test height does not exist, expect groupname return empty string
    group = "/gt1l/reference_surface_section/height/abcdefg/";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getReferencedGroupname_isFreeboardRateGroup)
{
    std::string expectGroupname = "/gt1l/reference_surface_section/";

    // Test /gt1l/freeboard_segment/heights/
    std::string group = "/gt1l/freeboard_segment/heights/";
    std::string groupname =
        configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_segment/heights/asr_25
    group = "/gt1l/freeboard_segment/heights/asr_25/";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_segment/geophysical/
    group = "/gt1l/freeboard_segment/geophysical/";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_segment/geophysical/latitude/
    group = "/gt1l/freeboard_segment/geophysical/latitude/";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getReferencedGroupname_isFreeboardRateGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/freeboard_seg/abcdefg/";
    std::string groupname =
        configuration_a->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration, ATL10_getReferencedGroupname_isLeadsGroup)
{
    std::string expectGroupname = "/gt1l/reference_surface_section/";

    // Test /gt1l/leads/
    std::string group = "/gt1l/leads/";
    std::string groupname =
        configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/leads/ssh_ndx
    group = "/gt1l/leads/ssh_ndx/";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getReferencedGroupname_isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test /gt1l/lea/ does not exist, expect groupname return empty string
    std::string group = "/gt1l/lea/";
    std::string groupname =
        configuration_a->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test /gt1l/lea/abcdefg/ does not exist, expect groupname return empty
    // string
    group = "/gt1l/lea/abcdefg/";
    groupname = configuration_a->getReferencedGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getReferencedGroupname_isFreeboardSegmentGroup)
{
    std::string expectGroupname = "/gt1l/reference_surface_section/";

    // Test /gt1l/freeboard_segment/
    std::string group = "/gt1l/freeboard_segment/";
    std::string groupname =
        configuration_a->getReferencedGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

//
// End ATL10 v006 Configuration::getReferencedGroupname()
//

//
// Start ATL10 v006 Configuration::getTargetGroupname()
//
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getTargetGroupname_isFreeboardRateGroup)
{
    std::string expectGroupname = "/gt1l/reference_surface_section/";

    // Test /gt1l/freeboard_segment/
    std::string group = "/gt1l/freeboard_segment/";
    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_segment/heights/asr_25
    group = "/gt1l/freeboard_segment/heights/asr_25/";
    groupname = configuration_a->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_segment/geophysical/
    group = "/gt1l/freeboard_segment/geophysical/";
    groupname = configuration_a->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/freeboard_segment/geophysical/latitude/
    group = "/gt1l/freeboard_segment/geophysical/latitude/";
    groupname = configuration_a->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getTargetGroupname_isReferenceSurfaceSectionGroup_isBeamIndex)
{
    std::string expectGroupname = "/gt1l/leads/";

    // Test /gt1l/reference_surface_section/beam_lead_ndx/
    std::string group = "/gt1l/reference_surface_section/beam_lead_ndx/";
    std::string datasetName = "beam_lead_ndx";
    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/reference_surface_section/
    group = "/gt1l/reference_surface_section/";
    groupname =
        configuration_a->getTargetGroupname(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(
    test_ATL10_v006_Configuration,
    ATL10_getTargetGroupname_isReferenceSurfaceSectionGroup_isSwathHeightIndex)
{
    std::string expectGroupname = "/freeboard_swath_segment/";

    // Test /gt1l/reference_surface_section/fbswath_ndx/
    std::string group = "/gt1l/reference_surface_section/fbswath_ndx/";
    std::string datasetName = "fbswath_ndx";
    configuration_a->setFreeboardSwathSegment(true);

    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);

    // Test /gt1l/reference_surface_section/
    group = "/gt1l/reference_surface_section/";
    groupname =
        configuration_a->getTargetGroupname(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getTargetGroupname_isReferenceSurfaceSectionGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/reference_surface_section/abcdefg/";
    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);

    // Test geophy does not exist, expect groupname return empty string
    group = "/gt1l/reference_surface_section/geophy/abcdefg/";
    groupname = configuration_a->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration, ATL10_getTargetGroupname_isLeadsGroup)
{
    std::string expectGroupname = "/gt1l/freeboard_segment/heights/";

    // Test /gt1l/leads/
    std::string group = "/gt1l/leads/";
    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getTargetGroupname_isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test abcdefg does not exist, expect groupname return empty string
    std::string group = "/gt1l/lea/";
    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getTargetGroupname_isFreeboardSegmentGroup)
{
    std::string expectGroupname = "/gt1l/reference_surface_section/";

    // Test /gt1l/freeboard_segment/
    std::string group = "/gt1l/freeboard_segment/";
    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

// Negative test misspell dataset name
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getTargetGroupname_isFreeboardSegmentGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell /gt1l/freeboard_seg/, expect groupname return empty string
    std::string group = "/gt1l/freeboard_seg/";
    std::string groupname =
        configuration_a->getTargetGroupname(shortName, group);
    ASSERT_EQ(expectGroupname, groupname);
}

//
// Start ATL10 v006 Configuration::getIndexBeginDatasetName()
//
TEST_F(test_ATL10_v006_Configuration,
       ATL10_getIndexBeginDatasetName_isLeadsGroup)
{
    std::string expectGroupname = "ssh_ndx";

    // Test /gt1l/leads/
    std::string group = "/gt1l/leads/";
    std::string groupname =
        configuration_a->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "lead_height";
    groupname = configuration_a->getIndexBeginDatasetName(
        shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getIndexBeginDatasetName_isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell gt1l/lea/
    std::string group = "/gt1l/lea/";
    std::string groupname =
        configuration_a->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getIndexBeginDatasetName_isFreeboardSegmentGroup)
{
    std::string expectGroupname = "beam_refsurf_ndx";

    // Test /gt1l/freeboard_segment/
    std::string group = "/gt1l/freeboard_segment/";
    std::string groupname =
        configuration_a->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_height";
    groupname = configuration_a->getIndexBeginDatasetName(
        shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getIndexBeginDatasetName_isBeamFreeboardGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell beam_free /gt1l/freeboard_bea/
    std::string group = "/gt1l/freeboard_bea/";
    std::string groupname =
        configuration_a->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    // Test misspell "freeboard_bea" /gt1l/freeboard_bea/beam_freeboard/
    std::string datasetName = "beam_fb_height";
    group = "/gt1l/freeboard_bea/beam_freeboard/";
    groupname = configuration_a->getIndexBeginDatasetName(
        shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(
    test_ATL10_v006_Configuration,
    ATL10_getIndexBeginDatasetName_isReferenceSurfaceSectionGroup_isSwathHeightIndex)
{
    std::string expectGroupname = "fbswath_ndx";

    std::string group = "/gt1l/reference_surface_section/";
    std::string datasetName = "fbswath_ndx";
    std::string groupname = configuration_a->getIndexBeginDatasetName(
        shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getIndexBeginDatasetName_isReferenceSurfaceSectionGroup_BeamIndex)
{
    std::string expectGroupname = "beam_lead_ndx";

    // Test /gt1l/reference_surface_section/
    //  repair = true only used in IcesatSubsetter::writeDataset()
    std::string group = "/gt1l/reference_surface_section/";
    std::string datasetName("");
    bool repair = true;
    std::string groupname = configuration_a->getIndexBeginDatasetName(
        shortName, group, datasetName, repair);
    EXPECT_EQ(expectGroupname, groupname);

    datasetName = "beam_fb_length";
    groupname = configuration_a->getIndexBeginDatasetName(
        shortName, group, datasetName, repair);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getIndexBeginDatasetName_isReferenceSurfaceSectionGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell "freeboard_beam_segm" /gt1l/freeboard_beam_segm/
    std::string group = "/gt1l/freeboard_beam_segm/";
    std::string groupname =
        configuration_a->getIndexBeginDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_length";
    groupname = configuration_a->getIndexBeginDatasetName(
        shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

//
// Start ATL10 v006 Configuration::getCountDatasetName()
//
TEST_F(test_ATL10_v006_Configuration, ATL10_getCountDatasetName_isLeadsGroup)
{
    std::string expectGroupname = "ssh_n";

    // Test /gt1l/leads/
    std::string group = "/gt1l/leads/";
    std::string groupname =
        configuration_a->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "lead_height";
    groupname =
        configuration_a->getCountDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getCountDatasetName__isLeadsGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell gt1l/lea/
    std::string group = "/gt1l/lea/";
    std::string groupname =
        configuration_a->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getCountDatasetName_isReferenceSurfaceSectionGroup)
{
    std::string expectGroupname = "beam_lead_n";

    // Test /gt1l/reference_surface_section/
    std::string group = "/gt1l/reference_surface_section/";
    std::string groupname =
        configuration_a->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_length";
    groupname =
        configuration_a->getCountDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration,
       ATL10_getCountDatasetName_isReferenceSurfaceSectionGroup_Negative)
{
    std::string expectGroupname = "";

    // Test misspell /gt1l/reference_surface_/
    std::string group = "/gt1l/reference_surface_/";
    std::string groupname =
        configuration_a->getCountDatasetName(shortName, group);
    EXPECT_EQ(expectGroupname, groupname);

    std::string datasetName = "beam_fb_length";
    groupname =
        configuration_a->getCountDatasetName(shortName, group, datasetName);
    EXPECT_EQ(expectGroupname, groupname);
}

TEST_F(test_ATL10_v006_Configuration, ATL10_isShortNameGrougInH5File)
{
    // The tests below uses mock data from above method void SetUp()
    std::string group = "/gt1l/reference_surface_section/";
    bool bisShortNameGrougInH5File =
        configuration_a->isShortNameGroupDatasetFromGranuleFile(shortName,
                                                                group);
    EXPECT_TRUE(bisShortNameGrougInH5File);

    group = "/gt1l/leads/";
    bisShortNameGrougInH5File =
        configuration_a->isShortNameGroupDatasetFromGranuleFile(shortName,
                                                                group);
    EXPECT_TRUE(bisShortNameGrougInH5File);

    group = "/gt1l/freeboard_segment/beam_refsurf_ndx/";
    bisShortNameGrougInH5File =
        configuration_a->isShortNameGroupDatasetFromGranuleFile(shortName,
                                                                group);
    EXPECT_TRUE(bisShortNameGrougInH5File);
}

TEST_F(test_ATL10_v006_Configuration, ATL10_isShortNameGrougInH5File_Negative)
{
    // The tests below uses mock data from above method void SetUp()
    // /gt1l/freeboard_beam_segment is ATL10 v005
    std::string group = "/gt1l/freeboard_beam_segment/";
    bool bisShortNameGrougInH5File =
        configuration_a->isShortNameGroupDatasetFromGranuleFile(shortName,
                                                                group);
    EXPECT_FALSE(bisShortNameGrougInH5File);

    // /gt1l/freeboard_beam_segment is ATL10 v005
    group = "/gt1l/freeboard_beam_segment/beam_refsurf_ndx/";
    bisShortNameGrougInH5File =
        configuration_a->isShortNameGroupDatasetFromGranuleFile(shortName,
                                                                group);
    EXPECT_FALSE(bisShortNameGrougInH5File);
}

TEST_F(test_ATL10_v006_Configuration, ATL10_getVersionNumber_v006)
{
    // The tests below uses mock data from above method void SetUp()
    std::string expectedVersion("ATL10v006");
    std::string version = configuration_a->getVersionNumber(shortName);
    EXPECT_EQ(expectedVersion, version);
}

TEST_F(test_ATL10_v006_Configuration, ATL10_getVersionNumber_v006_gt1r)
{
    // The tests below uses mock data from above method void SetUp()
    std::string expectedVersion("ATL10v006");
    std::string version = configuration_b->getVersionNumber(shortName);
    EXPECT_EQ(expectedVersion, version);
}

TEST_F(test_ATL10_v006_Configuration, ATL10_getVersionNumber_no_version)
{
    // The tests below uses mock data from above method void SetUp()
    std::string expectedVersion("");
    std::string version = configuration_c->getVersionNumber(shortName);
    EXPECT_EQ(expectedVersion, version);
}

} // namespace
