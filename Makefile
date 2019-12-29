CFLAGS= -std=c99 -DUA_ARCHITECTURE_POSIX -Wall -g3 #-Werror
LDLIBS= -lm -lrt -lpthread $(shell pkg-config --cflags --libs open62541 libcjson ncurses libxml-2.0 libgtop-2.0 glib-2.0 libmodbus libusb)
BUILD_dir=build
WORK_dir=work
SRC_dir=src
CANif_DEP_dir = ./src/sdaq-worker/work
CANif_DEP_HEADERS_dir = ./src/sdaq-worker/src/*.h
CANif_DEP_SRC_dir = ./src/sdaq-worker/src

Morfeas_daemon_DEP =  $(WORK_dir)/Morfeas_daemon.o \
					  $(WORK_dir)/Morfeas_XML.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(SRC_dir)/*.h

Morfeas_opc_ua_DEP =  $(WORK_dir)/Morfeas_opc_ua.o \
					  $(WORK_dir)/Morfeas_opc_ua_config.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_SDAQ_nodeset.o \
					  $(WORK_dir)/Morfeas_XML.o \
					  $(SRC_dir)/*.h

Morfeas_SDAQ_if_DEP = $(CANif_DEP_HEADERS_dir) \
					  $(WORK_dir)/Morfeas_SDAQ_if.o \
					  $(WORK_dir)/Morfeas_JSON.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_Logger.o \
					  $(SRC_dir)/*.h

all: $(BUILD_dir)/Morfeas_daemon \
	 $(BUILD_dir)/Morfeas_opc_ua \
	 $(BUILD_dir)/Morfeas_SDAQ_if

#Compilation of Morfeas applications
$(BUILD_dir)/Morfeas_opc_ua: $(Morfeas_opc_ua_DEP)
	gcc $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_SDAQ_if: $(Morfeas_SDAQ_if_DEP)
	gcc $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_daemon: $(Morfeas_daemon_DEP)
	gcc $(CFLAGS) $^ -o $@ $(LDLIBS)

#Compilation of Morfeas_IPC
$(WORK_dir)/Morfeas_IPC.o: $(SRC_dir)/Morfeas_IPC.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_opc_ua_config.o: $(SRC_dir)/Morfeas_opc_ua_config.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_daemon
$(WORK_dir)/Morfeas_daemon.o: $(SRC_dir)/Morfeas_daemon.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_opc_ua
$(WORK_dir)/Morfeas_opc_ua.o: $(SRC_dir)/Morfeas_opc_ua.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_SDAQ_if
$(WORK_dir)/Morfeas_SDAQ_if.o: $(SRC_dir)/Morfeas_SDAQ_if.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/SDAQ_drv.o: $(CANif_DEP_SRC_dir)/SDAQ_drv.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_SDAQ_nodeset.o: $(SRC_dir)/Morfeas_SDAQ_nodeset.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Supplementary dependencies
$(WORK_dir)/Morfeas_Logger.o: $(SRC_dir)/Morfeas_Logger.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_JSON.o: $(SRC_dir)/Morfeas_JSON.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_XML.o: $(SRC_dir)/Morfeas_XML.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

tree:
	mkdir -p $(BUILD_dir) $(WORK_dir)

delete-the-tree:
	rm -f -r $(WORK_dir) $(BUILD_dir)

clean:
	rm -f $(WORK_dir)/* $(BUILD_dir)/*

.PHONY: all clean clean-tree


