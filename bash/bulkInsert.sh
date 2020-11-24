#!/bin/bash

#paths
CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
configPath=${CURDIR}/../config
loadfilesPath=${CURDIR}/../loadfiles
srcPath=${CURDIR}/../src
# Generate loadfile
${srcPath}/readingFileGen/readingFileGen

#Necessary as there isn't enough space on local harddrive to sort file
export TMPDIR=/mnt/mysqldb/extHome/tmp

echo 'About to sort readings.csv'
# sort file based on two first columns(timestamp and sensorid)
SECONDS=0

sort -t, -k 1,1n -k 2,2n ${loadfilesPath}/readings19Nov.csv > ${loadfilesPath}/readingsSorted.csv
echo 'Sort took ' $SECONDS's'

echo 'Sorting complete, starting to split..'

# split into files of max a million, read1.csv, read2.csv...
SECONDS=0
split -dl 1000000 --additional-suffix=.csv ${loadfilesPath}/readingsSorted.csv ${loadfilesPath}/readingSplit/readingPart
echo 'Split took ' $SECONDS's'

echo 'Split complete\nStarting to load files..'

for f in ${loadfilesPath}/readingSplit/readingPart* 
do
  SECONDS=0
  echo 'loading file ' $f
  mysql --defaults-extra-file=${configPath}/newDb.conf -e "
    LOAD DATA LOCAL INFILE '$f'
    INTO TABLE HISTORYNUMERICTRENDRECORD
    FIELDS TERMINATED BY ','
    (HISTORY_ID, TIMESTAMP, VALUE, STATUS);"
  echo 'Insert took ' $SECONDS's'
done 

#Delete all splits
# rm readingPart*
