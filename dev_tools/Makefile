CC=gcc
CFLAGS=-std=c99 -DUA_ARCHITECTURE_POSIX -Wall
LDLIBS=$(shell pkg-config --cflags --libs open62541 libxml-2.0 libgtop-2.0) -lrt

comp_progs = opc_ua_server_test xml_parsing_test libgtop_test timer_test tree1 tree2

all: $(comp_progs)

opc_ua_server_test: opc_ua_server_test.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

xml_parsing_test: xml_parsing_test.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

libgtop_test: libgtop_test.c
	$(CC) -Wall $^ -o $@ $(LDLIBS)

timer_test: timer_test.c
	$(CC) $^ -o $@ -lrt -Wall

tree1: tree1.c
	$(CC) $^ -o $@ -lrt -Wall $(LDLIBS)

tree2: tree2.c
	$(CC) $^ -o $@ -lrt -Wall $(LDLIBS)

clean:
	rm -f *.o $(comp_progs)

.PHONY: all clean clean-tree


