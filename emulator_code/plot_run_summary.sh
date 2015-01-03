#!/bin/bash
#-t ms unit -F output file name
# bwm-ng  -t 100 -I eth1 -o csv -T rate -C ','  -u bits -F nodrop_buffer

dir='./'
maxy=1000
filename="$1"
suffix=".png"
picname=$1$suffix
# --rx \
#--maxy $maxy \
#       -i "eth1" \
python util/plot_rate.py \
       -f $dir/$filename \
       --legend Bandwidth during drop-based flow migration \
       --xlabel 'Time (100ms)' \
       --ylabel 'Rate (Mbps)' \
       --maxy $maxy \
       -i "total" \
       --total \
       -o $dir/$picname

