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
#include <iostream>
#include <fstream>


int getCountUpToSeconds(sql::Connection* conn, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit);

int getTotalCount(sql::Connection* conn);

int maxValueStatus(sql::Connection* conn);

int minValueStatus(sql::Connection* conn);

//Returns in seconds, NOT millis
uint64_t getLastEntryTimestamp(sql::Connection* cleanCon);

//Has to take epoch in millis
void printTimeFromMillis(uint64_t epochMillis);

//Has to take epoch in seconds!
void printTimeFromSeconds(uint64_t epoch);

uint64_t millis_to_seconds(uint64_t i);

double round(double d);

int main()
{
	//Redirect cout to file
	std::ofstream out("out.txt", std::fstream::app);
	std::streambuf* coutbuf = std::cout.rdbuf();
	std::cout.rdbuf(out.rdbuf());
	//HUSK OG FJERNE DISSE FØR KJØRING!!!
	/* std::cout.rdbuf(coutbuf); */
	/* return 0; */

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


		uint64_t lowerTime = 1000;
		/* uint64_t upperTime = 1581845760000; */

		/* uint64_t upperTime = 1581845640000; */
		uint64_t upperTime = 1588330587000;
		int numberOfRowsInClean = getTotalCount(cleanConnection);
		int numberOfRowsInDirty = getCountUpToSeconds(dirtyConnection, lowerTime, upperTime);

		std::cout << "Total number of rows in clean DB:\n\t" << numberOfRowsInClean << "\n";
		std::cout << "Rows between " << lowerTime << ", " << upperTime << " in dirtyDb:\n\t" << numberOfRowsInDirty << "\n";


		int maxVal = maxValueStatus(dirtyConnection);
		std::cout << "Status field Maxvalue: " << maxVal << "\n";


		int minVal = minValueStatus(dirtyConnection);
		std::cout << "Status field Minvalue: " << minVal << "\n";


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

	std::cout.rdbuf(coutbuf);
	return 0;
}



int getCountUpToSeconds(sql::Connection* conn, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit)
{
	sql::ResultSet* res;
    sql::PreparedStatement* stmt;

	stmt = conn->prepareStatement
	(
		"SELECT COUNT(*) as CNT FROM "
			"HISTORYNUMERICTRENDRECORD "
		"WHERE "
			"`TIMESTAMP` >= (?) AND `TIMESTAMP` < (?) "
	);


	stmt->setUInt64(1, timestampLowerLimit);
	stmt->setUInt64(2, timestampUpperLimit);

	res = stmt->executeQuery();

	res->next();
	int count = res ->getInt("CNT");

	delete res;
	delete stmt;
	return count;
}


int getTotalCount(sql::Connection* conn)
{

	sql::ResultSet* res;
    sql::PreparedStatement* stmt;

	stmt = conn->prepareStatement
	(
		"SELECT COUNT(*) as CNT FROM "
			"HISTORYNUMERICTRENDRECORD "
	);

	res = stmt->executeQuery();

	res->next();
	int count = res ->getInt("CNT");

	delete res;
	delete stmt;
	return count;


}


int maxValueStatus(sql::Connection* conn)
{
	sql::ResultSet* res;
    sql::PreparedStatement* stmt;

	stmt = conn->prepareStatement
	(
		"SELECT MAX(TIMESTAMP) as MAX FROM "
			"HISTORYNUMERICTRENDRECORD "
	);

	res = stmt->executeQuery();

	res->next();
	int count = res ->getInt("MAX");

	delete res;
	delete stmt;
	return count;
}

int minValueStatus(sql::Connection* conn)
{

	sql::ResultSet* res;
    sql::PreparedStatement* stmt;

	stmt = conn->prepareStatement
	(
		"SELECT MIN(TIMESTAMP) as MIN FROM "
			"HISTORYNUMERICTRENDRECORD "
	);

	res = stmt->executeQuery();

	res->next();
	int count = res ->getInt("MIN");

	delete res;
	delete stmt;
	return count;
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













