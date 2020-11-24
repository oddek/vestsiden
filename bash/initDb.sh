#!/bin/bash 

CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
configPath=${CURDIR}/../config
loadfilesPath=${CURDIR}/../loadfiles
srcPath=${CURDIR}/../src
sqlPath=${CURDIR}/../sql

echo 'Creating Database..'
mysql --defaults-extra-file=${configPath}/newDb.conf -s -N -e "
  source ${sqlScriptPath}/initDb.sql"

echo 'Inserting sensors..'
${srcPath}/updateSensors/updateSensors

