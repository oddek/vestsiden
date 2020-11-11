#!bin/bash
echo 'About to sort readings.csv'
#sort file based on two first columns(timestamp and sensorid)
sort -t, -k 1,1n -k 2,2n ../loadfiles/readings.csv
echo 'Sorting complete, starting to split..'
#split into files of max a million, read1.csv, read2.csv...
split -dl 1000000 --additional-suffix=.csv readings.csv read
