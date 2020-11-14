#include "../lib/Helpers.h"

int main()
{
	return 0;
	const bool log = true;
	std::streambuf* logBuf;
	if(log) logBuf = startLog();

	try
	{
		sql::Driver* driver = get_driver_instance();
		std::unique_ptr<sql::Connection> dirtyCon(driver->connect(getDirtyConnectionString(), sourceDbUsername, sourceDbPassword));
		std::unique_ptr<sql::Connection> cleanCon(driver->connect(getCleanConnectionString(), destDbUsername, destDbPassword));
		dirtyCon->setSchema("vestsiden");
		cleanCon->setSchema("vestsiden");

		fillSensorTable(dirtyCon, cleanCon);

		std::cout << "Done, cleaning up..\n";
	}
	catch(sql::SQLException& e)
	{
		std::cout << "# ERR: SQLException in " << __FILE__;
	    std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
	    std::cout << "# ERR: " << e.what();
	    std::cout << " (MySQL error code: " << e.getErrorCode();
	    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		std::cout << "Exit on error";
		if(log) endLog(logBuf);
		return -1;
	}
	std::cout << "Clean exit\n";
	if(log) endLog(logBuf);
	return 0;
}
