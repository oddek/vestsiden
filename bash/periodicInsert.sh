#!/bin/bash

SECONDS=0
echo 'Starting periodicFiller'
/mnt/mysqldb/extHome/vestsiden/src/periodicInsert/fill
echo 'Took ' $SECONDS's'
echo 'Done'
