#!/bin/bash


iperf -c 52.5.27.99 -p 5001 -i 1 -t 100 > output1 &
iperf -c 52.5.27.99 -p 5002 -i 1 -t 100 > output2 &
iperf -c 52.5.27.99 -p 5003 -i 1 -t 100 > output3 &
