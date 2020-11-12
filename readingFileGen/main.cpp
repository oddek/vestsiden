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
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>



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


std::string doubleToScientific(double d, int precision);

void insertCleanDataInFile(sql::Connection* cleanCon, std::vector<Entry> data, std::string filename);

uint64_t millis_to_seconds(uint64_t i);

//Prints time from millis
void printTimeFromMillis(uint64_t epochMillis);

//Has to take epoch in seconds!
void printTimeFromSeconds(uint64_t epoch);

//Returns in seconds, NOT millis
uint64_t getLastEntryTimestamp(sql::Connection* cleanCon);

std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit);

//Returns time 10 minutes ago in SECONDS
uint64_t getEpochUpperLimit();


std::map<std::string, int> getSensors(sql::Connection* cleanCon);

int main()
{

	std::string filename = "../loadfiles/readings.csv";

	//Redirect cout to file
	/* std::ofstream out("out.txt", std::fstream::app); */
	/* std::streambuf* coutbuf = std::cout.rdbuf(); */
	/* std::cout.rdbuf(out.rdbuf()); */
    auto start = std::chrono::system_clock::now();
	std::time_t start_time = std::chrono::system_clock::to_time_t(start);

	std::cout << "\n\nLog insert. Time: " << std::ctime(&start_time) << "\n";
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
		//Find the last timestamp registered in the clean database
		auto lastEntryTime = getLastEntryTimestamp(cleanConnection);
		std::cout << "Got last entry time:\n";
		printTimeFromSeconds(lastEntryTime);

		/* uint64_t selectLowerLimit = (lastEntryTime + 1) * 1000; */
		uint64_t selectLowerLimit = 1581845340000;

		/* uint64_t selectUpperLimit = 1581845640000; //getEpochUpperLimit(); */
		/* uint64_t selectUpperLimit = 1581845760000; //getEpochUpperLimit(); */
		uint64_t selectUpperLimit = getEpochUpperLimit() * 1000;
		std::cout << "Got upper limit for select:\n";
		printTimeFromMillis(selectUpperLimit);
		std::cout << "Will create csv, with\n\tLower limit: " << selectLowerLimit << "\n\tUpper limit: " << selectUpperLimit << "\n";


		uint64_t currentLower = selectLowerLimit;

		uint64_t incrementValue = 20000000;
		uint64_t currentUpper = currentLower + incrementValue;

		if(currentUpper > selectUpperLimit)
		{
			currentUpper = selectUpperLimit;
		}

		while(true)
		{



			auto t1 = std::chrono::high_resolution_clock::now();
			//Get X amount of data.
			std::cout << "Fetching data..\n";
			auto data = getLatestDirtyData(dirtyConnection, currentLower, currentUpper);
			//If there isn't any more new data, we are done
			if(data.empty())
			{
				std::cout << "Data empty\n";
				break;
			}
			//Insert clean data into database.
			std::cout << "Inserting data..\n";
			insertCleanDataInFile(cleanConnection, data, filename);
			auto t2 = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::seconds>( t2 - t1 ).count();
			std::cout << duration << "\n";

			currentLower = currentUpper;
			currentUpper += incrementValue;

			if(currentUpper > selectUpperLimit)
			{
				currentUpper = selectUpperLimit;
			}

		}
		return 0;
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
		std::cout << "Exit on error";
		/* std::cout.rdbuf(coutbuf); */
		return -1;
	}
	std::cout << "Clean exit\n";
	/* std::cout.rdbuf(coutbuf); */
	return 0;
}


void insertCleanDataInFile(sql::Connection* cleanCon, std::vector<Entry> data, std::string filename)
{
	auto sensors = getSensors(cleanCon);

	std::ofstream file(filename, std::fstream::app);

	std::cout << "writing to file: " << filename << "\n";
	std::cout << data.size() << " lines\n";
    for(auto& c : data)
    {
        int sensorId = sensors.at(c.sensorName);
        file << sensorId << ",";
        file << millis_to_seconds(c.timestamp) << ",";
        file << doubleToScientific(c.value, 5) << ",";
        file << c.status << "\n";
    }
    file.close();
}


std::map<std::string, int> getSensors(sql::Connection* cleanCon)
{
	sql::Statement* stmt;

	sql::ResultSet* res;
	stmt = cleanCon->createStatement();
	std::string query = "SELECT `ID`, `NAME` FROM `HISTORY_TYPE_MAP`";
	res = stmt->executeQuery(query);

	std::map<std::string, int> sensors;
	while(res->next())
	{
		int id = res->getInt("ID");
		std::string name = res->getString("NAME");

		sensors.insert(std::pair<std::string, int>(name, id));
	}

	delete stmt;
	return sensors;
}


std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit)
{
	std::cout << "lower: " << timestampLowerLimit << "\n";
	std::cout << "upper: " << timestampUpperLimit << "\n";
	//Gjetning
    std::vector<Entry> newData;
	sql::ResultSet* res;
    sql::PreparedStatement* stmt;

	stmt = dirtyCon->prepareStatement
	(
		"SELECT `TIMESTAMP`, `VALUE`, `HISTORY_ID`, `STATUS` FROM "
			"HISTORYNUMERICTRENDRECORD "
		"WHERE "
			"`TIMESTAMP` >= (?) AND `TIMESTAMP` < (?) "
	);


	stmt->setUInt64(1, timestampLowerLimit);
	stmt->setUInt64(2, timestampUpperLimit);

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

	std::cout << "Got new data\n";

	delete res;
	delete stmt;

    return newData;
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

std::string doubleToScientific(double d, int precision)
{
	std::ostringstream s;
	s << std::scientific;
	s << std::setprecision(precision);
	s << d;
	return s.str();
}











