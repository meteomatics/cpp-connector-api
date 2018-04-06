//
//  meteomatics_main.cpp
//  MeteomaticsApi
//
//  Created by Lukas Hammerschmidt on 17.10.17.
//  Copyright © 2018 Meteomatics. All rights reserved.
//


//
// Caveat: The MeteomaticsApiClient implementation is not thread-safe, due to libcurl!
//

#include "Meteomatics_ApiClient.h"

#include <iostream>


using namespace std;


int main(int argc, char* argv[])
{
    bool success = false;
    
    cout<<"------------------------------------------------------\n";
    cout<<"Compile-Time: "<<__DATE__<<" "<<__TIME__<<"\n";
    cout<<"------------------------------------------------------" << endl;

    if (argc != 3)
    {
        cout << "Usage: ./meteomatics USERNAME PASSWORD" << endl;
        return 0;
    }
    
    const string user = argv[1];
    const string password = argv[2];
    cout<<"User: " << user << ", Password: " << password << endl << endl;
    
    const int timeout=300;
    MeteomaticsApiClient api_client(user, password, timeout);
    
    
    // Parameters
    //
    // Parameters are handled as strings (or vector<string>s) as described in the
    //   documentation: http://api.meteomatics.com/Available-Parameters.html
    //
    // e.g.:
    std::vector<std::string> parameters = {"t_2m:C", "t_0m:C", "msl_pressure:hPa"};
    
    
    // Times
    //
    // The cpp-connector takes times as iso-strings as shown in the documentation:
    //   http://api.meteomatics.com/API-Request.html#time-description
    // For convenience, the MeteomaticsApiClient provides a function to create
    //   iso-time strings from 6 digits (no check is performed at this time if a
    //   valid date was entered):
    //   getIsoTimeStr  --> which takes either 6 integers
    //
    // e.g.:
    int year = api_client.getCurrentYear();
    int month = api_client.getCurrentMonth();
    int day = api_client.getCurrentDay();
    std::string singleTime = api_client.getIsoTimeStr(year, month, day, 0, 0, 0);
    
    
    // Coordinate grid
    //
    // coordinates are provided as type double
    //
    // e.g.:
    double lat_N = 50;
    double lat_S = 20;
    double lon_W = -15;
    double lon_E = 10;
    int nLatPts = 50;           // number of points for coordinate grid
    int nLonPts = 100;
    
  
    // Optional Parameters
    //
    // can be provided as strings of the option followed by the value, see documentation:
    //   http://api.meteomatics.com/API-Request.html
    //
    // e.g.
    std::vector<std::string> optionals = {"model=mix"};
    
    
    
    std::string msg;            // returns http error messages, should there be one -- empty otherwise
    
    
    
    // ---------------------------
    // query examples start here:
    //----------------------------

    
    //
    // Single Point  (single coordinate, single time, one or more parameters)
    //
    std::vector<double> resultVector;
    singleTime = api_client.getIsoTimeStr(year, month, day, 0, 0, 0);
    
    success = api_client.getPoint(singleTime, parameters, lat_N, lon_E, resultVector, msg, optionals);

    if (success)
    {
        std::cout << "Single Point Result:" << std::endl;
        for (std::size_t i=0; i<parameters.size(); ++i)
        {
            std::cout << parameters[i] << " = " << resultVector[i] << std::endl;
        }
        std::cout << std::endl;
        success = false;
    }
    else
        std::cout << "Error msg = " << msg.substr(0,500) << "[...]" << std::endl << std::endl;

    
    
    //
    // Time Series  (single coordinate, time span, one or more parameters)
    //
    int tomorrowsYear = api_client.getTomorrowsYear();
    int tomorrowsMonth = api_client.getTomorrowsMonth();
    int tomorrow = api_client.getTomorrow();
    
    std::string startTime = api_client.getIsoTimeStr(year, month, day, 0, 0, 0);
    std::string endTime = api_client.getIsoTimeStr(tomorrowsYear, tomorrowsMonth, tomorrow, 0, 0, 0);
    std::string timeStep = api_client.getTimeStepStr(0, 0, 0, 1, 0, 0);
    std::vector<std::string> returnTimes;
    Matrix resultMatrix;                                  // resultMatrix [time][parameter]

    success = api_client.getTimeSeries(startTime, endTime, timeStep, parameters, lat_N, lon_E, resultMatrix, returnTimes, msg);
    
    if (success)
    {
        std::cout << "Time Series Result: " << std::endl;
        std::cout << returnTimes[0] << " ";
        for (const auto& i : resultMatrix[0])
            std::cout << i << " ";
        std::cout << std::endl;
        std::cout << returnTimes[1] << " ";
        for (const auto& i : resultMatrix[1])
            std::cout << i << " ";
        std::cout << std::endl << std::endl;
        success = false;
    }
    else
        std::cout << "Error msg = " << msg.substr(0,500) << "[...]" << std::endl << std::endl;

    
    //
    // Grids (coordinates on a grid, single time, one parameter)
    //
    std::vector<double> latGridPts, lonGridPts;
    Matrix gridResult;                                      // gridResult [lat][lon]
    
    success = api_client.getGrid(singleTime, parameters[0], lat_N, lon_W, lat_S, lon_E, nLatPts, nLonPts, gridResult, latGridPts, lonGridPts, msg);
    
    if (success)
    {
        std::cout << "Grid Result (1 entry shown): " << std::endl;
        std::cout << "(" << latGridPts[0] << "," << lonGridPts[0] << ")  " << gridResult[0][0] << std::endl;
        std::cout << "Got " << gridResult.size() << " Lat grid points." << std::endl;
        std::cout << "Got " << gridResult[0].size() << " Lon grid points." << std::endl << std::endl;
        success = false;
    }
    else
        std::cout << "Error msg = " << msg.substr(0,500) << "[...]" << std::endl << std::endl;
    
    
    //
    // MultiTimePoints (multiple coordinates, multiple time, one or more parameters)
    //
    startTime = api_client.getIsoTimeStr(year,month,day,0,0,0);
    endTime = api_client.getIsoTimeStr(tomorrowsYear,tomorrowsMonth,tomorrow,0,0,0);
    timeStep = api_client.getTimeStepStr(0,0,0,6,0,0);
    std::vector<double> lats = std::vector<double>{45.84, 47.41, 47.51, 47.13};
    std::vector<double> lons = std::vector<double>{6.86, 9.35, 8.74, 8.22};
    
    std::vector<Matrix> mtpResults;                          // mtpResults [coord,lat,lon][time][par]
    std::vector<std::string> mtpTimes;

    success = api_client.getMultiPointTimeSeries(startTime, endTime, timeStep, parameters, lats, lons, mtpResults, mtpTimes, msg);
    
    if (success)
    {
        std::cout << "Multi Point Time Series Result: " << std::endl;
        for (std::size_t coor=0; coor<lats.size(); coor++)
        {
            for (std::size_t tms=0; tms<mtpTimes.size()/lats.size(); tms++)
            {
                for (std::size_t para=0; para<parameters.size(); para++)
                {
                    std::cout << "(" << lats[coor] << "," << lons[coor] << ")  " << mtpTimes[tms] << "  " <<  parameters[para] << "  " << mtpResults[coor][tms][para] << std::endl;
                }
            }
        }
        success = false;
        std::cout << std::endl;
    }
    else
        std::cout << "Error msg = " << msg.substr(0,500) << "[...]" << std::endl << std::endl;
    
    
    //
    // MultiPoints (multiple coordinates, single time, one or more parameters)
    //
    lats = std::vector<double>{45.84, 47.41, 47.51, 47.13};
    lons = std::vector<double>{6.86, 9.35, 8.74, 8.22};
    
                                                    // resultMatrix[lats,lons,coord][parameter]
    success = api_client.getMultiPoints(startTime, parameters, lats, lons, resultMatrix, msg);
    
    if (success)
    {
        std::cout << "Multi Points Result: " << std::endl;
        for (std::size_t i=0; i<lats.size(); i++)
        {
            std::cout << "(" << lats[i] << "," << lons[i] << ")  ";
            for (const auto& par : resultMatrix[i])
                std::cout << par << "  ";
            std::cout << std::endl;
        }
        success = false;
    }
    else
        std::cout << "Error msg = " << msg.substr(0,500) << "[...]" << std::endl << std::endl;

    return 0;
}
