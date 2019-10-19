CC=gcc
CFLAGS=-std=c99 -DUA_ARCHITECTURE_POSIX -Wall
LDLIBS=$(shell pkg-config --cflags --libs open62541 libxml-2.0) 

all: opc_ua_server_test xml_parsing_test shm_server shm_client

opc_ua_server_test: opc_ua_server_test.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS) 

xml_parsing_test: xml_parsing_test.c 
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	
shm_server: shm_server.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

shm_client: shm_client.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	
clean:
	rm -f *.o opc_ua_server_test xml_parsing_test shm_server shm_client

.PHONY: all clean clean-tree 


