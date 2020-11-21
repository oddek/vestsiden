# src/

This directory contains C++ source files and executables.

These are not meant to be run by themselves, but only accessed through the [bash scripts](../bash/README.md). 

All applications are built by calling 
```sh
make
```
within each respective folder


## lib

Contains:
* [Helpers.h](lib/Helpers.h)
* [Helpers.cpp](lib/Helpers.cpp)

All major logic lies here, and is meant to be used by the other software in this directory.


## updateSensors

Contains:
* [main.cpp](updateSensors/main.cpp)
* [Makefile](updateSensors/Makefile)

Called by:
* [initDb.sh](../bash/initDb.sh)
* [periodicInsert.sh](../bash/periodicInsert.sh)

Extracts all sensors from the dirty database, and inserts them into the new clean database.

Retains the same primary key for each sensor in the clean database as in the dirty database. 

Will run periodically to make sure that we dont try and insert sensordata for which no sensor exists.

Reasonably quick, but the insert ignore clause makes it slower than expected. 

## readingFileGen

Contains:
* [main.cpp](readingFileGen/main.cpp)
* [Makefile](readingFileGen/Makefile)

Called by:
* [bulkInsert.sh](../bash/bulkInsert.sh)

Extracts rows spanning from the earliest insert and up to 24 hours ago, from HISTORYNUMERICTRENDRECORD in dirty db. These rows will then be saved to a .csv file in [loadfiles/](../loadfiles/README.md).

Takes about 60 seconds per 1 million rows.


## periodicInsert

Contains:
* [main.cpp](periodicInsert/main.cpp)
* [Makefile](periodicInsert/Makefile)

Called by:
* [periodicInsert.sh](../bash/periodicInsert.sh)

Finds the highest timestamp currently existing in the clean database. Unfortunately this takes about ten minutes with 250 million rows. 

Selects incrementally all rows from the dirty database that have timestamps exceeding the highest timestamp, up to 24 hours ago. 

Inserts these rows into the clean database. 
