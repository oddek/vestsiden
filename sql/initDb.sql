drop database vestsiden;
CREATE DATABASE IF NOT EXISTS vestsiden DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci;

use vestsiden;

CREATE TABLE IF NOT EXISTS HISTORY_TYPE_MAP (
	ID smallint UNSIGNED NOT NULL AUTO_INCREMENT,
	NAME varchar(500) NOT NULL UNIQUE,
	VALUEFACETS varchar(500) NOT NULL,
	PRIMARY KEY (ID)
); 

CREATE TABLE IF NOT EXISTS HISTORYNUMERICTRENDRECORD (
	HISTORY_ID smallint UNSIGNED NOT NULL,
	TIMESTAMP int UNSIGNED NOT NULL,
	VALUE float NOT NULL,
	STATUS int NOT NULL,
	PRIMARY KEY (HISTORY_ID, TIMESTAMP)
);

SET sql_log_bin = 0;
