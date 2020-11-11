#!/bin/bash
configPath=$(pwd)/../config
filePath=$(pwd)/../loadfiles

sort -t, -k 1,1n  $filePath/sensors.csv > $filePath/sensorsSorted.csv

mysql --defaults-extra-file=$configPath/newDb.conf -e "
  LOAD DATA LOCAL INFILE '$filePath/sensorsSorted.csv'
  INTO TABLE HISTORYNUMERICTRENDRECORD
  FIELDS TERMINATED BY ','
  (TIMESTAMP, HISTORY_ID, VALUE, STATUS);"

