#!/bin/bash

CONFIG=../config/newDb.conf
dbname=$(awk '/^dbname/{print $3}' "${CONFIG}")
username=$(awk '/^username/{print $3}' "${CONFIG}")
password=$(awk '/^password/{print $3}' "${CONFIG}")
filePath=$(pwd)/../loadfiles

echo 'About to sort readings.csv'
#sort file based on two first columns(timestamp and sensorid)
sort -t, -k 1,1n -k 2,2n $filepath/readings.csv > $filepath/readingsSorted.csv
echo 'Sorting complete, starting to split..'
#split into files of max a million, read1.csv, read2.csv...
split -dl 1000000 --additional-suffix=.csv readingsSorted.csv readingPart

echo 'Split complete\nStarting to load files..'
for f in readingPart*
do
  echo 'loading file ' $f
  mysql -u$username -p$password -e "use $dbname" -e "
    LOAD DATA LOCAL INFILE '$f'
    INTO TABLE HISTORYNUMERICTRENDRECORD
    FIELDS TERMINATED BY ','
    (TIMESTAMP, HISTORY_ID, VALUE, STATUS);"
done 

#Delete all splits
rm readingPart*
