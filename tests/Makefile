CC=gcc
CFLAGS=-std=c99 -DUA_ARCHITECTURE_POSIX -Wall
LDLIBS=$(shell pkg-config --cflags --libs open62541 libxml-2.0 libgtop-2.0) -lrt

comp_progs = opc_ua_server_test xml_parsing_test shm_server shm_client libgtop_test timer_test

all: $(comp_progs)

opc_ua_server_test: opc_ua_server_test.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) 

xml_parsing_test: xml_parsing_test.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	
shm_server: shm_server.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

shm_client: shm_client.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

libgtop_test: libgtop_test.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

timer_test: timer_test.c
	$(CC) $^ -o $@ -lrt -Wall 
		
clean: 
	rm -f *.o $(comp_progs)

.PHONY: all clean clean-tree 


