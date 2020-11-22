#!/bin/bash 

configPath=$.(pwd)/../config
sqlScriptPath=$(pwd)/../sql
updateSensorTablePath=$(pwd)/../src/updateSensors

echo 'Creating Database..'
mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  source $sqlScriptPath/initDb.sql"

echo 'Inserting sensors..'
../src/updateSensors/updateSensors

