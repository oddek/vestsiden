#!/bin/bash
export TMPDIR=/mnt/mysqldb/extHome/tmp
echo 'About to sort readings.csv'
##sort file based on two first columns(timestamp and sensorid)
SECONDS=0
sort -t, -k 1,1n -k 2,2n ../loadfiles/readings2.csv > ../loadfiles/readingsSorted2.csv
echo 'Sort took ' $SECONDS's'
echo 'Sorting complete, starting to split..'
#split into files of max a million, read1.csv, read2.csv...
SECONDS=0
split -dl 1000000 --additional-suffix=.csv ../loadfiles/readingsSorted2.csv ../loadfiles/readingSplit/reading2Part
echo 'Split took ' $SECONDS's'

echo 'Split complete\nStarting to load files..'
for f in ../loadfiles/readingSplit/reading2Part*
do
  SECONDS=0
  echo 'loading file ' $f
  mysql --defaults-extra-file=../config/newDb.conf -e "
    LOAD DATA LOCAL INFILE '$f'
    INTO TABLE HISTORYNUMERICTRENDRECORD
    FIELDS TERMINATED BY ','
    (HISTORY_ID, TIMESTAMP, VALUE, STATUS);"
  echo 'Insert took ' $SECONDS's'
done 

#Delete all splits
# rm readingPart*
