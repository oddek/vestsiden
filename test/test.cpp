#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "../config/dbconfig.cpp"


#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>




//Data to be read from dirty DB
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
	std::string name;
	std::string valuefacet;
};

double round(double d);

uint64_t millis_to_seconds(uint64_t i);

void fillSensorTable(sql::Connection* dirtyCon, sql::Connection* cleanCon);

//Prints time from millis
void printTimeFromMillis(uint64_t epochMillis);

//Has to take epoch in seconds!
void printTimeFromSeconds(uint64_t epoch);

//Returns in seconds, NOT millis
uint64_t getLastEntryTimestamp(sql::Connection* cleanCon);

std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit);

//Insert vector of CleanEntries into clean database
bool insertCleanData(sql::Connection* cleanCon, std::vector<Entry> data);

//Returns time 10 minutes ago in SECONDS
uint64_t getEpochUpperLimit();



int main()
{
	//Init connectionstrings
	std::string sourceConnectionString = "tcp://" + sourceDbHostNameV6 + ":" + sourceDbPort;
	std::string destConnectionString = "tcp://" + destDbHostNameV6 + ":" + destDbPort;
	try
	{
		std::cout << "Connecting to databases..\n";
		std::cout << sourceConnectionString << "\n";
		//Init database connections
		sql::Driver* driver = get_driver_instance();
		sql::Connection* dirtyConnection;
		sql::Connection* cleanConnection;
		dirtyConnection = driver->connect(sourceConnectionString, sourceDbUsername, sourceDbPassword);
		cleanConnection = driver->connect(destConnectionString, destDbUsername, destDbPassword);
		std::cout << "Databases connected\n";

		dirtyConnection->setSchema("vestsiden");
		cleanConnection->setSchema("vestsiden");
		//Refill sensor table, in case there are some new sensors added
		fillSensorTable(dirtyConnection, cleanConnection);
		std::cout << "SensorTable filled\n";
		//Find the last timestamp registered in the clean database
		auto lastEntryTime = getLastEntryTimestamp(cleanConnection);
		std::cout << "Got last entry time:\n";
		printTimeFromSeconds(lastEntryTime);


		uint64_t selectLowerLimit = (lastEntryTime + 1) * 1000;
		std::cout << "Got lower limit for select:\n";
		printTimeFromSeconds(selectLowerLimit);


		/* uint64_t selectUpperLimit = 1581845640000; //getEpochUpperLimit(); */
		/* uint64_t selectUpperLimit = 1581845760000; //getEpochUpperLimit(); */
		uint64_t selectUpperLimit = 1588330587000; //getEpochUpperLimit();
		std::cout << "Got upper limit for select:\n";
		printTimeFromSeconds(selectUpperLimit);
		std::cout << "Will update db, with\n\tLower limit: " << selectLowerLimit << "\n\tUpper limit: " << selectUpperLimit << "\n";

		//While there still is new data in the dirty database
		while(true)
		{
			//Get X amount of data.
			std::cout << "Fetching data..\n";
			auto data = getLatestDirtyData(dirtyConnection, selectLowerLimit, selectUpperLimit);
			//If there isn't any more new data, we are done
			if(data.empty()) break;
			//Insert clean data into database.
			std::cout << "Inserting data..\n";
			insertCleanData(cleanConnection, data);
		}

		//Memory cleanup (not really necessary)
		std::cout << "Done, cleaning up..\n";
    	delete dirtyConnection;
		delete cleanConnection;

	}
	catch(sql::SQLException& e)
	{
		std::cout << "# ERR: SQLException in " << __FILE__;
	    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
	    std::cout << "# ERR: " << e.what();
	    std::cout << " (MySQL error code: " << e.getErrorCode();
	    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
	}
	std::cout << "Clean exit\n";
	return 0;


  return 0;
}

//Insert vector of CleanEntries into clean database
bool insertCleanData(sql::Connection* cleanCon, std::vector<Entry> data)
{
	for(auto c : data)
	{
		std::cout << "Sensor: " << c.sensorName << "\n";
	}
	sql::PreparedStatement* prep_stmt;

	for(auto c : data)
	{
		prep_stmt = cleanCon->prepareStatement
		(
			"INSERT INTO `HISTORYNUMERICTRENDRECORD` "
				"( "
					"`TIMESTAMP`, "
					"`VALUE`, "
					"`HISTORY_ID`, "
					"`STATUS` "
				") "
			"VALUES "
				"(?, ?, (SELECT `ID` FROM HISTORY_TYPE_MAP WHERE `NAME` = ?), ?) "
		);

		prep_stmt->setUInt(1, millis_to_seconds(c.timestamp));
		prep_stmt->setDouble(2, round(c.value));
		prep_stmt->setString(3, c.sensorName);
		prep_stmt->setInt(4, c.status);
		prep_stmt->execute();
	}

	delete prep_stmt;
	return true;
}


std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit)
{
	//Gjetning
	const unsigned int maxNumOfRows = 10;
	static unsigned int currentRowOffset = 0;

    std::vector<Entry> newData;
	sql::ResultSet* res;
    sql::PreparedStatement* stmt;

	stmt = dirtyCon->prepareStatement
	(
		"SELECT `TIMESTAMP`, `VALUE`, `HISTORY_ID`, `STATUS` FROM "
			"HISTORYNUMERICTRENDRECORD "
		"WHERE "
			"`TIMESTAMP` >= (?) AND `TIMESTAMP` < (?) "
		"ORDER BY `TIMESTAMP` ASC LIMIT ?, ? "
	);


	stmt->setUInt64(1, timestampLowerLimit);
	stmt->setUInt64(2, timestampUpperLimit);
	stmt->setUInt(3, currentRowOffset);
	stmt->setUInt(4, maxNumOfRows);

	res = stmt->executeQuery();

	while(res->next())
	{
		Entry d;
		d.timestamp = res->getUInt64("TIMESTAMP");
		d.value = res->getDouble("VALUE");
		d.sensorName = res->getString("HISTORY_ID");
		d.status = res->getInt("STATUS");
		newData.push_back(d);
	}

	std::cout << "Got new data, offset: " << currentRowOffset << "\n";

	currentRowOffset += maxNumOfRows;

	delete res;
	delete stmt;

    return newData;
}



//TESTED, WORKS!
void fillSensorTable(sql::Connection* dirtyCon, sql::Connection* cleanCon)
{
	std::vector<SensorEntry> entries;

	sql::Statement* stmt;
	sql::ResultSet* res;
	sql::PreparedStatement* prep_stmt;

	//Check status max value
	/* stmt = dirtyCon->createStatement(); */
	/* std::string query1 = "SELECT MAX(STATUS) as 'MAXSTATUS' FROM HISTORYNUMERICTRENDRECORD"; */
	/* res = stmt->executeQuery(query1); */
	/* res->next(); */
	/* std::cout << "status max value: " << res->getInt("MAXSTATUS") << "\n"; */

	/* //Check status low value */
	/* stmt = dirtyCon->createStatement(); */
	/* std::string query2 = "SELECT MIN(STATUS) as 'MINSTATUS' FROM HISTORYNUMERICTRENDRECORD"; */
	/* res = stmt->executeQuery(query2); */
	/* res->next(); */
	/* std::cout << "status min value: " << res->getInt("MAXSTATUS") << "\n"; */

	//Get count of sensors from dirtyDb
	stmt = dirtyCon->createStatement();
	std::string query = "SELECT COUNT(*) AS CNT FROM `HISTORY_TYPE_MAP`";
	res = stmt->executeQuery(query);
	res->next();
	int dirtySensorCount = res->getInt("CNT");

	//Get count of sensors from cleanDb
	stmt = cleanCon->createStatement();
	query = "SELECT COUNT(*) AS CNT FROM `HISTORY_TYPE_MAP`";
	res = stmt->executeQuery(query);
	res->next();
	int cleanSensorCount = res->getInt("CNT");

	std::cout << "Dirty sensor count: " << dirtySensorCount << "\n";
	std::cout << "Clean sensor count: " << cleanSensorCount << "\n";

	//If sensor counts are equal, there is no need to update
	if(dirtySensorCount == cleanSensorCount)
	{
		std::cout << "Number of sensors equal, no need to update\n";
		return;
	}

	std::cout << "Number of sensors are NOT equal, updating cleanDb..\n";

	stmt = dirtyCon->createStatement();
	query = "SELECT `ID_`, `VALUEFACETS` FROM `HISTORY_TYPE_MAP`";
	res = stmt->executeQuery(query);

	while(res->next())
	{
		SensorEntry s;
		s.name = res->getString("ID_");
		s.valuefacet = res->getString("VALUEFACETS");

		entries.push_back(s);
	}

	std::cout << "Size of entries: " << entries.size() << "\n";

	for(auto& s : entries)
	{
		prep_stmt = cleanCon->prepareStatement
		(
			"INSERT IGNORE INTO "
				"`HISTORY_TYPE_MAP`"
					"(`NAME`, `VALUEFACETS`) "
				"VALUES (?, ?) "
		);

		prep_stmt->setString(1, s.name);
		prep_stmt->setString(2, s.valuefacet);

		prep_stmt->execute();
	}

	delete stmt;
	delete res;
	delete prep_stmt;
}


//Returns in seconds, NOT millis
uint64_t getLastEntryTimestamp(sql::Connection* cleanCon)
{
	sql::Statement* stmt;
	sql::ResultSet* res;

	stmt = cleanCon->createStatement();

	std::string query = "SELECT COALESCE(MAX(TIMESTAMP), 0) as 'LastEntry' FROM HISTORYNUMERICTRENDRECORD";

	//Get highest timestamp in CleanDB;
	res = stmt->executeQuery(query);
	//Init to 1.1.1970:
	uint64_t lastEntry = 0;
	//If we got result, overwrite
	while(res->next())
	{
		std::cout << "Time from db: " << res->getUInt64("LastEntry") <<"\n";
		lastEntry  = res->getUInt64("LastEntry");
	}

	delete stmt;
	delete res;
	return lastEntry;
}

//Returns time 10 minutes ago in SECONDS
uint64_t getEpochUpperLimit()
{
	//Get time
	auto t = std::chrono::system_clock::now();
	//Convert to millis and uint32
	uint64_t limit = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();

	limit -= 10*60;

	return limit;
}


//Has to take epoch in millis
void printTimeFromMillis(uint64_t epochMillis)
{
	printTimeFromSeconds(epochMillis/1000);
}

//Has to take epoch in seconds!
void printTimeFromSeconds(uint64_t epoch)
{
	time_t t = epoch;

	std::cout << "\t" << epoch << "\n\t" << std::put_time(std::localtime(&t), "%Y/%m/%d %T") << "\n";
}

uint64_t millis_to_seconds(uint64_t i)
{
	return (i / 1000);
}

double round(double d)
{
	double v = (int)(d * 100 + .5);
	return (double)v / 100;
}













