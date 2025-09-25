#!/bin/bash
# Run ECU1 client inside its namespace
ip netns exec ecu1_ns ./build/ecu1
