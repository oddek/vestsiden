
#!/bin/bash

configPath=$(pwd)/../config

maxTimeStamp=1605657600

maxMillis=$(($maxTimeStamp * 1000))

echo $maxMillis

SECONDS=0
oldCount=`mysql --defaults-extra-file=$configPath/oldDb.conf -e "
  SELECT COUNT(*) as OldCount from HISTORYNUMERICTRENDRECORD WHERE TIMESTAMP < '$maxMillis';"`

echo 'Took ' $SECONDS's'
echo 'OldCount: ' $oldCount

