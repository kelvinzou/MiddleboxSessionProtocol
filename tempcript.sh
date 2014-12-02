#!/bin/bash
args=("$@")
if [ ${args[0]} == ${args[1]} ]; then 
	iptables -t nat -A OUTPUT -p tcp --dport 5001 -d 192.168.56.102 -j DNAT --to 192.168.56.1 
else
	iptables -t nat -D OUTPUT -p tcp --dport 5001 -d 192.168.56.102 -j DNAT --to 192.168.56.1 
fi
