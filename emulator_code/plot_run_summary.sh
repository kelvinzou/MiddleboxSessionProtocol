#!/bin/bash

# bwm-ng  -t 100 -I eth1 -o csv -T rate -C ','  -u bits -F drop_nobuffer

dir='./'
maxy=1000

#
#--maxy $maxy \
#       -i "eth1" \
python util/plot_rate.py \
       -f $dir/drop_nobuffer \
       --legend Bandwidth during drop-based flow migration \
       --xlabel 'Time (100ms)' \
       --ylabel 'Rate (Mbps)' \
       --maxy $maxy \
       -i "eth1" \
       --rx \
       --total \
       -o $dir/drop_nobuffer_bandwidth.png

