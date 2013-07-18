#!/bin/bash

killall pcp-server
pcp-server --ear 3 &>/dev/null &>/dev/null &
sleep 1
test_server_reping
