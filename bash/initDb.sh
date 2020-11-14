#!/bin/bash 

configPath=$(pwd)/../config
sqlScriptPath=$(pwd)/../sql
updateSensorTablePath=$(pwd)/../src/updateSensors


mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  source $sqlScriptPath/initDb.sql"

