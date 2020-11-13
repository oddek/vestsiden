#!/bin/bash
configPath=$(pwd)/../config
filePath=$(pwd)/../loadfiles

sort -t, -k 1,1n  ../loadfiles/sensors.csv > ../loadfiles/sensorsSorted.csv

mysql --defaults-extra-file=../config/newDb.conf -e "
  LOAD DATA LOCAL INFILE '$filePath/sensorsSorted.csv'
  INTO TABLE HISTORY_TYPE_MAP
  FIELDS TERMINATED BY ','
  (ID, NAME, VALUEFACETS);"

