#!/bin/bash 

configPath=$(pwd)/../config
sqlScriptPath=$(pwd)/../sql
sensorFileGenPath=$(pwd)/../sensorFileGen
loadfilesPath=$(pwd)/../loadfiles


mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  source $sqlScriptPath/initDb.sql"

