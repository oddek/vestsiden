#include "../lib/Helpers.h"

//SORTER ENTRYVECTOR FØR VI SETTER INN

int main()
{
	return 0;
	const bool log = true;
	std::streambuf* logBuf;
	if(log) logBuf = startLog();

	const std::string filename = "../../loadfiles/readings2.csv";
	//Set the max time interval for each select statement
	const int minuteFetchIncrement = 30;
	try
	{
		sql::Driver* driver = get_driver_instance();
		std::unique_ptr<sql::Connection> dirtyCon(driver->connect(getDirtyConnectionString(), sourceDbUsername, sourceDbPassword));
		std::unique_ptr<sql::Connection> cleanCon(driver->connect(getCleanConnectionString(), destDbUsername, destDbPassword));
		dirtyCon->setSchema("vestsiden");
		cleanCon->setSchema("vestsiden");

		//Earliest sensor reading in dirty db
		const uint64_t totalInsertLowerLimit = getLastEntryTimestamp(cleanCon);
		if(totalInsertLowerLimit == 0)
		{
			std::cout << "Lower timestamp from clean db is equal to 0. Use bulk insert instead";
			if(log) endLog(logBuf);
			return 0;
		}

		std::cout << "Got lower limit for file:\n";
		printTimeFromMillis(totalInsertLowerLimit);
		//Ten minutes ago at runtime
		const uint64_t totalInsertUpperLimit = getEpochUpperLimit();
		std::cout << "Got upper limit for file:\n";
		printTimeFromMillis(totalInsertUpperLimit);

		//Dont want to select too many rows at the time, so we need fill the file incrementally
		uint64_t currentLower = totalInsertLowerLimit;
		//Convert the max time interval to millis
		const uint64_t incrementValue = minuteFetchIncrement * 60 * 1000;
		uint64_t currentUpper = currentLower + incrementValue;
		if(currentUpper > totalInsertUpperLimit)
		{
			currentUpper = totalInsertUpperLimit;
		}

		//Get sensors for translating sensorname to ID
		const auto sensors = getSensorMap(cleanCon);

		int totalLinesWritten = 0;

		//Loop until the file has been filled with all rows from dirty db
		while(currentLower < totalInsertUpperLimit)
		{

			auto t1 = std::chrono::high_resolution_clock::now();
			std::cout << "Fetching data..\n\tLower bound: " << currentLower << "\n\tUpper bound: " << currentUpper << "\n";
			auto data = getLatestDirtyData(dirtyCon, currentLower, currentUpper);

			//Insert clean data into database.
			std::cout << "Inserting data..\n";
			int linesWritten = insertCleanData(cleanCon, data, sensors);
			totalLinesWritten += linesWritten;
			std::cout << "Lines written: " << linesWritten << "\n";

			//Increment timestamps for select
			currentLower = currentUpper;
			currentUpper += incrementValue;

			//We dont want to read more than what was there ten minutes ago
			if(currentUpper > totalInsertUpperLimit)
			{
				currentUpper = totalInsertUpperLimit;
			}

			//Print time loop took
			auto t2 = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::seconds>( t2 - t1 ).count();
			std::cout << "Iteration took: " << duration << "seconds\n";
			std::cout << "Total lines written: " << totalLinesWritten << "\n";
		}
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
