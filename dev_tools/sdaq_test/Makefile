CC=gcc
CFLAGS= -Wall
LDLIBS=-lrt #$(shell pkg-config --cflags --libs open62541 libxml-2.0 libgtop-2.0) 
BUILD_dir=build
WORK_dir=work
DEP=$(WORK_dir)/sdaq_drv.o sdaq_drv.h SDAQ_tester.c

$(BUILD_dir)/SDAQ_tester: $(DEP)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	
$(WORK_dir)/sdaq_drv.o: sdaq_drv.c 
	$(CC) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

tree: 
	mkdir -p $(BUILD_dir) $(WORK_dir)  

delete-the-tree:
	rm -f -r $(WORK_dir) $(BUILD_dir)

clean:
	rm -f $(WORK_dir)/* $(BUILD_dir)/*

.PHONY: all clean clean-tree 


