#!/bin/bash 

#paths
CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
configPath=${CURDIR}/../config
loadfilesPath=${CURDIR}/../loadfiles
srcPath=${CURDIR}/../src
sqlPath=${CURDIR}/../sql

echo 'Reseting Database..'

mysql --defaults-extra-file=${configPath}/newDb.conf -s -N -e "
  source ${sqlPath}/resetDb.sql"

echo 'Done' 
