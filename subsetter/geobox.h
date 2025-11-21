#ifndef Geobox_H
#define Geobox_H

// class to test whether dataset lat/lon values match those provided by user
class geobox
{
    // bounding box coordinates specified on the command line
    double west;
    double south;
    double east;
    double north;

  public:
    geobox(double w, double s, double e, double n)
        : west(w), south(s), east(e), north(n) {};
    bool contains_lat(double lat)
    {
        return (south < north ? lat <= north && lat >= south
                              : lat >= south || lat <= north);
    }
    bool contains_lon(double lon)
    {
        // if the bbox crosses the Anti-Meridian at the East bound,
        // add 360 to negative longitude values
        if (west < -180 && lon > 0)
            lon += -360;
        // if the bbox crosses the Anti-Meridian at the West bound,
        // add -360 to positive longitude values
        else if (east > 180 && lon < 0)
            lon += 360;

        return (west < east ? lon >= west && lon <= east
                            : lon >= west || lon <= east);
    }
    bool contains(double lat, double lon)
    {
        return (contains_lat(lat) && contains_lon(lon));
    }
    double getWest() { return west; }
    double getEast() { return east; }
    double getNorth() { return north; }
    double getSouth() { return south; }
};
#endif
