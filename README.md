# WaggleNetNet

The repository is forked from nRF24/RF24Mesh and modified to fit our needs. The documentation of the original repository can be found at: https://tmrh20.github.io/RF24Mesh

## Overview

WaggleNetNet allows WaggleNodes (https://github.com/WaggleNet/WaggleNode) to form a tree-like mesh network using the nRF24L01 2.4GHz Wireless Transceiver hardware. It handles dynamic address allocation of the nodes with some additional address filtering and security features (currently developing). Some examples are provided to be run on the Arduino and Raspberry Pi platforms but other libraries need to be linked during compilation.

## Quick Setup with Arduino IDE

To run this, you need compatible software environment:

* Arduino IDE

You will also need at least two sets of the following hardware:

* WaggleNet DevBoard P3, Versions 1.0 +
* nRF24L01 2.4GHz Wireless Transceiver

### Environment setup with Arduino IDE

1. Make sure you have the board **ATMega328PB Internal Clock (8MHz)** in your board drop-down menu. If it doesn't show up, follow the instructions to install the library: https://github.com/watterott/ATmega328PB-Testing

2. Make sure you have the **nRF24Mesh** libraries installed. Instructions can be found here: http://tmrh20.github.io/RF24Mesh/Setup-Config.html

### Running the example program

There are two sample program you need to deal with: `examples/RF24Mesh_Example/RF24Mesh_Example.ino` (node) and `examples/RF24Mesh_Example_Master/RF24Mesh_Example_Master.ino` (master). There should be **one and only one master** in your network setup; there is no clear limit for the number of nodes in the network as of the moment. 

1. Before you open the Arduino IDE, copy into the two example directories, the following header and source files: `RF24AddressBook.h, RF24AddressBook.cpp, RF24Mesh.cpp, RF24Mesh.h`.

2. Now, open up the example programs. Select the corrent boad as introduced above; choose the right port; compile and download the program. If the download is successful, `avrdude done` should show up on your "command line."

### Monitoring the result

Choose the port, open up the serial monitor and set the baudrate to 115200 as specified in the setup section. You should be able see serial outputs from the board after a while. If you are working with a single computer, you can open up multiple instances of the Arduino IDE to monitor outputs from different ports. 

### Expected serial output

If the connections are established, the master side should display received values and the node side should display `Send OK!` and the value sent. 

## Quick setup with PlatformIO





## Troubleshooting

Outlined below are some common errors we have run into. Possible solutions to the problems are also provided for your reference. You can also document the problems you have encountered and your solutions here. 

1. 