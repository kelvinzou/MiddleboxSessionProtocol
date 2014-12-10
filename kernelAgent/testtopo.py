'''


author: Kelvin Zou

File: testtopo
Description:
This function boots mininet and invokes iperf functions

'''


from mininet.topo import Topo
from mininet.net import Mininet
from mininet.log import lg, output
from mininet.node import CPULimitedHost, RemoteController, Host
from mininet.link import TCLink
from mininet.util import irange, custom, quietRun, dumpNetConnections
from mininet.cli import CLI
from time import sleep, time
from multiprocessing import Process
from subprocess import Popen
import random

import argparse

import sys
import os
import signal
from time import time

'''
          h3(M2)
          |
      /---s2-------\
h1--s1              s3----h5
      \		        /
       \           /
	h2 M1     (h4) M3
'''


from mininet.topo import Topo

class TestTopo( Topo ):
    #this is a simple topolgoy that has 4 switches and 4 hosts

    def __init__( self ):

        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        Host1 = self.addHost( 'h1' )
        MBox1 = self.addHost('h2')
        MBox2 = self.addHost( 'h3' )
	MBox3 = self.addHost('h4')
	Host2 = self.addHost('h5')

        leftSwitch = self.addSwitch( 's1' )
        rightSwitch = self.addSwitch( 's3' )
        midSwitch = self.addSwitch('s2')


        # Add linnet.startTerm() ks
        self.addLink( Host1, leftSwitch )
        self.addLink( Host2, rightSwitch )
        self.addLink( MBox1, leftSwitch )
        self.addLink( MBox3 , rightSwitch )
	self.addLink( MBox2, midSwitch)	
	
        self.addLink( leftSwitch, midSwitch )
        self.addLink( rightSwitch, midSwitch )


#topos = { 'TestTopo': ( lambda: TestTopo() ) }

def bootMininet():
    #create mininet with the topology
    host = custom(CPULimitedHost, cpu=0.2)
    link = custom(TCLink, bw=100, delay='20ms')
    topo = TestTopo()
<<<<<<< HEAD
    #OVSSwitch, KernelSwitch  controller= RemoteController,
=======
    #OVSSwitch, KernelSwitch controller=  RemoteController,

>>>>>>> df378e2f4c4322a32b941548eb055b0d0e200033
    net = Mininet(topo=topo,  host=host, link=link, build=True, autoPinCpus=True, autoSetMacs=True, listenPort = 6633)
    net.start()
    print("Background process!")
    CLI(net)    
    h1 = net.get('h1')
    print "Host", h1.name, "has IP address", h1.IP(), "and MAC address", h1.MAC()
    host1.cmd("./RECEIVE_RAW >/disk/local/kelvinzou/MiddleboxSessionProtocol/mininet/log1 &")
    print "start the net already"
    net.pingAll()

    sleep(5)
    hosts = [net.hosts[1],net.hosts[2]]
    print "Stop testing"
    host1.cmd("kill %./RECEIVE_RAW")
    print"successfully killed demon"
    #host1.cmd("echo hello")
    net.iperf(hosts, 'TCP', '10M')
    net.stop()




def main():
    bootMininet()
    return
if __name__ == '__main__':
    # Tell mininet to print useful information
    main()






