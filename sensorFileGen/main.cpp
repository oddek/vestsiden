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




void createSensorFile(sql::Connection* dirtyCon, std::string filename);

int main()
{

	std::string filename = "../loadfile/sensorfile.csv";

	return 0;

	//Init connectionstrings
	std::string sourceConnectionString = "tcp://" + sourceDbHostNameV6 + ":" + sourceDbPort;
	try
	{
		std::cout << "Connecting to databases..\n";
		std::cout << sourceConnectionString << "\n";
		//Init database connections
		sql::Driver* driver = get_driver_instance();
		sql::Connection* dirtyConnection;
		dirtyConnection = driver->connect(sourceConnectionString, sourceDbUsername, sourceDbPassword);
		std::cout << "Databases connected\n";

		dirtyConnection->setSchema("vestsiden");

		std::cout << "Creating sensor file\n";
		createSensorFile(dirtyConnection, filename);

		std::cout << "Done, cleaning up..\n";
    	delete dirtyConnection;

	}
	catch(sql::SQLException& e)
	{
		std::cout << "# ERR: SQLException in " << __FILE__;
	    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
	    std::cout << "# ERR: " << e.what();
	    std::cout << " (MySQL error code: " << e.getErrorCode();
	    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		std::cout << "Exit on error";
		return -1;
	}
	std::cout << "Clean exit\n";
	return 0;
}




//TESTED, WORKS!
void createSensorFile(sql::Connection* dirtyCon, std::string filename)
{
	sql::Statement* stmt;
	sql::ResultSet* res;


	stmt = dirtyCon->createStatement();
	std::string query = "SELECT `ID`, `ID_`, `VALUEFACETS` FROM `HISTORY_TYPE_MAP`";
	res = stmt->executeQuery(query);

    std::ofstream file(filename);

	while(res->next())
	{

        file << res->getInt("ID") + ",";
		file << res->getString("ID_") + ",";
		file << res->getString("VALUEFACETS") + ",";

	}

	delete stmt;
	delete res;
}

