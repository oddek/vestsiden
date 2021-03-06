


# Vestsiden DB


<!-- TABLE OF CONTENTS -->
## Table of Contents

* [About the Project](#about-the-project)
  * [Libraries](#libraries)
* [Prerequisites](#prerequisites)
* [Usage](#usage)
* [License](#license)
* [Contact](#contact)



<!-- ABOUT THE PROJECT -->
## About The Project

The purpose of this project is to transfer data from a database to another. 

Data is gathered once per minute from 1499 unique sensors at Vestsiden lower secondary school in Kongsberg, Norway. This data is stored in a database which is not optimized for such a large volume. Along with each sensor reading, there is also stored a lot of excess data which is of no interest to the researchers making use of the data. 

At the time of writing(November 2020), this database consists of about 250 million rows, growing with 2.1 million rows every day. This database will throughout the project be referred to as the "dirty database".

![Structure of dirty database](sql/img/dirtyDb.png?raw=true "Title")


All of the sensor readings will continuously be transferred to a new database, which is throughout the project referred to as the "clean database". The design for this database can be seen in the figure below:


![Structure of clean database](sql/img/cleanDb.png?raw=true "Title")

As the majority of queries executed on the database will be to select sensor data for a set of sensors within a given timespan, I found that it would make sense to build the primary key out of the sensor id and the timestamp. In that particular order. 

The complete project consists of software written in C++ as well as Bash scripts. 


### Libraries
* [Boost](https://www.boost.org)
* [MySQL Connector/C++ 1.1](https://dev.mysql.com/doc/connector-cpp/1.1/en/)

##### Side note about the choice of version 1.1 of the MySQL C++ connector:
It is generally recommended to use version 8 these days. However since version 1.1 is still maintained, much easier to install, and the fact that I have previous experiences with it, I chose not to use version 8.


## Prerequisites

To get a local copy up and running, these are the prerequisites:

```sh
apt install tasksel  
tasksel install lamp-server 
mysql_secure_installation
apt install g++
apt install build-essentials
apt install libboost-all-dev
apt install libmysqlcppconn-dev
```


<!-- USAGE -->
## Usage

First you need fill the contents of the files in config/, with credentials for the old and the new database. Detailed instructions can be found in the corresponding [readme](config/README.md). In the same directory you can also find the [my.cnf](config/my.cnf) file, which contains settings for MySQL.

To create the database, use the [initDb.sh](bash/initDb.sh) script. Once created, it will also extract all sensors from the dirty database, and insert them into the clean database.

The next step is do to bulk insertion of existing data from the dirty database, and into the new. As this process involves hundreds of millions of rows, this is done with the LOAD FILE functionality in MySQL. Details can be found in the [readme](bash/README.md) in the bash directory. 

After this, it is recommended to run the script [compareDatabases.sh](bash/compareDatabases.sh), in order to make sure that all the data has in fact been extracted from the old database, and inserted into the new. This will also compare the size of the databases in MB.

At this point, you will most likely want to set up a cronjob, to execute the script [periodicInsert.sh](bash/periodicInsert.sh) at a regular interval. To do this, open up the cronjob config file with this command:

```
crontab -e
```
and insert this line at the bottom of the file:
```
0 5,17 * * * /PATH/TO/PROJECT/ROOT/bash/periodicDbInsert.sh
```
This will make the script run once every 12 hours, at 05:00 and 17:00.

<!-- LICENSE -->
## License

Distributed under the MIT License. See [`LICENSE`](LICENSE) for more information.

<!-- CONTACT -->
## Contact

Kent Odde - kentodde89@gmail.com

Project Link: [https://github.com/oddek/vestsiden](https://github.com/oddek/vestsiden)



