#ifndef HELPERS_H
#define HELPER_H

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "../../config/dbconfig.cpp"

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>




//Data structure to hold rows read from dirty DB
struct Entry
{
	uint64_t timestamp;
	double value;
	std::string sensorName;
	int status;
};

//Data to be read from sensor table of dirty DB
struct SensorEntry
{
	int id;
	std::string name;
	std::string valuefacet;
};

//Returns vector of rows from dirty db
//Including the lower timestamp, excluding the upper timestamp
std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit);

//Appends data collected to a csv file
//Sensors in needed for translating human readable sensor names into the key from sensor table
//Returns number of written lines
int insertCleanDataInFile(std::vector<Entry> data, std::string filename, std::map<std::string, int> sensors);

//Inserts data collected in new db, meant for daily inserts, returns number of written lines
int insertCleanData(sql::Connection* cleanCon, std::vector<Entry> data, std::map<std::string, int> sensors);


//Returns time 10 minutes ago in millis
uint64_t getEpochUpperLimit();

//Returns entry with highest timestamp from cleandb
//If no entries exits, we return 0
uint64_t getLastEntryTimestamp(sql::Connection* cleanCon);

//Convert a double to scientific notation. Precision sets total number of digits.
//Ex: 1.54323e+10
std::string doubleToScientific(double d, int precision);

uint64_t millis_to_seconds(uint64_t i);

//Prints human readable time from millis
void printTimeFromMillis(uint64_t epochMillis);

//Returns map with sensorname and primarykey from sensortable in CLEANDB
std::map<std::string, int> getSensors(sql::Connection* cleanCon);

void initDbConnections(sql::Connection* dirtyCon, sql::Connection* cleanCon);

//Check if number of sensors match in old and new db, update new if necessary
void fillSensorTable(sql::Connection* dirtyCon, sql::Connection* cleanCon);

//Has to take epoch in seconds!
void printTimeFromSeconds(uint64_t epoch);

std::streambuf* startLog();

void endLog(std::streambuf* coutbuf);

bool sensorEntrySort(const SensorEntry& a, const SensorEntry& b);

#endif
