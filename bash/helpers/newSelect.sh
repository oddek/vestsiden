
#!/bin/bash

CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
configPath=${CURDIR}/../../config

SECONDS=0
newCount=`mysql --defaults-extra-file=${configPath}/newDb.conf -e "
  SELECT * from HISTORYNUMERICTRENDRECORD where HISTORY_ID = 2 AND TIMESTAMP > 1602772691 and TIMESTAMP < 1605451091;"`

echo 'NewCount: ' $newCount

echo 'Took ' $SECONDS's'
# select HISTORYNUMERICTRENDRECORD.*, HISTORY_TYPE_MAP.NAME FROM HISTORYNUMERICTRENDRECORD inner join HISTORY_TYPE_MAP ON HISTORY_TYPE_MAP.ID = HISTORYNUMERICTRENDRECORD.HISTORY_ID LIMIT 0,200;
