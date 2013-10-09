#!/bin/bash

killall pcp-server
pcp-server --ear 4 &>/dev/null &
sleep 1
test_server_reping
