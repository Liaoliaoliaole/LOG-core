# Core regression tests

Run these commands from the `LOG-core` repository root. The Makefile supplies
the same compiler and linker flags used by the production Core build; do not
reconstruct the link command manually.

```bash
make test-core-a1
make test-core-d
make test-core-o
make test-core-sdaq-cache
make test-core-logbook-disk
make test-core-nox
# Run all maintained project suites:
make test-core-all
```

`test-core-a1` builds and runs `opcua_config_parser_test.c`. It covers the
strict OPC UA configuration grammar, semantic duplicate checks, supported
interface rules, and parser/list-builder agreement.

`test-core-d` runs the production `Morfeas_daemon_config_valid()` function
against `tests/fixtures/daemon_config_validation_cases.json`. Web's
`logConfigValidationTest.php` consumes the same corpus, so a semantic rule
change cannot silently make only one layer accept a configuration.

`test-core-sdaq-cache` links the production `Morfeas_SDAQ_if.c` with its
`main()` renamed and tests the address-reservation cache's TTL, uniqueness,
ownership, and expiry semantics without opening CAN.

`test-core-logbook-disk` exercises `LogBook_file()`'s on-disk persistence
against the same renamed-`main()` link: read/write round-trip, new-format
checksum validation, legacy-format detection (read without migrating, cache
starts empty), and rejection of a file matching neither record size. It
includes a fixed regression for a use-after-free that existed in `"w"` mode:
the function captured the list head before sweeping expired entries, so a
head entry that expired between capture and sweep was read after being
freed.

`test-core-nox` checks the shared ten-second NOX lifetime boundary used by
both active-device reporting and logstat export.

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

For a complete source build followed by every regression suite:

```bash
make all
make test-core-all
```
