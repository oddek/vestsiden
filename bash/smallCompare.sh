#!/bin/bash


configPath=$(pwd)/../config

maxTimeStamp=1605285900000
SECONDS=0
# oldCount=`mysql --defaults-extra-file=$configPath/oldDb.conf -e "
#   SELECT COUNT(*) as OldCount from HISTORYNUMERICTRENDRECORD WHERE TIMESTAMP <= '$maxTimestamp';"`

oldCount=`mysql --defaults-extra-file=$configPath/oldDb.conf -e "
  SELECT COUNT(*) as OldCount from HISTORYNUMERICTRENDRECORD WHERE TIMESTAMP <= '$maxTimeStamp';"`

echo 'Took ' $SECONDS's'

echo 'OldCount: ' $oldCount
