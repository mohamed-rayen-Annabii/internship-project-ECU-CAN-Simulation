#!/bin/bash
# Run ECU2 server inside its namespace
ip netns exec ecu2_ns ./build/ecu2
