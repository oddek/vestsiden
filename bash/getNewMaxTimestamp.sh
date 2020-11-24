#!/bin/bash

configPath=$(pwd)/../config


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


# Bad way to get maximum timestamp value
SECONDS=0
maxTimestamp=`mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  SELECT MAX(TIMESTAMP) FROM HISTORYNUMERICTRENDRECORD;"`
echo 'Took ' $SECONDS's'
  
echo 'Highest timestamp found in new db: ' $maxTimestamp 
