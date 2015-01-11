#!/bin/bash
#-t ms unit -F output file name
# bwm-ng  -t 100 -I eth1 -o csv -T rate -C ','  -u bits -F nodrop_buffer

dir='./'
maxy=1200
filename="$1"
suffix=".png"
picname=$1$suffix
# --rx \
#--maxy $maxy \
#       -i "eth1" \
python util/plot_rate.py \
       -f $dir/$filename  ./drop_buffer \
       --legend "5 ms migration latency" "100 ms migration latency" \
       --xlabel 'Time (100ms)' \
       --ylabel 'Rate (Mbps)' \
       --maxy $maxy \
       -i "total" \
       --total \
       -o $dir/$picname

