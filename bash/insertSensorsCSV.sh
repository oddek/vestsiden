#!/bin/bash

CONFIG=../config/newDb.conf
dbname=$(awk '/^dbname/{print $3}' "${CONFIG}")
username=$(awk '/^username/{print $3}' "${CONFIG}")
password=$(awk '/^password/{print $3}' "${CONFIG}")
filePath=$(pwd)/../loadfiles

sort -t, -k 1,1n  $filePath/sensors.csv > $filePath/sensorsSorted.csv

mysql -u$username -p$password -e "use $dbname" -e "
  LOAD DATA LOCAL INFILE '$filePath/sensorsSorted.csv'
  INTO TABLE HISTORYNUMERICTRENDRECORD
  FIELDS TERMINATED BY ','
  (TIMESTAMP, HISTORY_ID, VALUE, STATUS);"

