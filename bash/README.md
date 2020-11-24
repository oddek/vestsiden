# bash/


## initDb.sh

Creates database by running the [initDb.sql](../sql/initDb.sql) script.

It also runs the C++ application [updateSensors](../src/updateSensors/main.cpp), which fills the new database with all the sensors from the old one. Details can be found in the [readme](../src/README.md).

## resetDb.sh

Deletes all rows in HISTORYNUMERICTRENDRECORD(readings) by running [resetDb.sql](../sql/resetDb.sql). The sensors remain untouched.

## bulkInsert.sh

Call C++ application [readingFileGen](../src/readingFileGen/main.cpp), in order to generate CSV file containing all entries in old database.

Calls GNU Sort on the file, and sorts by sensor id and timestamp.

Splits the file into several files all containing 1 million rows. 

Inserts all rows into new database. 

As these operations are very time-consuming, they are not automatically deleted.

All files are stored in the loadfiles/ directory]

## periodicInsert.sh

Calls the application [periodicInsert](../src/periodicInsert/main.cpp). Details in [readme](../src/README.md).

Meant to be run periodically as a cronjob. 

Recommended interval is 12 hours.

Uses about 30 seconds per 90 000 rows which is about one hour worth of data. 

## compareDatabases.sh

Find the size of both databases in MBs, prints them.

Finds highest timestamp of new database, and prints the number of rows in both of the databases up to that point. 

Takes about 2 hours. 

## helpers/

A small collection of helper scripts, used by scripts in this folder
