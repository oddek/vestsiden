#include "Helpers.h"

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

void insertCleanDataInFile(std::vector<Entry> data, std::string filename, std::map<std::string, int> sensors)
{
	//Open file in append mode
	std::ofstream file(filename, std::fstream::app);

	std::cout << "Writing " << data.size() << " lines to file\n";

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
