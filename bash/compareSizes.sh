#!/bin/bash

configPath=$(pwd)/../config

SECONDS=0
maxTimestamp=`mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  SELECT MAX(TIMESTAMP) FROM HISTORYNUMERICTRENDRECORD;"`
echo 'Took ' $SECONDS's'
  
echo 'Highest timestamp found in new db: ' $maxTimestamp 
maxMillis=$((($maxTimeStamp + 1) * 1000))

echo 'Highest timestamp found in millis: ' $maxMillis

SECONDS=0
oldCount=`mysql --defaults-extra-file=$configPath/oldDb.conf -e "
  SELECT COUNT(*) from HISTORYNUMERICTRENDRECORD WHERE TIMESTAMP < '$maxMillis';"`
echo 'Took ' $SECONDS's'

echo 'OldCount: ' $oldCount

SECONDS=0
newCount=`mysql --defaults-extra-file=$configPath/newDb.conf -e "
  SELECT COUNT(*) from HISTORYNUMERICTRENDRECORD;"`
echo 'Took ' $SECONDS's'

echo 'NewCount: ' $newCount
