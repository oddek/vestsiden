#!/bin/bash

#paths
CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
srcPath=${CURDIR}/../src

echo 'Starting periodicFiller'
SECONDS=0
${srcPath}/periodicInsert/fill
echo 'Took ' $SECONDS's'
echo 'Done'
