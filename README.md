## ECU CAN Simulation with Fault Injection

A Linux-based simulation of two Electronic Control Units (ECUs) communicating over a virtual CAN bus, with a built-in fault injection framework.

Test how ECU software behaves when the CAN network experiences packet loss, delay, corruption, or a completely blocked link — all without physical hardware.

## Overview

This project simulates automotive communication under faulty network conditions using Linux kernel features:

Network namespaces: Isolate each ECU as a separate network node.
Virtual CAN tunnels (vxcan): Connect the namespaces.
CAN gateway (cangw): Forward CAN frames through a dedicated fault namespace.
Fault injection: Use tc netem and iptables to simulate network failures.
C++17 ECU programs: ecu1 acts as the client, while ecu2 acts as the server.
Controller script: Manages setup, execution, fault injection, reset, and teardown.
## Project Structure
```text

├── src/                  # ECU source code (ecu1.cpp, ecu2.cpp)
├── build/                # Compiled binaries
├── faults/               # Fault injection scripts
├── test/                 # Test flags, logs, and summaries
├── logs/                 # ECU runtime logs
├── Makefile              # Builds both ECUs
├── controller.sh         # Main controller
├── setup_netns.sh        # Creates namespaces and virtual links
├── cleanup.sh            # Cleans up resources
├── inject_combo.sh       # Applies combined faults
├── run_ecu1.sh           # Runs ECU1 manually
├── run_ecu2.sh           # Runs ECU2 manually
└── fault_summary_can.txt # Example fault summary
```

## Prerequisites

Install the required packages on Ubuntu or Debian:

sudo apt update
sudo apt install -y build-essential iproute2 can-utils iptables expect

Getting Started

Clone the repository and build the project:

```bash
git clone https://github.com/mohamed-rayen-Annabii/internship-project-ECU-CAN-Simulation.git

cd internship-project-ECU-CAN-Simulation

make

chmod +x *.sh faults/*.sh test/*.sh
```

## Usage
1. Start the simulation

Run these commands in Terminal 1:

sudo ./controller.sh start
sudo ./controller.sh run

2. Inject network faults

In Terminal 2, inject one or more faults:

Simulate delay in both directions:

sudo ./controller.sh inject delay


Simulate packet loss in one direction:

sudo ./controller.sh inject loss --iface=fault1


Combine multiple faults:

sudo ./controller.sh inject delay loss corruption

3. Reset faults
sudo ./controller.sh reset

4. Stop the simulation and generate the summary
sudo ./controller.sh stop


The controller stops the ECUs, cleans up network resources, and generates the test summary at test/results.md.
