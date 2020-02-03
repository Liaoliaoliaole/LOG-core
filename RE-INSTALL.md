# Re-Compilation of the source

### Get the Latest Source and Submodules Releases
```
$ cd Morfeas_core
$ # Get Latest Source
$ git pull
$ # Get Latest Source for the submodules
$ git submodules foreach git pull origin master
```
### Re-Compilation and installation of the submodules

#### cJSON
```
$ cd src/cJSON
$ make clean
$ make -j$(nproc)
$ sudo make install
```
#### open62541
```
$ cd src/open62541
$ make clean
$ make -j$(nproc)
$ sudo make install
```
#### SDAQ_worker
```
$ cd src/sdaq_worker
$ make clean
$ make -j$(nproc)
$ sudo make install
```
### Re-Compilation and installation of the Morfeas-core Project
```
$ make clean
$ make -j$(nproc)
$ sudo make install
```