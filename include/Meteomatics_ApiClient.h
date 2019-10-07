//
//  Meteomatics_ApiClient.h
//  MeteomaticsApi
//
//  Created by Christian Schluchter on 04/02/16.
//  Copyright © 2018 Meteomatics. All rights reserved.
//

#ifndef MeteomaticsApiClient_h
#define MeteomaticsApiClient_h

#include <vector>

namespace MMIntern {
class MemoryClass;
class HttpClient;
}

typedef std::vector<std::vector<double>> Matrix;

class MeteomaticsApiClient
{
public:

    MeteomaticsApiClient(const std::string& _user, const std::string& password, const int timeout_seconds);
    
    //
    // -- query for a single point (one time, one coordinate)
    //
    bool getPoint(const std::string& time, const std::vector<std::string>& parameters, double lat, double lon, std::vector<double>& result, std::string& msg, const std::vector<std::string>& optionals={}) const;
    
    //
    // -- query for points on a grid points (one time, coordinate grid)
    //
    bool getGrid(const std::string& time, const std::string& parameter, const double lat_N, const double lon_W, const double lat_S, const double lon_E, const int nrGridPts_Lat, const int nrGridPts_Lon, Matrix& gridResult, std::vector<double>& latGridPts, std::vector<double>& lonGridPts, std::string& msg, const std::vector<std::string>& optionals={}) const;
    

    //
    // -- query for several times at a single point (multiple times, single coordinate)
    //
    bool getTimeSeries(const std::string& startTime, const std::string& stopTime, const std::string& timeStep, const std::vector<std::string>& parameters, double lat, double lon, Matrix& result, std::vector<std::string>& times, std::string& msg, const std::vector<std::string>& optionals={}) const;
    
    //
    // -- query for several times at multiple points (several times, multiple points)
    //
    bool getMultiPointTimeSeries(const std::string& startTime, const std::string& stopTime, const std::string& timeStep, const std::vector<std::string>& parameters, std::vector<double> lats, std::vector<double> lons, std::vector<Matrix>& result, std::vector<std::string>& times, std::string& msg, const std::vector<std::string>& optionals={}) const;
    
    //
    // -- query for a single time and multiple points (one time, multiple points)
    //
    bool getMultiPoints(const std::string& time, const std::vector<std::string>& parameters, const std::vector<double>& lats, const std::vector<double>& lons, Matrix& result, std::string& msg, const std::vector<std::string>& optionals={}) const;

    //
    // -- returns an iso-date string for 6 ints (or for a vector with 6 ints)
    //
    std::string getIsoTimeStr(const int year, const int month, const int day, const int hour, const int min, const int sec) const;
    
    //
    // -- returns a string in the time step format as required by the Meteomatics API
    //
    std::string getTimeStepStr(const int year, const int month, const int day, const int hour, const int min, const int sec) const;
    
    //
    // -- return current year / month / day as int
    //
    int getCurrentYear() const;
    int getCurrentMonth() const;
    int getCurrentDay() const;
    int getTomorrow() const;
    int getTomorrowsMonth() const;
    int getTomorrowsYear() const;
    
    // free ressources
    ~MeteomaticsApiClient();
    
protected:

    static std::string createParameterListString(const std::vector<std::string>& parameters);

    static double round_coordinate(double c);
    static std::string createLatLonListString(const std::vector<double>& lats, const std::vector<double>& lons);
    static std::string createLatLonListString(const std::vector<double>& lats, const std::vector<double>& lons, const int res_lat, const int res_lon);

    static std::string getOptionalSelectString(const std::vector<std::string>& optionals);

    bool readSinglePointTimeSeriesBin(MMIntern::MemoryClass& mem, Matrix& results, std::vector<std::string>& times) const;
    bool readMultiPointTimeSeriesBin(MMIntern::MemoryClass& mem, std::vector<Matrix>& results, std::vector<std::string>& times) const;
    bool readGridAndMatrixFromMBG2Format(MMIntern::MemoryClass& mem, Matrix& results, std::vector<double>& lats, std::vector<double>& lons) const;

    void datevec(double time, double& year, double& month, double& day, double& hour, double& minute, double& second) const;
    std::string convDateIso8601(double date) const;

    const MMIntern::HttpClient* httpClient;

    const int dataRequestTimeout;

    const std::array<int,6> getCurrentTime() const;
    const std::array<int,6> addDayToToday(const int days) const;
};


#include "Meteomatics_Internals.h"


#endif /* MeteomaticsApiClient_h */
