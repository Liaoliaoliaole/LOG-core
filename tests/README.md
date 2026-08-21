# Core regression tests

Run these commands from the `LOG-core` repository root. The Makefile supplies
the same compiler and linker flags used by the production Core build; do not
reconstruct the link command manually.

```bash
make test-core-a1
make test-core-o
```

`test-core-a1` builds and runs `opcua_config_parser_test.c`. It covers the
strict OPC UA configuration grammar, semantic duplicate checks, supported
interface rules, and parser/list-builder agreement.

`test-core-o` builds and runs `sdaq_offline_browse_gate_test.c`. It starts an
embedded open62541 server and covers the SDAQ Unit browse gate, ISO-channel
deletion cleanup, and configuration file signature tracking. It does not open
a network listener or replace the real Gateway Configuration Tool test.

The test targets require the same development libraries listed in the root
`README.md` and resolved by the Makefile through `pkg-config`, notably
open62541, GLib, libxml2, libmodbus, libcjson, libgtop, libsocketcan, noPoll,
ncurses, and D-Bus. A quick dependency check is:

```bash
pkg-config --cflags --libs \
  open62541 libcjson ncurses libxml-2.0 libgtop-2.0 glib-2.0 \
  libsocketcan nopoll libmodbus dbus-1
```

For a complete source build followed by both regression suites:

```bash
make all
make test-core-a1 test-core-o
```
