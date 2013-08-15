Net-Modeler
===========

This is an **extremely alpha** version of a replacement for
[ModelNet][1], which is intended to be able to emulate
arbitrary network topologies, including their bandwidth and
delays.

Why replace ModelNet?
---------------------

ModelNet is an incredibly useful research tool which
unfortunately, as with most things of its ilk, has become tired
and crufty. It will, as far as I can tell, only run on FreeBSD
6.3, which prevents use of more modern operating systems. This is
important because more modern operating systems have modern
network drivers, and thus can support better technology.

How does it work?
-----------------

Essentially Net-Modeler uses the [Netfilter][netfilter] hooks built into
modern versions of Linux to yank 'tagged' packets out of the
kernels network stack, delay them for a particular period of
time, then reinsert them into the network stack.

Try it out
----------

You're welcome to download and try out the current state of
affairs. **It will almost certainly cause kernel panics so do it
in a VM.**

    # Dependencies 
    $ sudo apt-get install build-essential
    $ sudo apt-get install linux-kernel-headers-$(uname -r)

    # Grab it
    $ git clone https://github.com/cwacek/net-modeler.git
    $ cd net-modeler

    $ # Build it
    $ make
    $ make module-install

The module is now installed and running. Now you need to
configure a topology. This works in a couple of steps:

1. Register a *model* defining the name of the
   topology and the number of nodes and links it will have.
2. Register *hops* which define the characteristics of the
    link between two virtual points.
3. Register *paths* which define how IP addresses are connected
   using those *hops*.

There's a tool to do just this:

    $ cd tools/net-modeler-ctrl

    $ # Register a model with 4 hops and 3 paths called 'Test'
    $ python net_modeler.py modelinfo set 21 "Test" 4 3

    $ # Register two hops with 100KBps bandwidth, 10ms latency, and 
    $ # a bufferlength of 100.
    $ python net_modeler.py hoptable set 0 100 10 100
    $ python net_modeler.py hoptable set 1 100 10 100

    $ # Create a path between 10.0.0.1 and 10.0.0.2 using hop 0,
    $ # and the reverse using hop 1.
    $ python net_modeler.py pathtable set 10.0.0.1 10.0.0.2 0
    $ python net_modeler.py pathtable set 10.0.0.2 10.0.0.1 1

You now have links instantiated. You also have to tell the
operating system that it should use those IP addresses:

    $ sudo ifconfig eth0:1 10.0.0.1
    $ sudo ifconfig eth0:2 10.0.0.2

Try it out:

    $ ping -I 10.0.0.1 10.0.0.2
    PING 10.0.0.2 (10.0.0.2) from 10.0.0.1 : 56(84) bytes of
    data.
    64 bytes from 10.0.0.2: icmp_req=1 ttl=64 time=19.5 ms
    64 bytes from 10.0.0.2: icmp_req=2 ttl=64 time=21.0 ms
    64 bytes from 10.0.0.2: icmp_req=3 ttl=64 time=21.0 ms


[1]: [http://modelnet.ucsd.edu/]
[netfilter]: http://www.netfilter.org/documentation/HOWTO//netfilter-hacking-HOWTO-4.html
