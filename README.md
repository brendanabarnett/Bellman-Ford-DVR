# Multithreaded Distance Vector Routing Protocol using the Bellman-Ford Algorithm
This program implements a distance vector routing system using the Bellman-Ford algorithm, simulating a network of nodes exchanging information to determine the shortest paths to other nodes in the network. It involves multiple threads, each representing a node in the network, calculating and updating the shortest paths simultaneously.

## Running the Program
This program may be compiled and ran using the following commands:
###### gcc -Wall -pthread dvr_bellman.c -o dvr_bellman
###### ./dvr_bellman

## Features
- ANSI/IEEE POSIX 1003.1 - 1995 standard used for POSIX thread implementation
- Initializes a network of nodes and computes shortest paths using the Bellman-Ford Algorithm
- Menu interface allows advancing relaxations, editing edges, or quitting

## Usage
- User inputs ('N' for next relaxation, 'E' to edit edges, 'Q' to quit) control the computation flow
- Edit edges by specifying a new edge: `from to weight`

## Additional Details
- For more information on the Bellman-Ford Algorithm or to see the initial graph, click [here](https://www.geeksforgeeks.org/bellman-ford-algorithm-dp-23/) 
- Note: The initial graph contains a negative cycle

## Next Steps
- Implement a graphical representation of the network to visualize node connections and shortest paths dynamically
- Optimize the code for performance and readability; decrease overhead
- Develop tools to measure and analyze network performance
    
