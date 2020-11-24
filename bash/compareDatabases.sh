#!/bin/bash

configPath=$(pwd)/../config

#Printing size of databases in MBs
./getDbSizes.sh



SECONDS=0
#Quicker way to get max
#Collects the highest timestamp from each sensor, and then selects the max value from those. 
maxTimestamp=`mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  SELECT 
    MAX(TIMESTAMP) 
  FROM 
    (SELECT
      HISTORY_ID, MAX(TIMESTAMP) AS max_ts
    FROM
      HISTORYNUMERICTRENDRECORD
    GROUP BY
      SENSOR) AS max_set;"`

echo 'Took ' $SECONDS's'
echo 'Highest timestamp found in new db: ' $maxTimestamp 

#Getting count in old database up to maxTimestamp
./getOldCount.sh maxTimestamp &
pid1=$!

#Getting total count in new database
./getNewCount.sh &
pid2=$!

wait $pid1 && echo "getOldCount exited normally" || echo "getOldCount exited abnormally with status $?"
wait $pid2 && echo "getNewCount exited normally" || echo "getNewCount exited abnormally with status $?

