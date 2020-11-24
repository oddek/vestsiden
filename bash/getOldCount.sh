
#!/bin/bash

CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
configPath=${CURDIR}/../config

maxTimeStamp=$1

maxMillis=$((($maxTimeStamp + 1) * 1000))

echo 'OldCount using ' $maxMillis

SECONDS=0
oldCount=`mysql --defaults-extra-file=$configPath/oldDb.conf -e "
  SELECT COUNT(*) as OldCount from HISTORYNUMERICTRENDRECORD WHERE TIMESTAMP < '$maxMillis';"`

echo 'Took ' $SECONDS's'
echo 'OldCount: ' $oldCount

