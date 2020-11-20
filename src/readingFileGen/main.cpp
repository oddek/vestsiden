#include "../lib/Helpers.h"

int main()
{
    std::ofstream logFile("status.log", std::fstream::app);
    Tee tee(std::cout, logFile);
    TeeStream log(tee);

	std::cout << "test";
	return 0;

	const std::string filename = "../../loadfiles/readings19Nov.csv";
	//Set the max time interval for each select statement
	const int hourlyFetchIncrement = 12;

	try
	{
		sql::Driver* driver = get_driver_instance();
		std::unique_ptr<sql::Connection> dirtyCon(driver->connect(getDirtyConnectionString(), sourceDbUsername, sourceDbPassword));
		std::unique_ptr<sql::Connection> cleanCon(driver->connect(getCleanConnectionString(), destDbUsername, destDbPassword));
		dirtyCon->setSchema("vestsiden");
		cleanCon->setSchema("vestsiden");

		//Earliest sensor reading in dirty db
		const uint64_t totalInsertLowerLimit = 1581845340000;
		log << "Got lower limit for file:\n";
		printTimeFromMillis(totalInsertLowerLimit, log);
		//Ten minutes ago at runtime
		/* const uint64_t totalInsertUpperLimit = getEpochUpperLimit(); */
		const uint64_t totalInsertUpperLimit = 1605657600000;//getEpochUpperLimit();
		log << "Got upper limit for file:\n";
		printTimeFromMillis(totalInsertUpperLimit, log);
		/* const uint64_t totalInsertUpperLimit = getEpochUpperLimit(); */

		//Dont want to select more than about 500 000 rows at the time, so we need fill the file incrementally
		uint64_t currentLower = totalInsertLowerLimit;
		//Convert the max time interval to millis
		const uint64_t incrementValue = hourlyFetchIncrement * 60 * 60 * 1000;
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
			//Start timer
			auto t1 = std::chrono::high_resolution_clock::now();
			log << "Fetching data..\n\tLower bound: " << currentLower << "\n\tUpper bound: " << currentUpper << "\n";
			auto data = getLatestDirtyData(dirtyCon, currentLower, currentUpper);

			//Insert clean data into database.
			log << "Inserting data..\n";
			int linesWritten = insertCleanDataInFile(data, filename, sensors);
			totalLinesWritten += linesWritten;
			log << "Lines written: " << linesWritten << "\n";

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
			log << "Iteration took: " << duration << "seconds\n";
			log << "Total lines written: " << totalLinesWritten << "\n";
		}
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
		return -1;
	}
	log << "Clean exit\n";
	return 0;
}
