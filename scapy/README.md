PCP Extension for Scapy
=======================

The scapy_pcp.py defines new PCP layer to be used with Scapy.
You can find Scapy documentation on the following address
http://www.secdev.org/projects/scapy/doc/index.html

Quick intro
-----------

    ~$ sudo ./scapy_pcp.py

This will start interactive mode in which PCP protocol extensions will be
loaded, and they can be used.

You can take a look at the fields available in PCP layer. Here is the
list of fields that you can edit in PCP for various opcodes:

    >>> PCP(opcode=1).show()
    ###[ PCP ]###
        version   = 2
        opcode    = map
        reserved  = 0
        lifetime  = 1000
        src_ip    = ::ffff:127.0.0.1
        nonce1    = 458271020
        nonce2    = 681028287
        nonce3    = 4231934202L
        protocol  = 6
        reserv1   = 0
        reserv2   = 0
        int_port  = 0
        ext_port  = 0
        ext_ip    = ::ffff:127.0.0.1
        
    >>> PCP(opcode=2).show()    
    ###[ PCP ]###
        version   = 2
        opcode    = peer
        reserved  = 0
        lifetime  = 1000
        src_ip    = ::ffff:127.0.0.1
        nonce1    = 458271020
        nonce2    = 681028287
        nonce3    = 4231934202L
        protocol  = 6
        reserv1   = 0
        reserv2   = 0
        int_port  = 0
        ext_port  = 0
        ext_ip    = ::ffff:127.0.0.1
        peer_port = 0
        reserv3   = 0
        peer_ip   = ::ffff:127.0.0.1
        
    >>> PCP(opcode=3).show()
    ###[ PCP ]###
        version   = 2
        opcode    = sadscp
        reserved  = 0
        lifetime  = 1000
        src_ip    = ::ffff:127.0.0.1
        nonce1    = 458271020
        nonce2    = 681028287
        nonce3    = 4231934202L


Instead of numbers you can also use following syntax::

    >>> PCP(opcode="map").show()
    >>> PCP(opcode="peer").show()
    >>> PCP(opcode="sadscp").show()

To see fields that can be set in PCP options you can use::

    >>> PCP_OPTION(option_code="third_party").show()
    >>> PCP_OPTION(option_code="prefer_failure").show()
    >>> PCP_OPTION(option_code="filter").show()

You can use similar syntax to check the fields in other layers as well.

For example of IP or UDP layer::

    >>> ls(IP())
    >>> IP().show()

Creating and sending PCP message
--------------------------------

To create PCP message you need to create IP and UDP layers and then add
PCP layer, with appropriate fields values set to certain values.

    >>> packet=  IP(dst="127.0.0.1")/\
        UDP(sport=55555, dport=5351)/\
        PCP(    opcode="map",
                version=1,
                src_ip="::ffff:10.10.10.11",
                lifetime=1200,
                result=12,
                ext_port=1234,
                protocol=6,
                int_port=3333)

The above command will create packet with destination 127.0.0.1 in IP header.
Source port at UDP layer will be 55555 and destination port will be port where
our PCP server is running.
You can change all the fields in PCP portion of the packet, so for example even
reserved fields can have custom value instead of default ZERO. This is ideal
for testing how PCP server handles various malformed requests.

To create packet with PCP option use PCP_OPTION() to append desired option.
You can add multiple options simply by stacing multiple PCP_OPTION() behind
each other::

    >>> packet=  IP(dst="127.0.0.1")/\
        UDP(sport=55555, dport=5351)/\
        PCP(    opcode="map",
                version=1,
                src_ip="::ffff:10.10.10.11",
                lifetime=1200,
                result=12,
                ext_port=1234,
                protocol=6,
                int_port=3333)/PCP_OPTION(option_code=”prefer_failure”)/\
                               PCP_OPTION(option_code=”third_party”, length=16)



Sending packet is done using Scapy commands sr() or sr1().

    >>> resp_pkt=sr1(packet)
    >>> resp_pkt.show()

You can then use resp_pkt to examine the response packet.

The return of sr() is little more complicated as it can send and receive
multiple packets. It returns two lists of packets, answered and unanswered.
To iterate over received pakcets in answered list, one would use something
similar to following code snippet::

    ans,unans = sr(d)
    for snd,rcv in ans:
          rcv.show()


Sniffing packets with Scapy
---------------------------

You can sniff packets from Scapy session with PCP Addon, by using
sniff() function::

    Welcome to Scapy (2.2.0-dev)
    PCP Addon
    >>> packets=sniff(filter="udp port 5351", prn=lambda x: x.summary(), count=2)
    Ether / IP / UDP 127.0.0.1:40167 > 127.0.0.1:5351 / PCP
    >>> packets[1].show()

This will sniff two packets to list packets. You can then access the information
stored with packets[1] syntax and inspect the information in packets.


