use vestsiden;

ALTER TABLE HISTORYNUMERICTRENDRECORD ADD INDEX `timestamp_index` (`TIMESTAMP`);
/* COMMIT; */
/* SET SESSION tx_isolation='READ-REPEATABLE'; */