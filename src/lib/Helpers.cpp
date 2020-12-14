#include "Helpers.h"
#include <cppconn/prepared_statement.h>

std::vector<Entry> getLatestDirtyData(std::unique_ptr<sql::Connection>& dirtyCon, uint64_t timestampLowerLimit, uint64_t timestampUpperLimit)
{
    std::vector<Entry> newData;

	std::string query =
		"SELECT "
			"`TIMESTAMP`, `VALUE`, `HISTORY_ID`, `STATUS` "
		"FROM "
			"HISTORYNUMERICTRENDRECORD "
		"WHERE "
			"`TIMESTAMP` >= (?) AND `TIMESTAMP` < (?) ";

	//Create prepared statement object
	std::unique_ptr<sql::PreparedStatement> prep_stmt(dirtyCon->prepareStatement(query));

	//Bind parameters
	prep_stmt->setUInt64(1, timestampLowerLimit);
	prep_stmt->setUInt64(2, timestampUpperLimit);
	//Perform query
	std::unique_ptr<sql::ResultSet> res(prep_stmt->executeQuery());

	//Fill vector with data
	while(res->next())
	{
		Entry d;
		d.timestamp = res->getUInt64("TIMESTAMP");
		d.value = res->getDouble("VALUE");
		d.sensorName = res->getString("HISTORY_ID");
		d.status = res->getInt("STATUS");
		newData.push_back(d);
	}

    return newData;
}

int insertCleanData(std::unique_ptr<sql::Connection>& cleanCon, std::vector<Entry> data, std::map<std::string, int> sensors)
{
	//If data is empty, we return immediately.
	//Else the empty insert will give an error
	if(data.empty())
	{
		return 0;
	}
	//Start of query
	std::string query =
		"INSERT INTO `HISTORYNUMERICTRENDRECORD` "
			"(`TIMESTAMP`, `VALUE`, `HISTORY_ID`, `STATUS`) "
		"VALUES ";
	//Append all the values in data vector to the same query
	//This is done to save time
	for(auto c : data)
	{
		//Translate sensorName to sensorId
		auto sensorSearch = sensors.find(c.sensorName);
		int sensorId;
		if(sensorSearch != sensors.end())
		{
			sensorId = sensorSearch->second;
		}
		else
		{
			continue;
		}
		sensorId = sensors.at(c.sensorName);

		query += "(";
		query += std::to_string(millis_to_seconds(c.timestamp)) + ", ";
		query += doubleToScientific(c.value, 5) + ", ";
		query += std::to_string(sensorId) + ", ";
		query += std::to_string(c.status) + "), ";
	}
	//Erase the last redundant comma and space
	query.erase(query.length()-2);
		//Create statement
	std::unique_ptr<sql::Statement> stmt(cleanCon->createStatement());
	//Perform query
	stmt->execute(query);
	//Returns number of rows inserted
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

std::map<std::string, int> getSensorMap(std::unique_ptr<sql::Connection>& cleanCon)
{
	std::string query =
		"SELECT "
			"`ID`, `NAME` "
		"FROM "
			"`HISTORY_TYPE_MAP`";

	//Create stmt and perform query
	std::unique_ptr<sql::Statement> stmt(cleanCon->createStatement());
	std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

	//Fill a std::map with all sensors
	std::map<std::string, int> sensors;
	while(res->next())
	{
		int id = res->getInt("ID");
		std::string name = res->getString("NAME");
		sensors.insert(std::pair<std::string, int>(name, id));
	}
	return sensors;
}


uint64_t getInsertUpperLimit()
{
	//Get time
	auto t = std::chrono::system_clock::now();
	//Convert to millis and uint32
	uint64_t limit = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();

	//Substract 24 hours
	limit -= 24*60*60;

	return seconds_to_millis(limit);
}

uint64_t getInsertLowerLimit(std::unique_ptr<sql::Connection>& cleanCon)
{
	//Get last entry of cleandb in millis
	uint64_t lastEntry =  getLastEntryTimestamp(cleanCon);
	//Add one second to it, so that we wont try and fetch an already existing row
	return lastEntry + 1000;
}

uint64_t getLastEntryTimestamp(std::unique_ptr<sql::Connection>& cleanCon)
{
	//Get max timestamp from clean db, if empty return 0
	std::string query =
	  "SELECT "
		"MAX(max_ts) AS LastEntry "
	  "FROM "
		"(SELECT "
		  "HISTORY_ID, MAX(TIMESTAMP) AS max_ts "
		"FROM "
		  "HISTORYNUMERICTRENDRECORD "
		"GROUP BY "
		  "HISTORY_ID) AS max_res";

	//Create stmt and perform query
	std::unique_ptr<sql::Statement> stmt(cleanCon->createStatement());
	std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

	uint64_t lastEntry = 0;
	while(res->next())
	{
		lastEntry  = res->getUInt64("LastEntry");
	}
	//CleanDB holds seconds, we want millis
	return seconds_to_millis(lastEntry);
}

void printTimeFromMillis(uint64_t epochMillis, TeeStream& log)
{
	time_t t = epochMillis/1000;

	log << "\t" << epochMillis << "\n\t" << std::put_time(std::localtime(&t), "%Y/%m/%d %T") << "\n";
}

uint64_t seconds_to_millis(uint64_t i)
{
	return i * 1000;
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

std::string getDirtyConnectionString()
{
	return "tcp://" + sourceDbHostNameV6 + ":" + sourceDbPort;
}

std::string getCleanConnectionString()
{
	return "tcp://" + destDbHostNameV6 + ":" + destDbPort;
}

int getSensorCountFromDb(std::unique_ptr<sql::Connection>& con)
{
	std::string query =
		"SELECT "
			"COUNT(*) AS CNT "
		"FROM "
			"`HISTORY_TYPE_MAP`";

	//Create stmt and perform query
	std::unique_ptr<sql::Statement> stmt(con->createStatement());
	std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

	res->next();
	return res->getInt("CNT");
}

int fillSensorTable(std::unique_ptr<sql::Connection>& dirtyCon, std::unique_ptr<sql::Connection>& cleanCon)
{
	//Get count of sensors from dirtyDb
	int dirtySensorCount = getSensorCountFromDb(dirtyCon);
	//Get count of sensors from cleanDb
	int cleanSensorCount = getSensorCountFromDb(cleanCon);
	//If sensor counts are equal, there is no need to update
	if(dirtySensorCount == cleanSensorCount)
	{
		return 0;
	}

	//Extract all sensors from dirty db
	std::string query =
		"SELECT "
			"`ID`, `ID_`, `VALUEFACETS` "
		"FROM "
			"`HISTORY_TYPE_MAP`";

	std::unique_ptr<sql::Statement> stmt(dirtyCon->createStatement());
	std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));

	std::vector<SensorEntry> entries;
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
	//Using multiple prepared statements and not multiple insert because of difficulty with escape characters in valuefacets.
	for(auto& s : entries)
	{
		query =
			"INSERT IGNORE INTO "
				"`HISTORY_TYPE_MAP`"
					"(`ID`, `NAME`, `VALUEFACETS`) "
			"VALUES "
				"(?, ?, ?) ";

		std::unique_ptr<sql::PreparedStatement> prep_stmt(cleanCon->prepareStatement(query));
		prep_stmt->setInt(1, s.id);
		prep_stmt->setString(2, s.name);
		prep_stmt->setString(3, s.valuefacet);

		prep_stmt->execute();
	}

	return entries.size();
}


//Has to take epoch in seconds!
void printTimeFromSeconds(uint64_t epoch, TeeStream& log)
{
	time_t t = epoch;

	log << "\t" << epoch << "\n\t" << std::put_time(std::localtime(&t), "%Y/%m/%d %T") << "\n";
}

bool sensorEntrySort(const SensorEntry& a, const SensorEntry& b)
{
	return a.id < b.id;
}


std::string getLogPath()
{
	//Get path of executable
	char result[ PATH_MAX ];
	ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
	std::string s( result, (count > 0) ? count : 0 );

	//Find position of last /
	size_t pos = s .find_last_of('/', s.size());
	//Erase name of executable and last slash, so we are left with only the path to the current dir
	s.erase(pos, s.size() - pos);
	return s;
}

void logCurrentTime(TeeStream& log)
{
	auto start = std::chrono::system_clock::now();
	std::time_t start_time = std::chrono::system_clock::to_time_t(start);
	log << "Time: " << std::ctime(&start_time) << "\n" << std::flush;
}
