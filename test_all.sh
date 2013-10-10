#!/bin/bash

function Get_Status {
if [ $1 -ne 0 ]
then
echo "$2 FAILED! Exit Status: $1" >> $CURRENT_DIR/test_results.txt
echo -e "$2 \e[00;31mFAILED\e[00m Exit Status: $1" >> $CURRENT_DIR/TEMP.tmp
echo -e "\e[00;31m---------------------------------------------------\e[00m "
echo -e "\n\n $2 \e[00;31mFAILED\e[00m Exit Status: $1 \n\n "
echo -e "\e[00;31m---------------------------------------------------\e[00m "
else
echo "$2 PASSED! Exit Status: $1" >> $CURRENT_DIR/test_results.txt
echo -e "$2 \e[00;32mPASSED\e[00m Exit Status: $1" >> $CURRENT_DIR/TEMP.tmp
echo -e "\e[00;32m---------------------------------------------------\e[00m "
echo -e "\n\n $2 \e[00;32mPASSED\e[00m Exit Status: $1 \n\n "
echo -e "\e[00;32m---------------------------------------------------\e[00m "
fi
return 0
}
#############################################################################

# BIN_PATH describes the path to scripts required by executables.

# this script is expected to be in /libpcp/tests, else you have to change $BIN_PATH
# variable, which directs script to executables.
# paths to scripts for pcp-app and pcp-server must be changed manually

CURRENT_DIR=$(pwd)
BIN_PATH=$CURRENT_DIR/build/bin/Debug
PATH_SCRIPT=tests
PATH=$PATH:$BIN_PATH:$CURRENT_DIR/win_utils

echo "" > $CURRENT_DIR/test_results.txt
echo "" > $CURRENT_DIR/TEMP.tmp

export PCP_USE_IPV6_SOCKET=0

$PATH_SCRIPT/test_flow_notify.sh
Get_Status $? "test_flow_notify           "

$PATH_SCRIPT/test_version_negotiation.sh
Get_Status $? "test_version_negotiation   "

$PATH_SCRIPT/test_pcp_client_map_opcode.sh
Get_Status $? "test_pcp_client_map_opcode "

$PATH_SCRIPT/test_pcp_client_peer_opcode.sh
Get_Status $? "test_pcp_client_peer_opcode"

$PATH_SCRIPT/test_server_restart.sh
Get_Status $? "test_server_restart        "

$PATH_SCRIPT/test_get_dscp.sh
Get_Status $? "test_get_dscp              "

$PATH_SCRIPT/test_flow_md.sh
Get_Status $? "test_flow_md               "

$PATH_SCRIPT/test_lifetime_renewal.sh
Get_Status $? "test_lifetime_renewal      "

$PATH_SCRIPT/test_server_reping.sh
Get_Status $? "test_server_reping         "

test_event_handler.exe
Get_Status $? "test_event_handler         "

test_gateway.exe
Get_Status $? "test_gateway               "

test_pcp_client_db.exe
Get_Status $? "test_pcp_client_db         "

test_pcp_api.exe
Get_Status $? "test_pcp_api               "

test_sock_ntop.exe
Get_Status $? "test_sock_ntop             "

test_pcp_logger.exe
Get_Status $? "test_pcp_logger            "

test_pcp_msg.exe
Get_Status $? "test_pcp_msg               "

$PATH_SCRIPT/test_server_reping.sh
Get_Status $? "test_server_reping         "

$PATH_SCRIPT/test_pcp_app.sh
Get_Status $? "test_pcp_app               "

$PATH_SCRIPT/test_pcp_server.sh
Get_Status $? "test_pcp_server            "

test_ping_gws.exe
Get_Status $? "test_ping_gws              "

cat $CURRENT_DIR/TEMP.tmp
rm $CURRENT_DIR/TEMP.tmp

echo -e Testing ended, results in \'libpcp/tests/test_results.txt\'

exit
