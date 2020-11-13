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

//Data structure to hold rows read from dirty DB
struct Entry
{
	uint64_t timestamp;
	double value;
	std::string sensorName;
	int status;
};

//Returns vector of rows from dirty db
//Including the lower timestamp, excluding the upper timestamp
std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit);

//Appends data collected to a csv file
//Sensors in needed for translating human readable sensor names into the key from sensor table
void insertCleanDataInFile(std::vector<Entry> data, std::string filename, std::map<std::string, int> sensors);

//Returns time 10 minutes ago in millis
uint64_t getEpochUpperLimit();

//Convert a double to scientific notation. Precision sets total number of digits.
//Ex: 1.54323e+10
std::string doubleToScientific(double d, int precision);

uint64_t millis_to_seconds(uint64_t i);

//Prints human readable time from millis
void printTimeFromMillis(uint64_t epochMillis);

//Returns map with sensorname and primarykey from sensortable in CLEANDB
std::map<std::string, int> getSensors(sql::Connection* cleanCon);

int totalLinesWritten = 0;

int main()
{
	std::string filename = "../loadfiles/readings2.csv";

	//Redirect cout to file
	std::ofstream out("out.txt", std::fstream::app);
	std::streambuf* coutbuf = std::cout.rdbuf();
	std::cout.rdbuf(out.rdbuf());

	//Start time for log
    auto start = std::chrono::system_clock::now();
	std::time_t start_time = std::chrono::system_clock::to_time_t(start);
	std::cout << "\n\nLog insert. Time: " << std::ctime(&start_time) << "\n";

	//Set connectionstrings
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
		dirtyConnection->setSchema("vestsiden");
		cleanConnection->setSchema("vestsiden");
		std::cout << "Databases connected\n";

		//Earliest sensor reading in dirty db
		const uint64_t fileInsertLowerLimit = 1581845340000;

		//Ten minutes ago at runtime
		uint64_t fileInsertUpperLimit = getEpochUpperLimit();
		/* uint64_t fileInsertUpperLimit = 1581849340000; */
		std::cout << "Got upper limit for select:\n";
		printTimeFromMillis(fileInsertUpperLimit);
		std::cout << "Will create csv, with\n\tLower limit: " << fileInsertLowerLimit << "\n\tUpper limit: " << fileInsertUpperLimit << "\n";


		//Dont want to select more than about 500 000 rows at the time, so we need fill the file incrementally
		uint64_t currentLower = fileInsertLowerLimit;
		const uint64_t incrementValue = 40000000;
		uint64_t currentUpper = currentLower + incrementValue;
		if(currentUpper > fileInsertUpperLimit)
		{
			currentUpper = fileInsertUpperLimit;
		}

		//Get sensors for translating sensorname to ID
		auto sensors = getSensors(cleanConnection);

		//Loop until the file has been filled with all rows from dirty db
		while(currentLower < fileInsertUpperLimit)
		{
			auto t1 = std::chrono::high_resolution_clock::now();
			std::cout << "Fetching data..\n";
			auto data = getLatestDirtyData(dirtyConnection, currentLower, currentUpper);

			//If there isn't any more new data, we are done
			/* if(data.empty()) */
			/* { */
			/* 	std::cout << "Data empty\n"; */
			/* 	break; */
			/* } */

			//Insert clean data into database.
			std::cout << "Inserting data..\n";
			insertCleanDataInFile(data, filename, sensors);

			//Increment timestamps for select
			currentLower = currentUpper;
			currentUpper += incrementValue;

			//We dont want to read more than what was there ten minutes ago
			if(currentUpper > fileInsertUpperLimit)
			{
				currentUpper = fileInsertUpperLimit;
			}

			//Print time loop took
			auto t2 = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::seconds>( t2 - t1 ).count();
			std::cout << duration << "\n";
			std::cout << "Total lines written: " << totalLinesWritten << "\n";
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
		std::cout << "Exit on error";
		std::cout.rdbuf(coutbuf);
		return -1;
	}
	std::cout << "Clean exit\n";
	std::cout.rdbuf(coutbuf);

    start = std::chrono::system_clock::now();
	start_time = std::chrono::system_clock::to_time_t(start);
	std::cout << "\n\nProgram finished at: " << std::ctime(&start_time) << "\n";
	return 0;
}

std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit)
{
	std::cout << "lower: " << timestampLowerLimit << "\n";
	std::cout << "upper: " << timestampUpperLimit << "\n";

    std::vector<Entry> newData;
	sql::ResultSet* res;
    sql::PreparedStatement* stmt;

	stmt = dirtyCon->prepareStatement
	(
		"SELECT "
			"`TIMESTAMP`, `VALUE`, `HISTORY_ID`, `STATUS` "
		"FROM "
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

void insertCleanDataInFile(std::vector<Entry> data, std::string filename, std::map<std::string, int> sensors)
{
	//Open file in append mode
	std::ofstream file(filename, std::fstream::app);

	std::cout << "Writing " << data.size() << " lines to file\n";

	totalLinesWritten += data.size();
    for(auto& c : data)
    {
		//Find sensor id from sensorname
        int sensorId = sensors.at(c.sensorName);
		//Insert to file
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

uint64_t getEpochUpperLimit()
{
	//Get time
	auto t = std::chrono::system_clock::now();
	//Convert to millis and uint32
	uint64_t limit = std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();

	limit -= 10*60*1000;

	return limit;
}

void printTimeFromMillis(uint64_t epochMillis)
{
	time_t t = epochMillis/1000;

	std::cout << "\t" << epochMillis << "\n\t" << std::put_time(std::localtime(&t), "%Y/%m/%d %T") << "\n";
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
