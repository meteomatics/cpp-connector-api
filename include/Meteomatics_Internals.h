//
//  Meteomatics_Internals.hpp
//  MeteomaticsApi
//
//  Created by Lukas Hammerschmidt on 17.10.17.
//  Copyright © 2018 Meteomatics. All rights reserved.
//

#ifndef Meteomatics_Internals_h
#define Meteomatics_Internals_h

#include <iomanip>
#include <cmath>
#include <sstream>
#include "curl/curl.h"
#include <iostream>
#include <string>
#include <assert.h>
#include <ctime>
#include <chrono>
#include <array>

namespace MMIntern {
    class MemoryClass;
    class HttpClient;
    
    bool http_code_success(int http_code);
    bool http_server_available(int http_code);   
}


//
//  METEOMATICS MEMORY CLASS
//
class MMIntern::MemoryClass
{
public:
    MemoryClass();
    MemoryClass(std::size_t initialCapacity);   // reserves an initial capacity (not resize)
    ~MemoryClass();
    
    void resetReadPos();                        // reading starts from beginning again
    
    std::size_t size() const;
    
    std::string readString(std::size_t _size);
    template<class T>
    std::size_t read(T& t);
    template<class T>
    std::size_t write(const T& t);
    template<class T>
    std::size_t readWithoutProceed(T& t);
    
    std::vector<char> mem;
    
private:
    std::size_t readPos;
    
    // WARNING: the operator>> doesn't provide any feedback whether data has been writen and the memory pointer moved, or not.
    template<class T>
    MemoryClass& operator>>(T& t);
    
    
    // WARNING: the operator<< doesn't provide any feedback whether data has been writen and the memory pointer moved, or not.
    template<class T>
    MemoryClass& operator<<(const T& t);
};

MMIntern::MemoryClass::MemoryClass()
: readPos(0)
{
}

MMIntern::MemoryClass::MemoryClass(std::size_t initialCapacity)
: readPos(0)
{
    mem.reserve(initialCapacity);
}

MMIntern::MemoryClass::~MemoryClass()
{
}

void MMIntern::MemoryClass::resetReadPos()
{
    readPos = 0;
}

std::size_t MMIntern::MemoryClass::size() const
{
    return mem.size();
}

std::string MMIntern::MemoryClass::readString(std::size_t _size)
{
    if (readPos + _size > mem.size())
    {
        _size = mem.size() - readPos;
    }
    std::string str((char*)(&mem[0] + readPos), _size);
    readPos += _size;
    return str;
}

template<class T>
std::size_t MMIntern::MemoryClass::read(T& t)
{
    *this >> t;
    return sizeof(T);
}

template<class T>
std::size_t MMIntern::MemoryClass::write(const T& t)
{
    *this << t;
    return sizeof(T);
}

template<class T>
std::size_t MMIntern::MemoryClass::readWithoutProceed(T& t)
{
    *this >> t;
    readPos -= sizeof(T);
    return sizeof(T);
}

template<class T>
MMIntern::MemoryClass& MMIntern::MemoryClass::operator>>(T& v)
{
    std::size_t typeSize = sizeof(T);
    
    if (readPos + typeSize > mem.size())                // in case reading exceeds mem blocks...
    {                                                   //   (maybe cout something)
        return *this;
    }
    
    std::copy(mem.begin() + readPos, mem.begin() + readPos + typeSize, reinterpret_cast<char*>(&v)); 
    readPos += typeSize;
    return *this;
}

template<class T>
MMIntern::MemoryClass& MMIntern::MemoryClass::operator<<(const T& v)
{
    std::size_t typeSize = sizeof(T);
    
    const char* contentPtr = reinterpret_cast<const char*>(&v);
    mem.insert(mem.end(), contentPtr, contentPtr+typeSize);
    return *this;
}
















//
//  METEOMATICS HTTP CLIENT
//

bool MMIntern::http_code_success(int http_code)
{
    return http_code >= 200 && http_code < 400;
}

bool MMIntern::http_server_available(int http_code)
{
    return http_code >= 200 && http_code < 500;
}

// Caveat: This HttpClient implementation is not thread-safe, due to libcurl
class MMIntern::HttpClient
{
public:
    HttpClient(const std::string& _url, const std::string& _user, const std::string& _password);
    ~HttpClient();
    
    static std::size_t writeMemoryCallback(void* contents, std::size_t size, std::size_t nmemb, void* userp);
    static std::size_t writeStringCallback(void *contents, std::size_t size, std::size_t nmemb, void *userp);
    
    std::size_t requestString(const std::string& url, const std::string& path, std::string& readBuffer, int timeout, int& http_code);
    std::size_t requestBinary(const std::string& url, const std::string& path, MemoryClass& mem, int timeout, int& http_code) const;
    
private:
    std::string server;
    std::string user;
    std::string password;
};


std::size_t MMIntern::HttpClient::writeMemoryCallback(void* contents, std::size_t size, std::size_t nmemb, void* userp)
{
    if (NULL == contents)
    {
        assert(!"HttpClient::writeMemoryCallback got NULL-pointer from libcurl");
        return 0;
    }
    
    MemoryClass* mem = static_cast<MemoryClass*>(userp);
    if (NULL == mem)
    {
        assert(!"HttpClient::writeMemoryCallback got NULL-pointer from user");
        return 0;
    }
    
    std::size_t realsize = size * nmemb;
    
    char* inCPtr = static_cast<char*>(contents);
    mem->mem.insert(mem->mem.end(), inCPtr, inCPtr+realsize);   // exceptions not caught...
    
    return realsize;
}

std::size_t MMIntern::HttpClient::writeStringCallback(void *contents, std::size_t size, std::size_t nmemb, void *userp)
{
    if (NULL == userp)
    {
        assert(!"HttpClient::writeMemoryCallback got NULL-pointer from user");
        return 0;
    }
    if (NULL == contents)
    {
        assert(!"HttpClient::writeMemoryCallback got NULL-pointer from libcurl");
        return 0;
    }
    
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

MMIntern::HttpClient::HttpClient(const std::string& _url, const std::string& _user, const std::string& _password)
: server(_url)
, user(_user)
, password(_password)
{
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if(res != CURLE_OK)
    {
        std::cout << "curl_global_init() failed: " << curl_easy_strerror(res) << std::endl;
        assert(false);
    }
}

MMIntern::HttpClient::~HttpClient()
{
    curl_global_cleanup();
}

std::size_t MMIntern::HttpClient::requestString(const std::string& url, const std::string& path, std::string& readBuffer, int timeout, int& http_code)
{
    http_code = 0;
    std::string query(url);
    query += path;
    
    readBuffer.clear();
    
    std::cout << "requesting string from " << query << std::endl;
    CURL* curl = curl_easy_init();
    if(curl)
    {
        struct curl_slist *headers=NULL; // init to NULL is important
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers );
        curl_easy_setopt(curl, CURLOPT_URL, query.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        if (!user.empty() && !password.empty())
        {
            const std::string auth=user+":"+password;
            curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());
        }
        
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            std::cout << "curl_easy_perform() to server " << url << " with query " << query <<" failed: " << curl_easy_strerror(res) << std::endl;
            return 0;
        }
        long l_http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &l_http_code);
        curl_easy_cleanup(curl);
        http_code = static_cast<int>(l_http_code);
        
        if (!http_server_available(http_code))
        {
            readBuffer.clear();
            std::cout << "Server " << url << " with query " << query <<" replied with code " << http_code << std::endl;
            return 0;
        }
        
        return readBuffer.length();
    }
    else
    {
        std::cout << "curl_easy_init failed, cannot query server " << url << " with query " << query << std::endl;
    }
    
    return 0;
}

std::size_t MMIntern::HttpClient::requestBinary(const std::string& url, const std::string& path, MemoryClass& memClass, int timeout, int& http_code) const
{
    http_code = 0;
    std::string query(url);
    query += path;
    memClass.resetReadPos();
    
    std::cout << "requesting binary from " << query << std::endl;
    CURL* curl = curl_easy_init();
    if(curl)
    {
        struct curl_slist *headers=NULL; // init to NULL is important
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers );
        curl_easy_setopt(curl, CURLOPT_URL, (query).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &memClass);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        if (!user.empty() && !password.empty())
        {
            const std::string auth=user+":"+password;
            curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());
        }
        
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            std::cout << "curl_easy_perform() to server " << url << " with query " << query <<" failed: " << curl_easy_strerror(res) << std::endl;
            return 0;
        }
        long l_http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &l_http_code);
        curl_easy_cleanup(curl);
        http_code = static_cast<int>(l_http_code);
        
        if (!http_server_available(http_code))
        {
            memClass.resetReadPos();
            std::cout << "Server " << url << " with query " << query <<" replied with code " << http_code << std::endl;
            return 0;
        }
        
        return memClass.size();
    }
    else
    {
        std::cout << "curl_easy_init failed, cannot query server " << url << " with query " << query << std::endl;
    }
    return 0;
}












//
//  METOMATICS API METHODS
//
MeteomaticsApiClient::MeteomaticsApiClient(const std::string& user, const std::string& password, const int timeout_seconds)
: httpClient(new MMIntern::HttpClient("api.meteomatics.com", user, password))
, dataRequestTimeout(timeout_seconds)
{
}

std::string MeteomaticsApiClient::getIsoTimeStr(const int year, const int month, const int day, const int hour, const int min, const int sec) const
{
    std::stringstream ss;
    ss << std::setprecision(4);
    ss << std::setw(4) << std::setfill('0') << year << '-';
    ss << std::setprecision(2);
    ss << std::setw(2) << std::setfill('0') << month << '-';
    ss << std::setw(2) << std::setfill('0') << day << 'T';
    ss << std::setw(2) << std::setfill('0') << hour << ':';
    ss << std::setw(2) << std::setfill('0') << min << ':';
    ss << std::setw(2) << std::setfill('0') << sec << 'Z';
    return ss.str();
}

std::string MeteomaticsApiClient::getTimeStepStr(const int year, const int month, const int day, const int hour, const int min, const int sec) const
{
    std::stringstream ss;
    if (year > 0)
        ss << year << "Y";
    if (month > 0)
        ss << month << "M";
    if (day > 0)
        ss << day << "D";
    if (hour+min+sec > 0)
        ss << "T";
    if (hour > 0)
        ss << hour << "H";
    if (min > 0)
        ss << min << "M";
    if (sec > 0)
        ss << sec << "S";
    if (ss.str() == "")
        return "0D";
    else
        return ss.str();
}

std::string MeteomaticsApiClient::getOptionalSelectString(const std::vector<std::string>& optionals)
{
    if (optionals.size() == 0)
    {
        return "";
    }
    
    std::string ret="?";
    for (std::size_t i=0; i<optionals.size(); i++)
    {
        ret += optionals[i];
        if (i != optionals.size()-1)
        {
            ret += "&";
        }
    }
    return ret;
}

bool MeteomaticsApiClient::readMultiPointTimeSeriesBin(MMIntern::MemoryClass& mem, std::vector<Matrix>& results, std::vector<std::string>& times) const
{
    int32_t nCoords;
    mem.read(nCoords);
    
    for (int32_t i=0; i<nCoords; i++)
    {
        int32_t nTimes;
        mem.read(nTimes);
        
        Matrix tmpMat;
        tmpMat.reserve(nTimes);
        for (int32_t j=0; j<nTimes; j++)
        {
            int32_t nParameter;
            mem.read(nParameter);
            
            double t;
            mem.read(t);
            times.push_back(convDateIso8601(t));
            
            std::vector<double> tmpValues;
            tmpValues.reserve(nParameter);
            for (int32_t k=0; k<nParameter; k++)
            {
                double value;
                mem.read(value);
                tmpValues.push_back(value);
            }
            tmpMat.push_back(tmpValues);
        }
        results.push_back(tmpMat);
    }
    
    return true;
}

bool MeteomaticsApiClient::readSinglePointTimeSeriesBin(MMIntern::MemoryClass& mem, Matrix& results, std::vector<std::string>& times) const
{
    int32_t numDates;
    int32_t numParams;
    mem.read(numDates);
    mem.readWithoutProceed(numParams);
    
    results.resize(numDates, std::vector<double>(numParams));
    times.resize(numDates);
    
    for (int i = 0; i < numDates; i++)
    {
        double date;
        mem.read(numParams);
        mem.read(date);
        times[i] = convDateIso8601(date);
        
        for (int j = 0; j < numParams; j++)
        {
            mem.read(results[i][j]);
        }
    }
    return true;
}

bool MeteomaticsApiClient::readGridAndMatrixFromMBG2Format(MMIntern::MemoryClass& mem, Matrix& results, std::vector<double>& lats, std::vector<double>& lons) const
{
    results.clear();
    if (mem.readString(sizeof(char)*4) != "MBG_")
    {
        std::cout << "ERROR. No MBG received" << std::endl;
        return false;
    }
    
    int32_t version;
    int32_t precision;
    int32_t numPayloadsPerForecast;
    int32_t payloadMeta;
    int32_t numForecasts;
    double forecastDateUx;
    int32_t numLat;
    int32_t numLon;
    
    mem.read(version);
    mem.read(precision);
    mem.read(numPayloadsPerForecast);
    mem.read(payloadMeta);
    mem.read(numForecasts);
    mem.read(forecastDateUx);
    
    if (version != 2)
    {
        std::cout << "only MBG version 2 supported, this is version " << version << std::endl;
        return false;
    }
    if (numPayloadsPerForecast > 100000)
    {
        std::cout << "WARNING! numForecasts too big (possibly big-endian): " << numPayloadsPerForecast << std::endl;
        return false;
    }
    if (numPayloadsPerForecast != 1)
    {
        std::cout << "WARNING! wrong number of payloads per forecast date received" << std::endl;
        return false;
    }
    if (payloadMeta != 0)
    {
        std::cout << "wrong payload type received: " << payloadMeta << std::endl;
        return false;
    }
    if (numForecasts != 1)
    {
        std::cout << "multiple validdates in mbg not yet supported in CacheClient" << std::endl;
        return false;
    }
    
    mem.read(numLat);
    lats.resize(static_cast<std::size_t>(numLat));
    
    for (auto& value : lats)
    {
        mem.read(value);
    }
    
    mem.read(numLon);
    lons.resize(static_cast<std::size_t>(numLon));
    for (auto& value : lons)
    {
        mem.read(value);
    }
    
    results.resize(numLat,std::vector<double>(numLon));
    
    if (precision == sizeof(float))
    {
        for (std::size_t i=0; i<lats.size(); i++)
        {
            for (std::size_t j=0; j<lons.size(); j++)
            {
                float tmpd;
                mem.read(tmpd);
                results[i][j] = static_cast<double>(tmpd);
            }
        }
    }
    else
    {
        for (std::size_t i=0; i<lats.size(); i++)
        {
            for (std::size_t j=0; j<lats.size(); j++)
            {
                mem.read(results[i][j]);
            }
        }
    }
    return true;
}

std::string MeteomaticsApiClient::createParameterListString(const std::vector<std::string>& parameters)
{
    std::string paramString("");
    for (std::size_t i = 0; i < parameters.size(); i++)
    {
        paramString += parameters[i];
        if (i < parameters.size()-1)
        {
            paramString +=",";
        }
    }
    return paramString;
}

std::string MeteomaticsApiClient::createLatLonListString(const std::vector<double>& lats, const std::vector<double>& lons)
{
    std::stringstream ss;
    const std::size_t numLat=lats.size();
    if (lons.size() != numLat)
    {
        std::cout << "Received different number of coordinates for lat and lon." << std::endl;
        return "";
    }
    
    for (std::size_t i = 0; i < numLat; i++)
    {
        ss << lats[i] << ',' << lons[i];
        if (i < numLat-1)
        {
            ss << "+";
        }
    }
    return ss.str();
}

std::string MeteomaticsApiClient::createLatLonListString(const std::vector<double>& lats, const std::vector<double>& lons, const int res_lat, const int res_lon)
{
    std::stringstream ss;
    const std::size_t numLat=lats.size();
    if (lons.size() != numLat and lons.size() > 2)
    {
        std::cout << "Received different number of coordinates for lat and lon or\n"
        "more than 2 coordinates. That is incompatible with grids." << std::endl;
        return "";
    }
    
    for (std::size_t i = 0; i < numLat; i++)
    {
        ss << lats[i] << ',' << lons[i];
        if (i < numLat-1)
        {
            ss << "_";
        }
    }
    ss << ":" << res_lat << "x" << res_lon;
    
    return ss.str();
}

bool MeteomaticsApiClient::getPoint(const std::string& time, const std::vector<std::string>& parameters, double lat, double lon, std::vector<double>& result, std::string& msg, const std::vector<std::string>& optionals) const
{
    std::string dummyStep = getTimeStepStr(0, 0, 0, 0, 0, 0);
    Matrix resultMatrix;
    std::vector<std::string> returnTimes;
    if (getTimeSeries(time, time, dummyStep, parameters, lat, lon, resultMatrix, returnTimes, msg, optionals))
    {
        result = resultMatrix[0];
    }
    else
        return false;
    
    return true;
}

bool MeteomaticsApiClient::getTimeSeries(const std::string& startTime, const std::string& stopTime, const std::string& timeStep, const std::vector<std::string>& parameters, double lat, double lon, Matrix& result, std::vector<std::string>& times, std::string& msg, const std::vector<std::string>& optionals) const
{
    result.clear();
    msg.clear();
    
    std::vector<Matrix> tmpM;
    if (getMultiPointTimeSeries(startTime, stopTime, timeStep, parameters, std::vector<double>(1,lat), std::vector<double>(1,lon), tmpM, times, msg, optionals))
    {
        result = tmpM[0];
    }
    else
    {
        return false;
    }
    
    return true;
}

bool MeteomaticsApiClient::getGrid(const std::string& time, const std::string& parameter, const double& lat_N, const double& lon_W, const double& lat_S, const double& lon_E, const int nGridPts_Lat, const int nGridPts_Lon, Matrix& gridResult, std::vector<double>& latGridPts, std::vector<double>& lonGridPts, std::string& msg, const std::vector<std::string>& optionals) const
{
    gridResult.clear();
    latGridPts.clear();
    lonGridPts.clear();
    msg.clear();
    
    std::string queryString = "/" + time
                            + "/" + parameter
                            + "/" + createLatLonListString(std::vector<double>{lat_N, lat_S}, std::vector<double>{lon_W, lon_E}, nGridPts_Lon, nGridPts_Lat)
                            + "/bin"
                            + getOptionalSelectString(optionals);
    
    int httpReturnCode = 0;
    
    MMIntern::MemoryClass mem(500);
    
    httpClient->requestBinary("api.meteomatics.com", queryString, mem, dataRequestTimeout, httpReturnCode);
    
    if (!MMIntern::http_code_success(httpReturnCode))
    {
        std::cout << "Http Error! Code: " << httpReturnCode;
        std::cout << ". For more information see returned msg string!" << std::endl;
        msg = mem.readString(mem.size());
        return false;
    }
    
    if (!readGridAndMatrixFromMBG2Format(mem, gridResult, latGridPts, lonGridPts))
    {
        std::cout << "Errror while reading grid and matrix MBG2 binary..." << std::endl;
        return false;
    }
    
    std::reverse(gridResult.begin(), gridResult.end());   // reverse => same order as in csv format
    std::reverse(latGridPts.begin(), latGridPts.end());
    
    return true;
}

bool MeteomaticsApiClient::getMultiPointTimeSeries(const std::string& startTime, const std::string& stopTime, const std::string& timeStep, const std::vector<std::string>& parameters, std::vector<double> lats, std::vector<double> lons, std::vector<Matrix>& result, std::vector<std::string>& times, std::string& msg, const std::vector<std::string>& optionals) const
{
    result.clear();
    msg.clear();
    
    std::string queryString = "/" + startTime + "--" + stopTime + ":P" + timeStep
    + "/" + createParameterListString(parameters)
    + "/" + createLatLonListString(lats, lons)
    + "/bin"
    + getOptionalSelectString(optionals);
    
    int httpReturnCode = 0;
    
    MMIntern::MemoryClass mem(500);
    
    httpClient->requestBinary("api.meteomatics.com", queryString, mem, dataRequestTimeout, httpReturnCode);
    
    if (!MMIntern::http_code_success(httpReturnCode))
    {
        std::cout << "Http Error! Code: " << httpReturnCode;
        std::cout << ". For more information see returned msg string!" << std::endl;
        msg = mem.readString(mem.size());
        return false;
    }
    
    if (lats.size() == 1)
    {
        Matrix tmpM;
        if (!readSinglePointTimeSeriesBin(mem, tmpM, times))
        {
            std::cout << "Error while reading mem-object." << std::endl;
            return false;
        }
        result.push_back(tmpM);
    }
    else
    {
        if (!readMultiPointTimeSeriesBin(mem, result, times))
        {
            std::cout << "Error while reading mem-object." << std::endl;
            return false;
        }
    }
    return true;
}

bool MeteomaticsApiClient::getMultiPoints(const std::string& time, const std::vector<std::string>& parameters, std::vector<double> lats, std::vector<double> lons, Matrix& result, std::string& msg, const std::vector<std::string>& optionals) const
{
    result.clear();
    std::vector<Matrix> tmpResults;
    std::vector<std::string> timeVec;
    if (getMultiPointTimeSeries(time, time, getTimeStepStr(0, 0, 0, 0, 0, 0), parameters, lats, lons, tmpResults, timeVec, msg, optionals))
    {
        result.resize(lats.size());
        for (std::size_t i = 0; i<lats.size(); i++)
        {
            result[i] = tmpResults[i][0];
        }
    }
    else
        return false;
    
    return true;
}

void MeteomaticsApiClient::datevec(double time,double &year,double &month,double &day,double &hour,double &minute,double &second) const
{
    /* Cumulative days per month in both nonleap and leap years. */
    double cdm0[] = {0,31,59,90,120,151,181,212,243,273,304,334,365};
    double cdml[] = {0,31,60,91,121,152,182,213,244,274,305,335,366};
    
    int leap=0,iy=0,mon=0;
    double *cdm;
    double yp=0,mo=0,d=0,h=0,mi=0,s=0;
    double t=0,ts=0,y=0,tmax = 1.2884901888e+11;
    
    /* tmax = 30*2^32*/
    
    year=0;
    month=0;
    day=0;
    hour=0;
    minute=0;
    second=0;
    
    t = time;
    if (std::fabs(t) > tmax)
    {
        std::cout<<"Error in datevec: Date number out of range: " << time<< std::endl;
        return;
    }
    
    if (t == std::floor(t))
    {
        s = mi = h = 0.;
    }
    else
    {
        t = 86400*t;
        t = std::floor(t+0.5);
        ts = t;
        t = std::floor(t/60.);
        s = ts - 60.*t;
        ts = t;
        t = std::floor(t/60.);
        mi = ts - 60.*t;
        ts = t;
        t = std::floor(t/24.);
        h = ts - 24.*t;
    }
    
    
    t = std::floor(t);
    
    if (t == 0)
    {
        yp = mo = d = 0;
    }
    else
    {
        y = std::floor(t/365.2425);
        ts = t - (365.0*y + std::ceil(0.25*y)-std::ceil(0.01*y)+std::ceil(0.0025*y));
        if (ts <= 0)
        {
            y = y - 1.;
            t = t - (365.0*y + std::ceil(0.25*y)-std::ceil(0.01*y)+std::ceil(0.0025*y));
        }
        else
        {
            t = ts;
        }
        
        yp = y;
        iy = (int) y;
        leap = (((iy%4 == 0) && (iy%100 != 0)) || (iy%400 == 0));
        cdm = (leap ? cdml : cdm0);
        mon = int( t/29.-1);
        
        if (t > cdm[mon+1])
            mon++;
        
        mo = mon+1;
        t = t - cdm[mon];
        d = t;
    }
    
    
    year = yp;
    month = mo;
    day = d;
    hour = h;
    minute = mi;
    second = s;
    
    return;
}

std::string MeteomaticsApiClient::convDateIso8601(double date) const
{
    double year=0,month=0,day=0,hour=0,min=0,sec=0;
    datevec(date,year,month,day,hour,min,sec);
    
    return getIsoTimeStr(year, month, day, hour, min, sec);
}

int MeteomaticsApiClient::getCurrentYear() const
{
    return getCurrentTime()[0];
}
int MeteomaticsApiClient::getCurrentMonth() const
{
    return getCurrentTime()[1];
}
int MeteomaticsApiClient::getCurrentDay() const
{
    return getCurrentTime()[2];
}

const std::array<int,6> MeteomaticsApiClient::getCurrentTime() const
{
    std::array<int,6> current_time;
    
    std::time_t ltime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *Tm;
    
    Tm=gmtime(&ltime);
    
    current_time[0] = Tm->tm_year+1900;
    current_time[1] = Tm->tm_mon+1;
    current_time[2] = Tm->tm_mday;
    current_time[3] = Tm->tm_hour;
    current_time[4] = Tm->tm_min;
    current_time[5] = Tm->tm_sec;
    
    return current_time;
}

const std::array<int,6> MeteomaticsApiClient::addDayToToday(const int days) const
{
    std::time_t ltime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *Tm;
    
    Tm=std::gmtime(&ltime);
    
    Tm->tm_mday += days;
    std::time_t later_time = mktime(Tm);
    Tm=std::gmtime(&later_time);
    
    std::array<int,6> tomorrows_time;
    tomorrows_time[0] = Tm->tm_year+1900;
    tomorrows_time[1] = Tm->tm_mon+1;
    tomorrows_time[2] = Tm->tm_mday;
    tomorrows_time[3] = Tm->tm_hour;
    tomorrows_time[4] = Tm->tm_min;
    tomorrows_time[5] = Tm->tm_sec;
    
    return tomorrows_time;
}

int MeteomaticsApiClient::getTomorrow() const
{
    return addDayToToday(1)[2];
}
int MeteomaticsApiClient::getTomorrowsMonth() const
{
    return addDayToToday(1)[1];
}
int MeteomaticsApiClient::getTomorrowsYear() const
{
    return addDayToToday(1)[0];
}

MeteomaticsApiClient::~MeteomaticsApiClient()
{
    delete httpClient;
}

#endif /* Meteomatics_Internals_h */

