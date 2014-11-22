"""Custom topology example
Author Kelvin Xuan Zou. 
This is the python mininet start script, and it generates the network and xterminal. 


"""

"""
          h3(M2)
          |
      /---s2-------\
h1--s1              s3----h5
      \		    	/
       \           /
	h2 M1     (h4) M3
"""



from mininet.topo import Topo
from mininet.net import Mininet
from mininet.log import lg, output
from mininet.node import CPULimitedHost, RemoteController, Host, Controller
from mininet.link import TCLink
from mininet.util import irange, custom, quietRun, dumpNetConnections
from mininet.cli import CLI
from time import sleep, time
from multiprocessing import Process
from subprocess import Popen
import random
from functools import partial
import argparse

import sys
import os
import signal

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


        # Add links
        self.addLink( Host1, leftSwitch )
        self.addLink( Host2, rightSwitch )
        self.addLink( MBox1, leftSwitch )
        self.addLink( MBox3 , rightSwitch )
	self.addLink( MBox2, midSwitch)	
	
        self.addLink( leftSwitch, midSwitch )
        self.addLink( rightSwitch, midSwitch )


topos = { 'custopo': ( lambda: TestTopo() ) }

def bootMininet():
    #create mininet with the topology
    host = custom(CPULimitedHost, cpu=0.2)
    link = custom(TCLink, bw=100, delay='20ms')
    topo = TestTopo()
    #OVSSwitch, KernelSwitch controller=  RemoteController,

    net = Mininet(topo=topo,  controller= partial( RemoteController, ip='127.0.0.1', port=6633 ), host=host, link=link, build=True, autoPinCpus=True )
    net.start()
    net.startTerms()
    print("Background process!")
    CLI(net)    
    net.stop()

def main():
    bootMininet()
    return
if __name__ == '__main__':
    # Tell mininet to print useful information
    main()


