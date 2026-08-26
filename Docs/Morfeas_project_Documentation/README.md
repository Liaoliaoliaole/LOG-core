# Documentation of Morfeas-Core

This directory contains the original LaTeX architectural documentation of the Morfeas Project. It is retained for historical and component-reference use; it does not define the current deployment, SDAQ calibration, restore, or LOG WEB operator contract. For current instructions use the repository-root `README.md`, `RE-INSTALL.md`, `tests/README.md`, and the LOG WEB Help manual.

### Requirements
For compilation of this project the following dependencies required.
* [GNU Make](https://www.gnu.org/software/make/) - GNU make utility
* [TeX Live](https://www.tug.org/texlive/) - Libre(free) software distribution for the TeX typesetting system.
* [LaTex-mk](http://latex-mk.sourceforge.net/) - Complete system for simplifying the management of small to large sized LaTeX documents.

## Compilation of the documentation
```
$ make tree
$ make -j$(nproc)
```
The compiled documentation located under the **./build-doc** directory.

## Authors
* **Sam Harry Tzavaras** - *Initial work*

## License
The source code of the Documentation of Morfeas project is licensed under FDLv1.3 or later - see the [License](../../fdl-1.3.md) file for details.
