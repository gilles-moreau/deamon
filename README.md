# deamon
Distributed application based on Slurm. Individual node (or process) can be run on machines located 
in the same flat LAN. A multicast discovery algorithm builds the cluster.

This constitutes the core architecture on which a distributed system can be built. Next 
step is to include FUSE to build a distributed file system.

## Architecture
Paradigm is client-server. Each node has a socket-based message engine to send messages and 
receive responses.

## Multicast discovery algorithm
- Resilient to failure: whenever a process (controller or slave) is down, automatic 
registration is
