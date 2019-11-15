CC=gcc
CFLAGS= -std=c99 -DUA_ARCHITECTURE_POSIX -Wall
LDLIBS= -lrt -lpthread $(shell pkg-config --cflags --libs open62541 libxml-2.0 libgtop-2.0 glib-2.0)
BUILD_dir=build
WORK_dir=work
SRC_dir=src
DEP=$(WORK_dir)/Discover_and_autoconfig.o $(WORK_dir)/Measure.o $(WORK_dir)/Logging.o $(WORK_dir)/info.o $(WORK_dir)/SDAQ_drv.o $(WORK_dir)/SDAQ_xml.o

all: $(BUILD_dir)/morfeas_opc_ua $(BUILD_dir)/morfeas_SDAQ_if

$(BUILD_dir)/morfeas_opc_ua:  $(SRC_dir)/*.h $(WORK_dir)/morfeas_opc_ua.o 
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	
$(BUILD_dir)/morfeas_SDAQ_if: $(SRC_dir)/*.h $(WORK_dir)/morfeas_SDAQ_if.o 
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	
$(WORK_dir)/morfeas_opc_ua.o: $(SRC_dir)/morfeas_opc_ua.c
	$(CC) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/morfeas_SDAQ_if.o: $(SRC_dir)/morfeas_SDAQ_if.c
	$(CC) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

tree: 
	mkdir -p $(BUILD_dir) $(WORK_dir)  

delete-the-tree:
	rm -f -r $(WORK_dir) $(BUILD_dir)

clean:
	rm -f $(WORK_dir)/* $(BUILD_dir)/*

.PHONY: all clean clean-tree 


