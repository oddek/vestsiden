#!/bin/bash

CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
srcPath=${CURDIR}/../src

SECONDS=0
echo 'Starting periodicFiller'
${srcPath}/periodicInsert/fill
echo 'Took ' $SECONDS's'
echo 'Done'
