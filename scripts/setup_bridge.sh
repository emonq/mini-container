#!/bin/bash

# Create a bridge network
ip link add name mini-container type bridge
ip addr add 172.20.0.1/16 dev mini-container
ip link set mini-container up