# Morfeas-Core

## Preamp
This repository related to Morfeas core where is a sub-project of the Morfeas Project.

### The name "Morfeas"
Morfeas is the Latinisation of of the Greek word "Μορφέας" which translate as:
Him that can give or change form (or shape). The name "Morfeas" describe the design [Philosophy](#design-philosophy)
of the project, Many small building blocks that change the form of the information from one protocol to other(s).

### Project Description
The Morfeas project was initially start as an implementation of a software gateway solution system
(currently named Morfeas-Proto) that provide (and translate) measurements data from some proprietary devices (SDAQ family)
with CANbus compatible interface (SDAQnet) to OPC-UA protocol (Open62541 based).

As the Morfeas project developed additional support added for other devices (MDAQ, IO-BOX, MTI) with different interfaces (ModBus-TCP, USB).

Furthermore, a web interface sub-project added to the Morfeas project under the name "Morfeas-web".
Thisof, provide a layman friendly configuration interface for the gateway, the OPC-UA server's Nodeset and the connected devices.
### Design Philosophy
The Morfeas-core project designed as a micro-component with supervision system.
This means that all the components called from a supervisor programs,
runs as daemon type processes, and they communicating using messages established from the **Morfeas IPC protocol**
that passing through **PIPEs**. All the project's components are written in **ANSI C**.
#### The Morfeas-core components:
* **Morfeas**: The supervisor.
* **Morfeas_opc_ua**: OPC-UA server
* Driver/Handler for each supported interface:
  * **Morfeas_SDAQ_if**: SDAQ_net handler
  * **Morfeas_MDAQ_if**: ModBus-TCP handler for MDAQs
  * **Morfeas_IOBOX_if**: ModBus-TCP handler for IO-BOXes
  * **Morfeas_MTI_if**: ModBus-TCP and USB  handler for MTIs

**MTI**: Mobile Telemetry Interface.

### Requirements
For compilation of this project the following dependencies required.
* [GCC](https://gcc.gnu.org/) - The GNU Compilers Collection
* [GNU Make](https://www.gnu.org/software/make/) - GNU make utility
* [CMake](https://cmake.org/) - Cross-platform family of open source tools for package build.
* [NCURSES](https://www.gnu.org/software/ncurses/ncurses.html) - A free (libre) software emulation library of curses.
* [GLib](https://wiki.gnome.org/Projects/GLib) - GNOME core application building blocks libraries.
* [LibGTop](https://developer.gnome.org/libgtop/stable/) - A library to get system specific data
* [libxml2](http://xmlsoft.org/) -  Library for parsing XML documents
* [libusb](https://libusb.info/) - A ANSI C library that provides generic access to USB devices.
* [libmodbus](https://www.libmodbus.org/) - A free software library for communication via ModBus protocol.

##### Optionally
* [CAN-Utils](https://elinux.org/Can-utils) - CANBus utilities

## The Source
The source of the Morfeas-core project have the following submodules:
* [SDAQ_worker](https://gitlab.com/fantomsam/sdaq-worker) - A libre (free) utilities suite for SDAQ devices.
* [cJSON](https://github.com/DaveGamble/cJSON) - An Ultralightweight open source JSON parser for ANSI C.
* [open62541](https://open62541.org/) - An open source C (C99) implementation of OPC-UA.

### Get the Source
```
$ # Clone the project's source code
$ git clone https://gitlab.com/fantomsam/morfeas_project.git Morfeas_core
$ cd Morfeas_core
$ # Get Source of the submodules
$ git submodule update --init --recursive --remote --merge
```
### Compilation of the submodules
#### cJSON
```
$ cd src/cJSON
$ mkdir build && cd build
$ cmake ..
$ make -j$(nproc)
$ sudo make install
$ sudo ldconfig
```
#### open62541
```
$ cd src/open62541
$ mkdir build && cd build
$ cmake -D BUILD_SHARED_LIBS=ON ..
$ make -j$(nproc)
$ sudo make install
$ sudo ldconfig
```
#### SDAQ_worker
```
$ cd src/sdaq_worker
$ make tree
$ make -j$(nproc)
$ sudo make install
```
### Compilation of the Morfeas-core Project
```
$ make tree
$ make -j$(nproc)
```
The executable binaries located under the **./build** directory.

## Authors
* **Sam Harry Tzavaras** - *Initial work*

## License
The source code of the SDAQ_worker project is licensed under GPLv3 or later - see the [License](LICENSE) file for details.


