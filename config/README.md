# config/

## Setup configuration files containing credentials

Create a renamed copy of the files
* [dummy_dbconfig.cpp](dummy_dbconfig.cpp)
* [dummy_oldDb.conf](dummy_oldDb.conf)
* [dummy_newDb.conf](dummy_newDb.conf)

by running the following commands

```sh
cp dummy_dbconfig.cpp dbconfig.cpp
cp dummy_oldDb.conf oldDb.conf
cp dummy_newDb.conf newDb.conf
```

Then fill the copies with the necessary credentials for accessing the databases.

## Change MySQL settings

The tested MySQL configuration can be found in [my.cnf](my.cnf). To change your settings, open your my.cnf file with your favorite editor:
```sh
vim /etc/mysql/my.cnf
```
and copy in the settings. 

These settings were optimized for a server with 8GB of RAM. 
