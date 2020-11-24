#!/bin/bash

CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
configPath=${CURDIR}/../config

SECONDS=0
newCount=`mysql --defaults-extra-file=$configPath/newDb.conf -e "
  SELECT COUNT(*) as NewCount from HISTORYNUMERICTRENDRECORD;"`
echo 'Took ' $SECONDS's'

echo 'NewCount: ' $newCount

