GCC_opt=gcc -O3
CFLAGS= -std=c99 -DUA_ARCHITECTURE_POSIX -Wall -g3 #-Werror
LDLIBS= -lm -lrt -li2c -lpthread $(shell pkg-config --cflags --libs open62541 libcjson ncurses libxml-2.0 libgtop-2.0 glib-2.0 libmodbus libusb)
BUILD_dir=build
WORK_dir=work
SRC_dir=src
CANif_DEP_HEADERS_dir = ./src/sdaq-worker/src/*.h
CANif_DEP_SRC_dir = ./src/sdaq-worker/src

HEADERS = $(SRC_dir)/IPC/*.h \
		  $(SRC_dir)/Morfeas_opc_ua/*.h \
		  $(SRC_dir)/Supplementary/*.h \
		  $(SRC_dir)/Morfeas_RPi_Hat/*.h

Morfeas_daemon_DEP =  $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_daemon.o \
					  $(WORK_dir)/Morfeas_XML.o \
					  $(WORK_dir)/Morfeas_IPC.o

Morfeas_opc_ua_DEP =  $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_opc_ua.o \
					  $(WORK_dir)/Morfeas_opc_ua_config.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_SDAQ_nodeset.o \
					  $(WORK_dir)/Morfeas_MDAQ_nodeset.o \
					  $(WORK_dir)/Morfeas_IOBOX_nodeset.o \
					  $(WORK_dir)/Morfeas_XML.o

Morfeas_SDAQ_if_DEP = $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_SDAQ_if.o \
					  $(WORK_dir)/Morfeas_JSON.o \
					  $(WORK_dir)/Morfeas_RPi_Hat.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_Logger.o \
					  $(CANif_DEP_HEADERS_dir)

Morfeas_MDAQ_if_DEP = $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_MDAQ_if.o \
					  $(WORK_dir)/Morfeas_JSON.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_Logger.o 

Morfeas_IOBOX_if_DEP = $(WORK_dir)/Morfeas_run_check.o \
					   $(WORK_dir)/Morfeas_IOBOX_if.o \
					   $(WORK_dir)/Morfeas_JSON.o \
					   $(WORK_dir)/SDAQ_drv.o \
					   $(WORK_dir)/Morfeas_IPC.o \
					   $(WORK_dir)/Morfeas_Logger.o 

all: $(BUILD_dir)/Morfeas_daemon \
	 $(BUILD_dir)/Morfeas_opc_ua \
	 $(BUILD_dir)/Morfeas_SDAQ_if \
	 $(BUILD_dir)/Morfeas_MDAQ_if \
	 $(BUILD_dir)/Morfeas_IOBOX_if

#Compilation of Morfeas applications
$(BUILD_dir)/Morfeas_daemon: $(Morfeas_daemon_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_opc_ua: $(Morfeas_opc_ua_DEP) $(HEADERS)
	gcc $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_SDAQ_if: $(Morfeas_SDAQ_if_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_MDAQ_if: $(Morfeas_MDAQ_if_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_IOBOX_if: $(Morfeas_IOBOX_if_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

#Compilation of Morfeas_IPC
$(WORK_dir)/Morfeas_IPC.o: $(SRC_dir)/IPC/Morfeas_IPC.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_opc_ua_config.o: $(SRC_dir)/Morfeas_opc_ua/Morfeas_opc_ua_config.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_opc_ua
$(WORK_dir)/Morfeas_opc_ua.o: $(SRC_dir)/Morfeas_opc_ua/Morfeas_opc_ua.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_daemon
$(WORK_dir)/Morfeas_daemon.o: $(SRC_dir)/Morfeas_daemon.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_SDAQ_if
$(WORK_dir)/Morfeas_SDAQ_if.o: $(SRC_dir)/Morfeas_SDAQ/Morfeas_SDAQ_if.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/SDAQ_drv.o: $(CANif_DEP_SRC_dir)/SDAQ_drv.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_RPi_Hat.o: $(SRC_dir)/Morfeas_RPi_Hat/Morfeas_RPi_Hat.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_SDAQ_nodeset.o: $(SRC_dir)/Morfeas_SDAQ/Morfeas_SDAQ_nodeset.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_MDAQ_if
$(WORK_dir)/Morfeas_MDAQ_if.o: $(SRC_dir)/Morfeas_MDAQ/Morfeas_MDAQ_if.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_MDAQ_nodeset.o: $(SRC_dir)/Morfeas_MDAQ/Morfeas_MDAQ_nodeset.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_IOBOX_if
$(WORK_dir)/Morfeas_IOBOX_if.o: $(SRC_dir)/Morfeas_IOBOX/Morfeas_IOBOX_if.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_IOBOX_nodeset.o: $(SRC_dir)/Morfeas_IOBOX/Morfeas_IOBOX_nodeset.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Supplementary dependencies
$(WORK_dir)/Morfeas_Logger.o: $(SRC_dir)/Supplementary/Morfeas_Logger.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_JSON.o: $(SRC_dir)/Supplementary/Morfeas_JSON.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_XML.o: $(SRC_dir)/Supplementary/Morfeas_XML.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_run_check.o: $(SRC_dir)/Supplementary/Morfeas_run_check.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

tree:
	mkdir -p $(BUILD_dir) $(WORK_dir)

delete-the-tree:
	rm -f -r $(WORK_dir) $(BUILD_dir)

clean:
	rm -f $(WORK_dir)/* $(BUILD_dir)/*

install:
	@echo "Installation of executable Binaries"
	@install $(BUILD_dir)/Morfeas_daemon -v -t /usr/local/bin/
	@install $(BUILD_dir)/Morfeas_opc_ua -v -t /usr/local/bin/
	@install $(BUILD_dir)/Morfeas_SDAQ_if -v -t /usr/local/bin/
	@install $(BUILD_dir)/Morfeas_MDAQ_if -v -t /usr/local/bin/
	@install $(BUILD_dir)/Morfeas_IOBOX_if -v -t /usr/local/bin/
	@echo "\nInstallation of Systemd service for Morfeas_daemon"
	cp -r -n ./systemd/* /etc/systemd/system/
	@echo "\n If you want to run the Morfeas-System at boot, run:"
	@echo  "# systemctl enable Morfeas_system.service"

uninstall:
	@echo  "Stop Morfeas_system service"
	systemctl stop Morfeas_system.service
	@echo  "Remove related binaries and systemd file"
	rm /usr/local/bin/Morfeas_*
	rm -r /etc/systemd/system/Morfeas_system*

.PHONY: all clean clean-tree


