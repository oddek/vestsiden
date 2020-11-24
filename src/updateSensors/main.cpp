#include "../lib/Helpers.h"

int main()
{
	std::cout << getLogPath() << "\n";
    std::ofstream logFile(getLogPath() + "/status.log", std::fstream::app);
    Tee tee(std::cout, logFile);
    TeeStream log(tee);

	log << "NEW LOG INSERT!\n" << std::flush;
	logCurrentTime(log);

	return 0;
	try
	{
		sql::Driver* driver = get_driver_instance();
		std::unique_ptr<sql::Connection> dirtyCon(driver->connect(getDirtyConnectionString(), sourceDbUsername, sourceDbPassword));
		std::unique_ptr<sql::Connection> cleanCon(driver->connect(getCleanConnectionString(), destDbUsername, destDbPassword));
		dirtyCon->setSchema("vestsiden");
		cleanCon->setSchema("vestsiden");

		int i = fillSensorTable(dirtyCon, cleanCon);
		if(i == 0) log << "No update necessary";
		else log << "Updated sensor table, currently " << i << " sensors in db\n";

		log << "Done, cleaning up..\n";
	}
	catch(sql::SQLException& e)
	{
		log << "# ERR: SQLException in " << __FILE__;
	    log << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
	    log << "# ERR: " << e.what();
	    log << " (MySQL error code: " << e.getErrorCode();
	    log << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		log << "Exit on error";
		logCurrentTime(log);
		return -1;
	}
	logCurrentTime(log);
	log << "Clean exit\n";
	return 0;
}
