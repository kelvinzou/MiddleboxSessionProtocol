#!/bin/bash

# bwm-ng  -t 100 -I eth1 -o csv -T rate -C ','  -u bits -F nodrop_buffer

dir='./'
maxy=1000
filename="$1"
suffix=".png"
picname=$1$suffix
#
#--maxy $maxy \
#       -i "eth1" \
python util/plot_rate.py \
       -f $dir/$filename \
       --legend Bandwidth during drop-based flow migration \
       --xlabel 'Time (30ms)' \
       --ylabel 'Rate (Mbps)' \
       --maxy $maxy \
       -i "eth1" \
       --rx \
       --total \
       -o $dir/$picname

