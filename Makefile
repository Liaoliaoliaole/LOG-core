CC=gcc
CFLAGS=-std=c99 -DUA_ARCHITECTURE_POSIX -Wall
LDLIBS=$(shell pkg-config --cflags --libs open62541) 
BUILD_dir=build
WORK_dir=work
SOURCE_dir=src

$(BUILD_dir)/morfeas_opc_ua: $(WORK_dir)/morfeas_opc_ua.o 
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	
$(WORK_dir)/morfeas_opc_ua.o: $(SOURCE_dir)/morfeas_opc_ua.c
	$(CC) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

tree: 
	mkdir -p $(BUILD_dir) $(WORK_dir)  

delete-the-tree:
	rm -f -r $(WORK_dir) $(BUILD_dir)

clean:
	rm -f $(WORK_dir)/* $(BUILD_dir)/*

.PHONY: all clean clean-tree 


