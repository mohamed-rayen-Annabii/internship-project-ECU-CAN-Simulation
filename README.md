ECU CAN Simulation with Fault Injection

A Linux-based simulation of two Electronic Control Units (ECUs) communicating over a virtual CAN bus, with a built-in fault injection framework. It lets you test how ECU software behaves when the CAN network is degraded by packet loss, delay, corruption, or a fully blocked link, all without any physical hardware.

Testing automotive communication under faulty network conditions normally requires dedicated hardware. This project reproduces that setup entirely in software using Linux kernel features:
Network namespaces isolate each ECU, so every one behaves like a separate node.
Virtual CAN tunnels (vxcan) connect the namespaces.
A dedicated fault namespace sits between the two ECUs. A CAN gateway (cangw) forwards frames through it, and this is where faults are injected using tc netem and iptables.
Two C++17 programs (ecu1 as client, ecu2 as server) exchange CAN frames and record what was received, lost, delayed, or corrupted.
A controller script drives the whole workflow: setup, run, inject, reset, and teardown.

Project Structure :

├── src/                  # ECU source code (ecu1.cpp, ecu2.cpp)
├── build/                # Compiled binaries (ecu1, ecu2), created by make
├── faults/               # Fault injection scripts (loss, delay, corruption, block, reset)
├── test/                 # Test flags, logs and summary generation
├── logs/                 # ECU runtime logs, created on first run
├── Makefile              # Builds both ECUs
├── controller.sh         # Main entry point: start / run / inject / reset / stop
├── setup_netns.sh        # Creates namespaces, vxcan links, and cangw forwarding
├── cleanup.sh            # Removes namespaces, interfaces, and leftover processes
├── inject_combo.sh       # Applies several faults at the same time
├── run_ecu1.sh           # Runs ECU1 manually inside its namespace
├── run_ecu2.sh           # Runs ECU2 manually inside its namespace
└── fault_summary_can.txt # Example fault summary output


Prerequisites :
sudo apt update
sudo apt install -y build-essential iproute2 can-utils iptables expect
Getting Started :
git clone https://github.com/mohamed-rayen-Annabii/internship-project-ECU-CAN-Simulation.git
cd internship-project-ECU-CAN-Simulation
make
chmod +x *.sh faults/*.sh test/*.sh

Usage :
in Terminal 1 :
./controller.sh start     # Create namespaces, vxcan links, and cangw forwarding
./controller.sh run       # Launch ECU1 and ECU2; output is also saved to logs/

in Terminal 2 :
./controller.sh inject delay                       # one fault, both directions
./controller.sh inject loss --iface=fault1         # one fault, one direction
./controller.sh inject delay loss corruption       # combined faults
./controller.sh reset                              # remove all faults

./controller.sh stop      # Stop ECUs, reset faults, clean up, generate the test summary at test/results.md

