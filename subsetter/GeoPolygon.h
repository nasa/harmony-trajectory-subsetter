#ifndef GeoPolygon_H
#define GeoPolygon_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional/optional.hpp>
#include <boost/foreach.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#ifdef SDPS
#include <boost/geometry/geometries/multi_polygon.hpp>
#elifdef HARMONY
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/type_traits/is_empty.hpp>
#endif

#include "geobox.h"

using namespace std;
using namespace boost;
namespace pt = boost::property_tree;

typedef geometry::model::d2::point_xy<double> point_type;
typedef geometry::model::polygon<point_type> polygon_type;
#ifdef SDPS
typedef geometry::model::geometries::multi_polygon<polygon_type> multi_polygon_type;
#elifdef HARMONY
typedef geometry::model::multi_polygon<polygon_type> multi_polygon_type;
#endif
typedef geometry::model::box<point_type> box_type;


class GeoPolygon
{
public:
    
    multi_polygon_type polygons;
    box_type box;
    bool crossedEast;
    bool crossedWest;
    
    GeoPolygon(pt::ptree root):crossedEast(false),crossedWest(false)
    {
        //cout << "GeoPolygon" << endl;
        readPolygon(root);
        //cout << geometry::dsv(polygons) << endl;
    };
    
    //vector<vector<vector<vector<double>>>> getVertices() { return vertices;};
    
     /**
     * get a minimal bounding box surrounding the polygon
     * @return geobox object
     */
    geobox getBbox()
    {
        double w, s, e, n;
        
        geometry::envelope(polygons, box);
        
        cout << "box: " << geometry::dsv(box) << endl;
        
        w = box.min_corner().get<0>();
        s = box.min_corner().get<1>();
        e = box.max_corner().get<0>();
        n = box.max_corner().get<1>();
        
        // polygon crosses Anti-Meridian
        if (w < -180) crossedWest = true;
        else if (e > 180) crossedEast = true;
        
        /*cout << "west: " << w << endl;
        cout << "south: " << s << endl;
        cout << "east: " << e << endl;
        cout << "north: " << n << endl;*/
        
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
        
        point_type pt(lon, lat);
        
        if (boost::geometry::within(pt, polygons))
        {
            //cout << "point: " << geometry::dsv(pt) << " is within the polygon" << endl;
            return true;
        }
        else
        {
            //cout << "point: " << geometry::dsv(pt) << " is not within the polygon" << endl;
            return false;
        }
    }
    
    /**
     * return true if "polygons" is empty
     */
    bool isEmpty()
    {
#ifdef SDPS
        return (boost::geometry::is_empty(polygons))? true : false;
#elifdef HARMONY
        return polygons.empty();
#endif
    }
    
private:
    
    /* stores polygon/multi-polygon vertices
     * vector<double> - one vertex of the polygon
     * vector<vector<double>> - vertices of a polygon
     * vector<vector<vector<double>>> - polygon with inner rings
     * vector<<vector<vector<vector<double>>>> - multi-polygon
     */
    vector<vector<vector<vector<double>>>> vertices;
        
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
     * +-------+----+- Polygon      # 3D Array (of linear rings of points of [x,y])
     */
    void readPolygon(pt::ptree tree)
    {        
        vector<string> objTypes = {"Polygon", "MultiPolygon"};
        
        string objType = getType(tree);
        
        // found Polygon or MultiPolygon
        if (std::find(objTypes.begin(), objTypes.end(), objType) != objTypes.end())
        {
            getCoordinatesFromGeoJSON(tree);
        }
        else if (objType == "FeatureCollection" && tree.get_child_optional("features"))
        {
            BOOST_FOREACH(pt::ptree::value_type &nodei, tree.get_child("features"))
            {
                readPolygon(nodei.second);
            }
        }
        else if (objType == "Feature" && tree.get_child_optional("geometry"))
        {
            readPolygon(tree.get_child("geometry"));
        }
        else if (objType == "GeometryCollection" && tree.get_child_optional("geometries"))
        {
            BOOST_FOREACH(pt::ptree::value_type &nodei, tree.get_child("geometries"))
            {
                readPolygon(nodei.second);
            }
        }
        
    }
    
    /**
     * extract polygon vertices from the tree
     * @param tree GeoJSON content
     */
    void getCoordinatesFromGeoJSON(pt::ptree tree)
    {                
        //cout << "getCooordinatesFromGeoJSON" << endl;
        bool isPolygon = (getType(tree) == "Polygon") ? true : false;
        bool isMultiPolygon = (getType(tree) == "MultiPolygon") ? true : false;
        
        // if it is neither "Polygon" or MultiPolygon", return
        if (!isPolygon && !isMultiPolygon) return;
        
        bool outer = true;
        int inner = 0;
        polygon_type poly;
        
        BOOST_FOREACH(pt::ptree::value_type nodei, tree.get_child("coordinates"))
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
            else if (isMultiPolygon) getMultiPolygon(nodei.second, poly);
        }  
        polygons.push_back(poly);
    }
    
    /**
     * extract polygon from the tree
     * @param tree GeoJSON content
     */
    void getPolygon(polygon_type &poly, pt::ptree tree, bool outer, int inner)
    {
        //cout << "getPolygon: " << inner << endl;
        vector<double> point;
        
        BOOST_FOREACH(pt::ptree::value_type &nodei, tree)
        {
            BOOST_FOREACH(pt::ptree::value_type &nodej, nodei.second)
            {
                point.push_back(nodej.second.get_value<double>());
                if (point.size() == 2) break;
            }
            point_type pt(point[0], point[1]);
            point.clear();
            //cout << geometry::dsv(pt) << endl;
            if (outer) 
            {
                poly.outer().push_back(pt);
            }
            else
            {
                poly.inners()[inner-1].push_back(pt);
            }
        }
    }
    
    /**
     * extract multi-polygon from the tree
     * @param tree GeoJSON content
     */
    void getMultiPolygon(pt::ptree tree, polygon_type &poly)
    {
        //cout << "getMultiPolygon" << endl;
        bool outer = true;
        int inner = 0;
        int counter = 1;
        BOOST_FOREACH(pt::ptree::value_type &nodei, tree)
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
    string getType(pt::ptree tree)
    {
        string objType = "";
        if (tree.get_child_optional("type"))
        {
            objType = tree.get<string>("type");
        }
        
        return objType;
    }
    
};
#endif
