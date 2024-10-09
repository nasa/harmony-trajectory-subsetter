#include <gtest/gtest.h>
#include <iostream>

#include "gtest_utilities.h"

#include "H5Cpp.h"
#include "../../../subsetter/ForwardReferenceCoordinates.h"
#include "../../../subsetter/Configuration.h"
#include "../../../subsetter/geobox.h"
#include "../../../subsetter/GeoPolygon.h"
#include "../../../subsetter/Temporal.h"

#include <string>
#include <regex>


class ForwardReferenceCoordinatesTest : public ::testing::Test
{
protected:
    ForwardReferenceCoordinatesTest()
    {
        std::string config_file_path = gtest_utilities::getFullPath("harmony_service/subsetter_config.json");
        config = new Configuration(config_file_path);
        coordinate_object = new ForwardReferenceCoordinates(groupname, geoboxes, temporal, geopolygon, config);

        // Read in test data.
        index_begin_dataset = gtest_utilities::readDataset(gtest_utilities::getFullPath("tests/data/ATL03_index_begin.h5"), "ph_index_beg");

    }

    ~ForwardReferenceCoordinatesTest()
    {
        delete config;
        delete coordinate_object;
        delete index_begin_dataset;
    }

    int64_t* index_begin_dataset = nullptr;
    ForwardReferenceCoordinates* coordinate_object = nullptr;

private:

    std::string groupname = "ATL03";
    std::vector<geobox>* geoboxes = nullptr;
    Temporal* temporal = nullptr;
    GeoPolygon* geopolygon = nullptr;
    Configuration* config = nullptr;

};

TEST_F(ForwardReferenceCoordinatesTest, DefineOneSegment_StartIndexBeginNonFillValue)
{
    // Segment data generated from bounding box {68.9502,-0.61901,69.43799,-0.18814}.
    long selectedStartIdx = 147825;
    long selectedCount = 718;
    long maxIndexBegIdx = 149892;
    long maxTrajIndex = 1639461;

    long firstTrajIndex_expected = 1606025;
    long firstTrajIndex_result = 0;          // Returned-by-reference
    long trajSegLength_expected = 12652;
    long trajSegLength_result = 0;           // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx, selectedCount,
                                        firstTrajIndex_result, trajSegLength_result,
                                        maxIndexBegIdx, maxTrajIndex,
                                        this->index_begin_dataset);

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);

}

TEST_F(ForwardReferenceCoordinatesTest, DefineOneSegment_StartIndexBeginFillValue)
{
    // Segment data generated from bounding box {69.421946,-0.226334,69.452095,-0.215356}.
    long selectedStartIdx = 148638;
    long selectedCount = 60;
    long maxIndexBegIdx = 149892;
    long maxTrajIndex = 1639461;

    long firstTrajIndex_expected = 1619651;
    long firstTrajIndex_result = 0;          // Returned-by-reference
    long trajSegLength_expected = 294;
    long trajSegLength_result = 0;           // Returned-by-reference

    coordinate_object->defineOneSegment(selectedStartIdx, selectedCount,
                                        firstTrajIndex_result, trajSegLength_result,
                                        maxIndexBegIdx, maxTrajIndex,
                                        this->index_begin_dataset);

    std::cout << "firstTrajIndex_result = " << firstTrajIndex_result;
    std::cout << "\ntrajSegLength_result = " << trajSegLength_result;

    EXPECT_EQ(firstTrajIndex_expected, firstTrajIndex_result);
    EXPECT_EQ(trajSegLength_expected, trajSegLength_result);
}
