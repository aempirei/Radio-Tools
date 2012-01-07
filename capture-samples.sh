#!/bin/bash

freq=101900000
length=10
rate=400000
antenna="TX/RX"
datatype=double
let samples=$length*$rate

sudo /usr/local/share/uhd/examples/rx_samples_to_file --freq ${freq} --rate ${rate} --nsamps ${samples} --ant ${antenna} --type ${datatype}
