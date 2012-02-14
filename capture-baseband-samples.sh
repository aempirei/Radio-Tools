#!/bin/bash

freq=0
length=10
rate=192000
datatype=double
let samples=$length*$rate

sudo /usr/local/share/uhd/examples/rx_samples_to_file --freq ${freq} --rate ${rate} --nsamps ${samples} --type ${datatype}
