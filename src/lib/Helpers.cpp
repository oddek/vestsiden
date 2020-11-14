#include "Helpers.h"

std::vector<Entry> getLatestDirtyData(sql::Connection* dirtyCon, const uint64_t& timestampLowerLimit, const uint64_t& timestampUpperLimit)
{
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

int insertCleanData(sql::Connection* cleanCon, std::vector<Entry> data, std::map<std::string, int> sensors)
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

	stmt->execute(query);
	delete stmt;
	return data.size();
}

int insertCleanDataInFile(std::vector<Entry> data, std::string filename, std::map<std::string, int> sensors)
{
	//Open file in append mode
	std::ofstream file(filename, std::fstream::app);

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
	return data.size();
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

uint64_t getLastEntryTimestamp(sql::Connection* cleanCon)
{
	sql::Statement* stmt;
	sql::ResultSet* res;

	stmt = cleanCon->createStatement();

	//If the database is empty, we return 0
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
	return lastEntry * 1000;
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


void initDbConnections(sql::Connection* dirtyCon, sql::Connection* cleanCon)
{

	//Set connectionstrings
	std::string sourceConnectionString = "tcp://" + sourceDbHostNameV6 + ":" + sourceDbPort;
	std::string destConnectionString = "tcp://" + destDbHostNameV6 + ":" + destDbPort;

	std::cout << "Connecting to databases..\n";
	std::cout << sourceConnectionString << "\n";
	//Init database connections
	sql::Driver* driver = get_driver_instance();
	dirtyCon = driver->connect(sourceConnectionString, sourceDbUsername, sourceDbPassword);
	cleanCon = driver->connect(destConnectionString, destDbUsername, destDbPassword);
	dirtyCon->setSchema("vestsiden");
	cleanCon->setSchema("vestsiden");
	std::cout << "Databases connected\n";
}


void fillSensorTable(sql::Connection* dirtyCon, sql::Connection* cleanCon)
{
	std::vector<SensorEntry> entries;

	sql::Statement* stmt;
	sql::ResultSet* res;
	sql::PreparedStatement* prep_stmt;

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

	//If sensor counts are equal, there is no need to update
	if(dirtySensorCount == cleanSensorCount)
	{
		std::cout << "Number of sensors equal, no need to update\n";
		return;
	}

	std::cout << "Number of sensors are NOT equal, updating cleanDb..\n";

	//Extract all sensors from dirty db
	stmt = dirtyCon->createStatement();
	query = "SELECT `ID`, `ID_`, `VALUEFACETS` FROM `HISTORY_TYPE_MAP`";
	res = stmt->executeQuery(query);

	while(res->next())
	{
		SensorEntry s;
		s.id = res ->getInt("ID");
		s.name = res->getString("ID_");
		s.valuefacet = res->getString("VALUEFACETS");

		entries.push_back(s);
	}

	//Sort entries based on ID
	std::sort(entries.begin(), entries.end(), sensorEntrySort);

	//Insert sensor entries
	//Unique constraint in database will handle duplicates.
	//Will not throw an error
	for(auto& s : entries)
	{
		prep_stmt = cleanCon->prepareStatement
		(
			"INSERT IGNORE INTO "
				"`HISTORY_TYPE_MAP`"
					"(`ID`, `NAME`, `VALUEFACETS`) "
				"VALUES (?, ?, ?) "
		);

		prep_stmt->setInt(1, s.id);
		prep_stmt->setString(2, s.name);
		prep_stmt->setString(3, s.valuefacet);

		prep_stmt->execute();
	}

	delete stmt;
	delete res;
	delete prep_stmt;
}


//Has to take epoch in seconds!
void printTimeFromSeconds(uint64_t epoch)
{
	time_t t = epoch;

	std::cout << "\t" << epoch << "\n\t" << std::put_time(std::localtime(&t), "%Y/%m/%d %T") << "\n";
}

std::streambuf* startLog()
{
	//Redirect cout to file
	std::ofstream out("out.txt", std::fstream::app);
	std::streambuf* coutbuf = std::cout.rdbuf();
	std::cout.rdbuf(out.rdbuf());

	//Start time for log
    auto start = std::chrono::system_clock::now();
	std::time_t start_time = std::chrono::system_clock::to_time_t(start);
	std::cout << "\n\nLog insert. Time: " << std::ctime(&start_time) << "\n";

	return coutbuf;
}

void endLog(std::streambuf* coutbuf)
{
    auto start = std::chrono::system_clock::now();
	std::time_t start_time = std::chrono::system_clock::to_time_t(start);
	std::cout << "\n\nProgram finished at: " << std::ctime(&start_time) << "\n";
	std::cout.rdbuf(coutbuf);
}

bool sensorEntrySort(const SensorEntry& a, const SensorEntry& b)
{
	return a.id < b.id;
}
