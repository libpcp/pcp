#!/usr/bin/python
#
# Copyright (c) 2013 by Cisco Systems, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import logging
from scapy.all import *
import random

pcp_opcodes={   1 : "map",
                2 : "peer",
                3 : "sadscp",
                129 : "map_res",
                130 : "peer_res",
                131 : "sadscp_res"
    }

pcp_options={   1 : "third_party",
                2 : "prefer_failure",
                3 : "filter"
    }

class PCP(Packet):
	name = "PCP"
	fields_desc = [ ByteField("version",2),
                    ByteEnumField("opcode", 2, pcp_opcodes),
                    #PCP request specific fields
                    ConditionalField(ShortField("reserved",0), lambda pkt: pkt.opcode in [1,2,3] ),
                    ConditionalField(IntField("lifetime", 1000), lambda pkt: pkt.opcode in [1,2,3] ),
                    ConditionalField(Emph(IP6Field("src_ip", "::ffff:127.0.0.1")), lambda pkt: pkt.opcode in [1,2,3] ),
                    #PCP response specific fields
                    ConditionalField(ByteField("reserved",0), lambda pkt: pkt.opcode in [129,130] ),
                    ConditionalField(ByteField("result",0), lambda pkt: pkt.opcode in [129,130]),
                    ConditionalField(IntField("lifetime", 1000), lambda pkt: pkt.opcode in [129,130]),
                    ConditionalField(IntField("epoch", 0), lambda pkt: pkt.opcode in [129,130]),
                    ConditionalField(IntField("reserved1", 0), lambda pkt: pkt.opcode in [129,130]),
                    ConditionalField(IntField("reserved2", 0), lambda pkt: pkt.opcode in [129,130]),
                    ConditionalField(IntField("reserved3", 0), lambda pkt: pkt.opcode in [129,130]),
                    #Nonce that goes with MAP, PEER and SADSCP opcodes
                    ConditionalField(IntField("nonce1",random.randint(0,0xffffffff)), lambda pkt: pkt.opcode in [1,2,3,129,130,131] and pkt.version == 2 ),
                    ConditionalField(IntField("nonce2",random.randint(0,0xffffffff)), lambda pkt: pkt.opcode in [1,2,3,129,130,131] and pkt.version == 2 ),
                    ConditionalField(IntField("nonce3",random.randint(0,0xffffffff)), lambda pkt: pkt.opcode in [1,2,3,129,130,131] and pkt.version == 2 ),
                    #MAP and PEER specific fileds
                    ConditionalField(ByteField("protocol", 6), lambda pkt: pkt.opcode in [1,2,129,130] ),
                    ConditionalField(ByteField("reserv1", 0), lambda pkt: pkt.opcode in [1,2,129,130] ),
                    ConditionalField(ShortField("reserv2", 0), lambda pkt: pkt.opcode in [1,2,129,130] ),
                    ConditionalField(ShortField("int_port", 0), lambda pkt: pkt.opcode in [1,2,129,130] ),
                    ConditionalField(ShortField("ext_port", 0), lambda pkt: pkt.opcode in [1,2,129,130] ),
                    ConditionalField(Emph(IP6Field("ext_ip", "::ffff:127.0.0.1")), lambda pkt: pkt.opcode in [1,2,129,130] ),
                    #PEER specific
                    ConditionalField(ShortField("peer_port", 0), lambda pkt: pkt.opcode in [2,130]),
                    ConditionalField(ShortField("reserv3", 0), lambda pkt: pkt.opcode in [2,130] ),
                    ConditionalField(Emph(IP6Field("peer_ip", "::ffff:127.0.0.1")), lambda pkt: pkt.opcode in [2,130] ),

                    ]

class PCP_OPTION(Packet):
    name="PCP Option"
    fields_desc = [ ByteEnumField("option_code", 2, pcp_options),
                    ByteField("reserved",0),
                    ShortField("length",0),
# Third Party specific
                    ConditionalField(Emph(IP6Field("int_ip", "::ffff:127.0.0.1")), lambda pkt: pkt.option_code == 1),
# Filter specific
                    ConditionalField(ByteField("reserved2", 0), lambda pkt: pkt.option_code == 3),
                    ConditionalField(ByteField("prefix_len", 0), lambda pkt: pkt.option_code == 3),
                    ConditionalField(ShortField("peer_port", 0), lambda pkt: pkt.option_code == 3),
                    ConditionalField(Emph(IP6Field("peer_ip", "::ffff:127.0.0.1")), lambda pkt: pkt.option_code == 3),

    ]

bind_layers(UDP, PCP, sport=5351)
bind_layers(UDP, PCP, dport=5351)
bind_layers(PCP, PCP_OPTION)
bind_layers(PCP_OPTION, PCP_OPTION)
conf.L3socket=L3RawSocket # needed for sending packets to localhost

if __name__=='__main__':
    interact(mydict=globals(), mybanner="PCP Addon")
