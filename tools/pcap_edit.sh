#!/bin/bash

PCAP_FILE=$1
NEW_PCAP="../build/test.pcap"

OWN_MAC=$2
REMOTE_MAC=$3
OWN_IPV4=$4

TEST_OWN_MAC=AA:AA:AA:AA:AA:AA
TEST_REMOTE_MAC=BB:BB:BB:BB:BB:BB

TEST_OWN_IPV4=32.32.32.32

change_filename() {
    mv /tmp/2temp.pcap /tmp/1temp.pcap
}

bittwiste -I $PCAP_FILE -O /tmp/1temp.pcap -T eth -d $OWN_MAC,$TEST_OWN_MAC             
bittwiste -I /tmp/1temp.pcap -O /tmp/2temp.pcap -T eth -d $REMOTE_MAC,$TEST_REMOTE_MAC  && change_filename

bittwiste -I /tmp/1temp.pcap -O /tmp/2temp.pcap -T eth -s $OWN_MAC,$TEST_OWN_MAC        && change_filename
bittwiste -I /tmp/1temp.pcap -O /tmp/2temp.pcap -T eth -s $REMOTE_MAC,$TEST_REMOTE_MAC  && change_filename

bittwiste -I /tmp/1temp.pcap -O /tmp/2temp.pcap -T ip -s $OWN_IPV4,$TEST_OWN_IPV4       && change_filename
bittwiste -I /tmp/1temp.pcap -O $NEW_PCAP -T ip -d $OWN_IPV4,$TEST_OWN_IPV4



#./pcap_edit.sh ../build/ip.pcap 6c:ba:b8:ec:d0:5c 04:d4:c4:52:7d:67 172.64.149.23