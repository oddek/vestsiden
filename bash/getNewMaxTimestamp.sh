#!/bin/bash

configPath=$(pwd)/../config

SECONDS=0
maxTimestamp=`mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  SELECT MAX(TIMESTAMP) FROM HISTORYNUMERICTRENDRECORD;"`
echo 'Took ' $SECONDS's'
  
echo 'Highest timestamp found in new db: ' $maxTimestamp 
