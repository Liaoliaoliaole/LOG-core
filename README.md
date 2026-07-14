<div align="center"> <img src="./Docs/Morfeas_project_Documentation/ArtWork/Morfeas_logo.png" width="100"> </div>

# Morfeas-Core
This repository related to Morfeas core where is a sub-project of the Morfeas Project.

### The name "Morfeas"
Morfeas is the Latinisation of the Greek word "Μορφέας" which translate as:
Him that can give or change form (or shape). The name "Morfeas" describe the design [Philosophy](#design-philosophy)
of the project. Many small building blocks that change the form of the information from one protocol to other(s).

### Project Description
The Morfeas project initially started as an implementation of a software gateway solution system
(currently named Morfeas-Proto) that provide (and translate) measurements data from some proprietary devices (SDAQ family)
with CANbus compatible interface (SDAQnet) to OPC-UA protocol (Open62541 based).

As the Morfeas project developed additional support added for other devices (MDAQ, IO-BOX, MTI, UniNOx) with different interfaces (ModBus-TCP, USB).

Furthermore, a web interface sub-project added to the Morfeas project under the name ["Morfeas-web"](https://gitlab.com/fantomsam/morfeas_web).
Thisof, will provide a layman friendly configuration interface for the gateway, the OPC-UA server's Nodeset and the connected devices.

**In relation with this project [Morfeas_RPi_Hat](./src/Morfeas_RPi_Hat)**
### Design Philosophy
The Morfeas-core project designed as a micro-component with supervision system.
This means that all the components called from a supervisor programs,
runs as daemon type processes, and they communicating using messages established from the **Morfeas IPC protocol**
that passing through **PIPEs**. All the project's components are written in **ANSI C**.

### Documentation
Project documentation located @:[/Docs/Morfeas_project_Documentation](./Docs/Morfeas_project_Documentation)

#### The Morfeas-core components:
* **Morfeas_daemon**: The supervisor.
* **Morfeas_opc_ua**: OPC-UA server.
* Driver/Handler for each supported interface:
  * **Morfeas_SDAQ_if**: SDAQ_net handler.
  * **Morfeas_MDAQ_if**: ModBus-TCP handler for MDAQs.
  * **Morfeas_IOBOX_if**: ModBus-TCP handler for IO-BOXes.
  * **Morfeas_MTI_if**: ModBus-TCP handler for MTIs.
  * **Morfeas_NOX_if**: CANBus handler for UniNOx.

### Requirements
For compilation of this project the following dependencies required.
* [GCC](https://gcc.gnu.org/) - The GNU Compilers Collection.
* [GNU Make](https://www.gnu.org/software/make/) - GNU make utility.
* [CMake](https://cmake.org/) - Cross-platform family of open source tools for package build.
* [OpenSSL](https://www.openssl.org/) - A full-featured toolkit for TLS and SSL protocols.
* [GnuTLS](https://gnutls.org/) - A secure communications library for SSL, TLS and DTLS protocols.
* [zlib](https://www.zlib.net/zlib_how.html) - A free software library used for data compression.
* [NCURSES](https://www.gnu.org/software/ncurses/ncurses.html) - A free (libre) software emulation library of curses.
* [GLib](https://wiki.gnome.org/Projects/GLib) - GNOME core application building blocks libraries.
* [LibGTop](https://developer.gnome.org/libgtop/stable/) - A library to get system specific data.
* [libxml2](http://xmlsoft.org/) - Library for parsing XML documents.
* [libmodbus](https://www.libmodbus.org/) - A free software library for communication via ModBus protocol.
* [Libsocketcan](https://directory.fsf.org/wiki/Libsocketcan) - Library to control some basic functions in SocketCAN from userspace.
* [libi2c](https://packages.debian.org/jessie/libi2c-dev) - A library that provide I2C functionality to Userspace.
* [libdbus](https://www.freedesktop.org/wiki/Software/dbus/#index1h1) - The library of the D-Bus.
<!--* [Libwebsockets](https://libwebsockets.org/) - An ANSI C library for implementing modern network protocols.
	* [libusb](https://libusb.info/) - An ANSI C library that provides generic access to USB devices.-->

##### Optionally
* [CAN-Utils](https://elinux.org/Can-utils) - CANBus utilities.
* [I2C-tools](https://packages.debian.org/jessie/i2c-tools) - Heterogeneous set of I2C tools for Linux kernel.

## The Source
The source of the Morfeas-core project have the following submodules:
* [SDAQ_worker](https://gitlab.com/fantomsam/sdaq-worker) - A libre (free) utilities suite for SDAQ devices.
* [cJSON](https://github.com/DaveGamble/cJSON) - An Ultralightweight open source JSON parser for ANSI C.
* [noPoll](http://www.aspl.es/nopoll/) - An open source ANSI C implementation of WebSocket (RFC 6455).
* [open62541](https://open62541.org/) - An open source C (C99) implementation of OPC-UA (IEC 62541).

### Get the Source
```bash
# Clone LOG-core
git clone <your_repo_url> Morfeas_core
cd Morfeas_core

# Pull submodules exactly as pinned by this repo
git submodule sync --recursive
git submodule update --init --recursive
```

## Pinned Dependency Baseline

### LOG core v1 (baseline)

- `src/cJSON`: `c859b25da02955fef659d658b8f324b5cde87be3`
- `src/noPoll`: `ce4da1a102be8a19269813d97562582e7ce94bcc`
- `src/open62541`: `f63e2a819aff6e468242dc2e54ccbd5b75d63654`
- `src/sdaq-worker`: `ccc690c57e4171762c42a7e6412b7d92ea5e789f`

Note: `nopoll` upstream `pkg-config` version string can still show `0.4.8.b456` even when pinned to commit `ce4da1a...`.

### LOG core v2 (validated Bookworm build)
Submodule commits pinned:

- `src/cJSON`: `12c4bf1986c288950a3d06da757109a6aa1ece38` (v1.7.15-44)
- `src/noPoll`: `fa8b8c90a69fed63b1f1e4f7e9e95672f34c054f` (0.4.9-2)
- `src/open62541`: `c1960fa4897d06c5fae8c41823fc446c8bbd6345` (v1.4.10-1104)
- `src/sdaq-worker`: `ccc690c57e4171762c42a7e6412b7d92ea5e789f` (heads/master — unchanged)

Notable upgrades from v1 → v2: open62541 moved from `f63e2a8` to `c1960fa` (v1.4.10 series), which is why the `UA_ServerConfig_clear` / `UA_ServerConfig_clean` compatibility shim was added. cJSON and noPoll received minor upstream bumps.

## Build and Rebuild
Use the provided scripts instead of manual per-library build commands.

### Full rebuild (dependencies + core)
Use when dependency pins changed or first setup on a new machine.

```bash
cd Morfeas_core
./build_core_full.sh
sudo systemctl restart Morfeas_system.service
```

### Code-only rebuild (core only)
Use when only LOG-core source code changed and dependency pins did not change.

```bash
cd Morfeas_core
./build_core_only.sh
```

### Quick runtime checks
```bash
systemctl is-active Morfeas_system.service
ps -ef | egrep 'Morfeas_daemon|Morfeas_opc_ua|Morfeas_SDAQ_if' | grep -v egrep
pkg-config --modversion open62541 libcjson nopoll
ldd /usr/local/bin/Morfeas_opc_ua | egrep 'open62541|cjson|nopoll|icu|ssl'
```

### Configuration for Morfeas_daemon, D-Bus and Systemd service
```bash
# Optional: copy the configuration directory
cp -r configuration ~/

# Edit runtime configuration
vim ~/configuration/Morfeas_config.xml

# Adjust service unit/override if needed
sudo systemctl edit Morfeas_system.service --full
sudo vim /etc/systemd/system/Morfeas_system.service.d/Morfeas_system.conf

# Adjust D-Bus policy if needed
sudo vim /etc/dbus-1/system.d/Morfeas_system.conf

# Start/enable service
sudo systemctl start Morfeas_system.service
sudo systemctl enable Morfeas_system.service
```

### Re-Compilation of the source
Guide link [here](./RE-INSTALL.md)

---

## What's New in v2

### 1. Dependency
- open62541 API compatibility: `UA_ServerConfig_clear` vs `UA_ServerConfig_clean` probed at link time via `__attribute__((weak))` — same binary works across open62541 build variants

### 2. SDAQ Address Allocation Redesign
The previous LogBook was append-only and accumulated duplicate entries over time, causing address conflicts. The current design:

- **Deduplicated TTL cache** — 1 record per S/N, 1 record per address; always rewritten atomically
- **Address reservation** — when a device goes offline, its address slot is reserved for that S/N for **14 days**; the device recovers its original address automatically on return
- **O(1) conflict check** — `address_owners[64]` in-memory table replaces linear list scans
- **On-card upgrade behavior** — if an existing SD card is upgraded in place and the stored SDAQ LogBook uses the previous format, Morfeas clears that address cache and rebuilds it from SDAQs that are currently online. This does not apply to a freshly imaged/replaced SD card. When the cache is rebuilt, SDAQ addresses may be reassigned according to the current conflict-free allocation rules.
- Max 62 simultaneous devices; extras go to Parking (addr 63) and retry automatically when a slot frees

### 3. Reserved Measurement Error Codes
Invalid or unavailable measurements carry typed numeric error codes instead of `NaN` or device-specific strings:

| Code | Meaning |
|------|---------|
| `-901` | Offline / disconnected source while the handler is alive |
| `-902` | No sensor |
| `-903` | Stall / heating / not ready |
| `-904` | Unclassified invalid runtime condition |
| `-905` | Source unregistered or device-level transport unreachable |
| `-906` | Standby / heater off |
| `-907` | Signal invalid / data invalid |

`Out of Range` and `Over Range` now preserve the measured value (previously discarded). Error codes propagate through IPC → OPC UA → web backend and display as red integers in Morfeas Web.

---

## License
The source code of the project is licensed under GPLv3 or later - see the [License](LICENSE) file for details.
