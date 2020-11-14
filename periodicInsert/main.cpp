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



/* Lag egen funksjon for å få db Conection. */
/* Fiks getlatestdirtydata query */
/* Sorter entry vector før du setter inn */
/* rydd opp i prints */
/* fiks timestamp litt ryddigere og enhetlig mellom millis og seconds */


//Data to be read from dirty DB
struct Entry
{
	uint64_t timestamp;
	double value;
	std::string sensorName;
	int status;
};


//Insert vector of CleanEntries into clean database
bool insertCleanData(sql::Connection* cleanCon, std::vector<Entry> data);

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



int main()
{
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
		//Refill sensor table, in case there are some new sensors added
		std::cout << "SensorTable filled\n";
		//Find the last timestamp registered in the clean database
		auto lastEntryTime = getLastEntryTimestamp(cleanConnection);
		std::cout << "Got last entry time:\n";
		printTimeFromSeconds(lastEntryTime);


		uint64_t selectLowerLimit = (lastEntryTime + 1) * 1000;
		std::cout << "Got lower limit for select:\n";
		printTimeFromMillis(selectLowerLimit);

		uint64_t selectUpperLimit = getEpochUpperLimit();
		std::cout << "Got upper limit for select:\n";
		printTimeFromMillis(selectUpperLimit);
		std::cout << "Will update db, with\n\tLower limit: " << selectLowerLimit << "\n\tUpper limit: " << selectUpperLimit << "\n";

		auto sensors = getSensors(cleanConnection);
		std::cout << "Got sensors..";

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
		std::cout << "Exit on error";
		/* std::cout.rdbuf(coutbuf); */
		return -1;
	}
	std::cout << "Clean exit\n";
	/* std::cout.rdbuf(coutbuf); */
	return 0;
}


bool insertCleanData(sql::Connection* cleanCon, std::vector<Entry> data, std::map<std::string, int> sensors)
{




	sql::Statement* stmt;
	stmt = cleanCon->createStatement();
	std::string query = "";
	query += "INSERT INTO `HISTORYNUMERICTRENDRECORD` ";
	query += "(`TIMESTAMP`, `VALUE`, `HISTORY_ID`, `STATUS`) ";
	query += "VALUES ";


	for(auto c : data)
	{

		int sensorId = sensors.at(c.sensorName);



		query += "(";
		query += std::to_string(millis_to_seconds(c.timestamp)) + ", ";
		query += doubleToScientific(c.value, 5) + ", ";
		query += std::to_string(sensorId) + ", ";
		query += std::to_string(c.status);
		query += "), ";
	}

	query.erase(query.length()-2);
	/* std::cout << query << "\n"; */

	stmt->execute(query);
	delete stmt;
	return true;
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
