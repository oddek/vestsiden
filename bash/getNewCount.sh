#!/bin/bash

configPath=$(pwd)/../config

SECONDS=0
newCount=`mysql --defaults-extra-file=$configPath/newDb.conf -e "
  SELECT COUNT(*) as NewCount from HISTORYNUMERICTRENDRECORD;"`
echo 'Took ' $SECONDS's'

echo 'NewCount: ' $newCount

# select HISTORYNUMERICTRENDRECORD.*, HISTORY_TYPE_MAP.NAME FROM HISTORYNUMERICTRENDRECORD inner join HISTORY_TYPE_MAP ON HISTORY_TYPE_MAP.ID = HISTORYNUMERICTRENDRECORD.HISTORY_ID LIMIT 0,200;
