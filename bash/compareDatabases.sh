#!/bin/bash

#paths
CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
configPath=${CURDIR}/../config

#Printing size of databases in MBs
${CURDIR}/helpers/getDbSizes.sh

#Quicker way to get max timestamp from new database
#Collects the highest timestamp from each sensor, and then selects the max value from those. 
SECONDS=0
maxTimestamp=`mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  SELECT 
    MAX(max_ts) 
  FROM 
    (SELECT
      HISTORY_ID, MAX(TIMESTAMP) AS max_ts
    FROM
      HISTORYNUMERICTRENDRECORD
    GROUP BY
      HISTORY_ID) AS max_set;"`
echo 'Took ' $SECONDS's'

echo 'Highest timestamp found in new db: ' $maxTimestamp 

# These scripts are executing concurrently:

#Getting count in old database up to maxTimestamp
${CURDIR}/helpers/getOldCount.sh $maxTimestamp &
pid1=$!

#Getting total count in new database
${CURDIR}/helpers/getNewCount.sh &
pid2=$!

wait $pid1 && echo "getOldCount exited normally" || echo "getOldCount exited abnormally with status $?"
wait $pid2 && echo "getNewCount exited normally" || echo "getNewCount exited abnormally with status $?"

