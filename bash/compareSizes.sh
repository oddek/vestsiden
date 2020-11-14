#!/bin/bash

configPath=$(pwd)/../config


SECONDS=0
maxTimestamp=`mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  SELECT MAX(TIMESTAMP) FROM HISTORYNUMERICTRENDRECORD;"`
  
echo 'Highest timestamp found in new db: ' $maxTimestamp '\n\tTook ' $SECONDS's'
SECONDS=0
mysql --defaults-extra-file=$configPath/newDb.conf -e "
  SELECT COUNT(*) as OldCount from HISTORYNUMERICTRENDRECORD WHERE TIMESTAMP < $maxTimestamp;"
echo 'Took ' $SECONDS's'

SECONDS=0
mysql --defaults-extra-file=$configPath/oldDb.conf -e "
  SELECT COUNT(*) as NewCount from HISTORYNUMERICTRENDRECORD;" 
echo 'Took ' $SECONDS's'



# select HISTORYNUMERICTRENDRECORD.*, HISTORY_TYPE_MAP.NAME FROM HISTORYNUMERICTRENDRECORD inner join HISTORY_TYPE_MAP ON HISTORY_TYPE_MAP.ID = HISTORYNUMERICTRENDRECORD.HISTORY_ID LIMIT 0,200;
