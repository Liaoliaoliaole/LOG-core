# Rebuild and Reinstall LOG Core

Use the repository scripts. They encode the supported dependency order and keep
the `src/sdaq-worker` submodule at the exact commit recorded by LOG-core.

## Update source

```bash
cd LOG-core
git pull --ff-only
git submodule sync --recursive
git submodule update --init --recursive
```

Do **not** run `git submodule foreach git pull origin master`. A submodule must
remain pinned to the parent commit; independently pulling its branch can build a
combination that was never reviewed or tested together.

## Normal code update

Use this for LOG-core or SDAQ-worker source changes when the third-party
dependency pins are unchanged:

```bash
cd LOG-core
./build_core_only.sh
```

`build_core_only.sh` synchronizes `src/sdaq-worker`, builds and installs
`SDAQ_worker`, then builds and installs LOG core and restarts
`Morfeas_system.service`. This is also the path used by System Update.

## Full dependency rebuild

Use this only for first installation or when the pinned versions of cJSON,
noPoll, open62541, or another third-party dependency change:

```bash
cd LOG-core
./build_core_full.sh
```

## Verify the installed result

```bash
systemctl is-active Morfeas_system.service
SDAQ_worker -V
ps -ef | egrep 'Morfeas_daemon|Morfeas_opc_ua|Morfeas_SDAQ_if' | grep -v egrep
```

For source-level regression checks before deployment, run:

```bash
make test-core-all
cd src/sdaq-worker && make test
```
