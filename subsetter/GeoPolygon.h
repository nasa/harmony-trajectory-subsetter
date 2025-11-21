#ifndef GeoPolygon_H
#define GeoPolygon_H

#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/optional/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <vector>

#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/type_traits/is_empty.hpp>

#include "LogLevel.h"
#include "geobox.h"

namespace property_tree = boost::property_tree;

typedef boost::geometry::model::d2::point_xy<double> point_type;
typedef boost::geometry::model::polygon<point_type> polygon_type;
typedef boost::geometry::model::multi_polygon<polygon_type> multi_polygon_type;
typedef boost::geometry::model::box<point_type> box_type;

class GeoPolygon
{
  public:
    multi_polygon_type polygons;
    box_type box;
    bool crossedEast;
    bool crossedWest;

    GeoPolygon(property_tree::ptree root)
        : crossedEast(false), crossedWest(false)
    {
        readPolygon(root);
    };

    /**
     * get a minimal bounding box surrounding the polygon
     * @return geobox object
     */
    geobox getBbox()
    {
        double w, s, e, n;

        boost::geometry::envelope(polygons, box);

        LOG_DEBUG("box: " << boost::geometry::dsv(box));

        w = box.min_corner().get<0>();
        s = box.min_corner().get<1>();
        e = box.max_corner().get<0>();
        n = box.max_corner().get<1>();

        // polygon crosses Anti-Meridian
        if (w < -180)
            crossedWest = true;
        else if (e > 180)
            crossedEast = true;

        return geobox(w, s, e, n);
    }

    /**
     * determine whether the point is within the polygon
     * @param lat latitude
     * @param lon longitude
     * @return true if the point is within the polygon, false otherwise
     */
    bool contains(double lat, double lon)
    {
        // if the bbox crosses the Anti-Meridian at the East bound,
        // add 360 to negative longitude values
        if (crossedEast == true && lon < 0)
        {
            lon = lon + 360;
        }
        // if the bbox crosses the Anti-Meridian at the West bound,
        // add -360 to positive longitude values
        else if (crossedWest == true && lon > 0)
        {
            lon = lon + -360;
        }

        point_type property_tree(lon, lat);

        if (boost::geometry::within(property_tree, polygons))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /**
     * return true if "polygons" is empty
     */
    bool isEmpty()
    {
#ifdef SDPS
        return (boost::geometry::is_empty(polygons)) ? true : false;
#elif defined HARMONY
        return polygons.empty();
#endif
    }

  private:
    /* stores polygon/multi-polygon vertices
     * std::vector<double> - one vertex of the polygon
     * std::vector<std::vector<double>> - vertices of a polygon
     * std::vector<std::vector<std::vector<double>>> - polygon with inner rings
     * std::vector<<std::vector<std::vector<std::vector<double>>>> -
     * multi-polygon
     */
    std::vector<std::vector<std::vector<std::vector<double>>>> vertices;

    /**
     * extract polygon from the GeoJSON content
     * Hiearchy of keywords for GeoJSON schema
     * GeoJSON -
     * |
     * +- FeatureCollection -
     * |   |
     * +---+- Feature ?  # with Properties
     * |       |
     * +-------+- GeometryCollection -
     * |       |    |
     * |       |  (Geometry)
     * |       |    |
     * +-------+----+- MultiPolygon # 4D Array (array of polygons, ?)
     * |       |    |
     * +-------+----+- Polygon      # 3D Array (of linear rings of points of
     * [x,y])
     */
    void readPolygon(property_tree::ptree tree)
    {
        LOG_DEBUG("GeoPolygon::readPolygon(): ENTER");

        std::vector<std::string> objTypes = {"Polygon", "MultiPolygon"};

        std::string objType = getType(tree);

        // found Polygon or MultiPolygon
        if (std::find(objTypes.begin(), objTypes.end(), objType) !=
            objTypes.end())
        {
            getCoordinatesFromGeoJSON(tree);
        }
        else if (objType == "FeatureCollection" &&
                 tree.get_child_optional("features"))
        {
            BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                           tree.get_child("features"))
            {
                readPolygon(nodei.second);
            }
        }
        else if (objType == "Feature" && tree.get_child_optional("geometry"))
        {
            readPolygon(tree.get_child("geometry"));
        }
        else if (objType == "GeometryCollection" &&
                 tree.get_child_optional("geometries"))
        {
            BOOST_FOREACH (property_tree::ptree::value_type &nodei,
                           tree.get_child("geometries"))
            {
                readPolygon(nodei.second);
            }
        }
    }

    /**
     * extract polygon vertices from the tree
     * @param tree GeoJSON content
     */
    void getCoordinatesFromGeoJSON(property_tree::ptree tree)
    {
        LOG_DEBUG("GeoPolygon::getCoordinatesFromGeoJSON(): ENTER");

        bool isPolygon = (getType(tree) == "Polygon") ? true : false;
        bool isMultiPolygon = (getType(tree) == "MultiPolygon") ? true : false;

        // if it is neither "Polygon" or MultiPolygon", return
        if (!isPolygon && !isMultiPolygon)
            return;

        bool outer = true;
        int inner = 0;
        polygon_type poly;

        BOOST_FOREACH (property_tree::ptree::value_type nodei,
                       tree.get_child("coordinates"))
        {
            if (isPolygon)
            {
                if (!outer)
                {
                    inner++;
                    poly.inners().resize(inner);
                }
                getPolygon(poly, nodei.second, outer, inner);
                outer = false;
            }
            else if (isMultiPolygon)
                getMultiPolygon(nodei.second, poly);
        }
        polygons.push_back(poly);
    }

    /**
     * extract polygon from the tree
     * @param tree GeoJSON content
     */
    void getPolygon(polygon_type &poly,
                    property_tree::ptree tree,
                    bool outer,
                    int inner)
    {
        LOG_DEBUG("GeoPolygon::getPolygon(): ENTER");

        std::vector<double> point;

        BOOST_FOREACH (property_tree::ptree::value_type &nodei, tree)
        {
            BOOST_FOREACH (property_tree::ptree::value_type &nodej,
                           nodei.second)
            {
                point.push_back(nodej.second.get_value<double>());
                if (point.size() == 2)
                    break;
            }
            point_type property_tree(point[0], point[1]);
            point.clear();
            if (outer)
            {
                poly.outer().push_back(property_tree);
            }
            else
            {
                poly.inners()[inner - 1].push_back(property_tree);
            }
        }
    }

    /**
     * extract multi-polygon from the tree
     * @param tree GeoJSON content
     */
    void getMultiPolygon(property_tree::ptree tree, polygon_type &poly)
    {
        LOG_DEBUG("GeoPolygon::getMultiPolygon(): ENTER");

        bool outer = true;
        int inner = 0;
        int counter = 1;
        BOOST_FOREACH (property_tree::ptree::value_type &nodei, tree)
        {
            if (!outer)
            {
                inner++;
                poly.inners().resize(inner);
            }
            getPolygon(poly, nodei.second, outer, inner);
            outer = false;
            polygons.push_back(poly);
            poly.clear();
        }
    }

    /**
     * returns object type extracted from GeoJSON
     * @param tree GeoJSON content
     * @return objType type of the object
     */
    std::string getType(property_tree::ptree tree)
    {
        std::string objType = "";
        if (tree.get_child_optional("type"))
        {
            objType = tree.get<std::string>("type");
        }

        return objType;
    }
};
#endif
