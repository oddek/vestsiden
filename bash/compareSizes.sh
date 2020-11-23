#!/bin/bash

configPath=$(pwd)/../config

SECONDS=0
oldSize=`mysql --defaults-extra-file=$configPath/oldDb.conf -s -N -e "
  SELECT table_schema vestsiden,
          ROUND(SUM(data_length + index_length) / 1024 / 1024, 1) 'DB Size in MB' 
  FROM information_schema.tables 
  GROUP BY table_schema;"` 

echo 'Took ' $SECONDS's'
echo 'Size of old db: ' $oldSize 

SECONDS=0
newSize=`mysql --defaults-extra-file=$configPath/newDb.conf -s -N -e "
  SELECT table_schema vestsiden,
          ROUND(SUM(data_length + index_length) / 1024 / 1024, 1) 'DB Size in MB' 
  FROM information_schema.tables 
  GROUP BY table_schema;"` 

echo 'Took ' $SECONDS's'
echo 'Size of new db: ' $newSize 
