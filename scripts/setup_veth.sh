#!/bin/bash

# $1: container id, $2: container pid, $3: ip, $4: gateway
ip link add name veth$1-0 type veth peer name veth$1-1

VETH_INSIDE=veth$1-0
VETH_OUTSIDE=veth$1-1
NS=ns$2

mkdir -p /var/run/netns
ln -s /proc/$2/ns/net /var/run/netns/$NS

ip link set dev $VETH_OUTSIDE master mini-container
ip link set $VETH_INSIDE netns $2
ip netns exec $NS ip link set dev $VETH_INSIDE up
ip netns exec $NS ip addr add $3 dev $VETH_INSIDE
ip netns exec $NS ip route add default via $4
ip netns exec $NS ip link set $VETH_INSIDE name eth0
ip netns exec $NS ip link set dev eth0 up
ip netns delete $NS
ip link set dev $VETH_OUTSIDE up