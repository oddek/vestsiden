#!/bin/bash 

configPath=$(pwd)/../config
sqlScriptPath=$(pwd)/../sql

echo 'Reseting Database..'

mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  source $sqlScriptPath/resetDb.sql"

echo 'Done' 
