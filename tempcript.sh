#!/bin/bash
args=("$@")
if [ ${args[0]} == ${args[1]} ]; then 
	iptables -t nat -A OUTPUT -p tcp --dport 5001 -d 192.168.56.102 -j DNAT --to 192.168.56.1 
	iptables -t nat -A INPUT -p tcp --sport 5001 -s 192.168.56.1 -j SNAT --to 192.168.56.102 
else
	iptables -t nat -D OUTPUT -p tcp --dport 5001 -d 192.168.56.102 -j DNAT --to 192.168.56.1 
	iptables -t nat -D INPUT -p tcp --sport 5001 -s 192.168.56.1 -j SNAT --to 192.168.56.102 
fi
