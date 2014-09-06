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
          h2
          |
      /---s2-------\
h1--s1              s3----h3
      \----s4------/
            |
            h4

'''


from mininet.topo import Topo

class TestTopo( Topo ):
    #this is a simple topolgoy that has 4 switches and 4 hosts

    def __init__( self ):

        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        leftHost = self.addHost( 'h1' )
        upHost = self.addHost('h2')
        rightHost = self.addHost( 'h3' )
        downHost = self.addHost('h4')
        leftSwitch = self.addSwitch( 's1' )
        rightSwitch = self.addSwitch( 's3' )
        upSwitch = self.addSwitch('s2')
        downSwitch = self.addSwitch('s4')

        # Add links
        self.addLink( leftHost, leftSwitch )
        self.addLink( rightSwitch, rightHost )
        self.addLink( upSwitch, upHost )
        self.addLink( downSwitch , downHost )

        self.addLink( leftSwitch, upSwitch )
        self.addLink( rightSwitch, upSwitch )
        self.addLink( leftSwitch, downSwitch )
        self.addLink( rightSwitch, downSwitch )


#topos = { 'TestTopo': ( lambda: TestTopo() ) }

def bootMininet():
    #create mininet with the topology
    host = custom(CPULimitedHost, cpu=0.2)
    link = custom(TCLink, bw=10, delay='0ms')
    topo = TestTopo()
    net = Mininet(topo=topo, controller= RemoteController,  host=host, link=link, build=True, cleanup=True, autoPinCpus=True, autoSetMacs=True, listenPort = 6633)
    net.start()
    print "start the net already"
    net.pingAll()
    hosts = [net.hosts[1],net.hosts[2]]
    net.iperf(hosts, 'TCP', '10M')
    net.stop()




def main():
    bootMininet()
    return
if __name__ == '__main__':
    # Tell mininet to print useful information
    main()






