###############################################################################
#                     Makefile for Morfeas_core Project                       #
# Copyright (C) 12019-12021  Sam harry Tzavaras        	                      #
#                                                                             #
# This program is free software; you can redistribute it and/or               #
#  modify it under the terms of the GNU General Public License                #
#  as published by the Free Software Foundation; either version 3             #
#  of the License, or any later version.                                      #
#                                                                             #
# This program is distributed in the hope that it will be useful,             #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of             #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              #
#  GNU General Public License for more details.                               #
#                                                                             #
# You should have received a copy of the GNU General Public License           #
# along with this program; If not, see <https://www.gnu.org/licenses/>.       #
###############################################################################

GCC_opt= gcc -O3 -s#-g3
CFLAGS= -std=c99 -DUA_ARCHITECTURE_POSIX -Wall# -Werror
LDLIBS= -lm -lrt -li2c -lpthread $(shell pkg-config --cflags --libs $(LIBs))
BUILD_dir=build
WORK_dir=work
SRC_dir=src
CANif_DEP_HEADERS_dir = ./src/sdaq-worker/src/*.h
CANif_DEP_SRC_dir = ./src/sdaq-worker/src
D_opt = -D RELEASE_HASH='"$(shell git log -1 --format=%h)"' \
		-D RELEASE_DATE=$(shell git log -1 --format=%ct) \
        -D COMPILE_DATE=$(shell date +%s)
LIBs=open62541 \
	 libcjson \
	 ncurses \
	 libxml-2.0 \
	 libgtop-2.0 \
	 glib-2.0 \
	 libsocketcan \
	 nopoll \
	 libmodbus \
	 dbus-1 #\ libwebsockets \ libusb-1.0

HEADERS = $(SRC_dir)/IPC/*.h \
		  $(SRC_dir)/Morfeas_opc_ua/*.h \
		  $(SRC_dir)/Supplementary/*.h \
		  $(SRC_dir)/Morfeas_RPi_Hat/*.h \
		  $(SRC_dir)/Morfeas_MTI/*.h \
		  $(SRC_dir)/Morfeas_NOX/*.h

Morfeas_daemon_DEP =  $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_daemon.o \
					  $(WORK_dir)/Morfeas_XML.o \
					  $(WORK_dir)/MTI_func.o \
					  $(WORK_dir)/NOX_func.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_info.o

Morfeas_opc_ua_DEP =  $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_opc_ua.o \
					  $(WORK_dir)/Morfeas_opc_ua_config.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/MTI_func.o \
					  $(WORK_dir)/NOX_func.o \
					  $(WORK_dir)/Morfeas_DBus_method_caller.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_SDAQ_nodeset.o \
					  $(WORK_dir)/Morfeas_MDAQ_nodeset.o \
					  $(WORK_dir)/Morfeas_IOBOX_nodeset.o \
					  $(WORK_dir)/Morfeas_MTI_nodeset.o \
					  $(WORK_dir)/Morfeas_NOX_nodeset.o \
					  $(WORK_dir)/Morfeas_XML.o \
					  $(WORK_dir)/Morfeas_JSON.o \
					  $(WORK_dir)/Morfeas_info.o

Morfeas_SDAQ_if_DEP = $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_SDAQ_if.o \
					  $(WORK_dir)/Morfeas_JSON.o \
					  $(WORK_dir)/Morfeas_RPi_Hat.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/MTI_func.o \
					  $(WORK_dir)/NOX_func.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_Logger.o \
					  $(WORK_dir)/Morfeas_info.o \
					  $(CANif_DEP_HEADERS_dir)

Morfeas_MDAQ_if_DEP = $(WORK_dir)/Morfeas_run_check.o \
					  $(WORK_dir)/Morfeas_MDAQ_if.o \
					  $(WORK_dir)/Morfeas_JSON.o \
					  $(WORK_dir)/SDAQ_drv.o \
					  $(WORK_dir)/MTI_func.o \
					  $(WORK_dir)/NOX_func.o \
					  $(WORK_dir)/Morfeas_IPC.o \
					  $(WORK_dir)/Morfeas_Logger.o \
					  $(WORK_dir)/Morfeas_info.o

Morfeas_IOBOX_if_DEP = $(WORK_dir)/Morfeas_run_check.o \
					   $(WORK_dir)/Morfeas_IOBOX_if.o \
					   $(WORK_dir)/Morfeas_JSON.o \
					   $(WORK_dir)/SDAQ_drv.o \
					   $(WORK_dir)/MTI_func.o \
					   $(WORK_dir)/NOX_func.o \
					   $(WORK_dir)/Morfeas_IPC.o \
					   $(WORK_dir)/Morfeas_Logger.o \
					   $(WORK_dir)/Morfeas_info.o

Morfeas_MTI_if_DEP = $(WORK_dir)/MTI_func.o \
					 $(WORK_dir)/Morfeas_run_check.o \
					 $(WORK_dir)/Morfeas_MTI_if.o \
					 $(WORK_dir)/Morfeas_MTI_DBus.o \
					 $(WORK_dir)/Morfeas_JSON.o \
					 $(WORK_dir)/SDAQ_drv.o \
					 $(WORK_dir)/NOX_func.o \
					 $(WORK_dir)/Morfeas_IPC.o \
					 $(WORK_dir)/Morfeas_Logger.o \
					 $(WORK_dir)/Morfeas_info.o

Morfeas_NOX_if_DEP = $(WORK_dir)/Morfeas_run_check.o \
					 $(WORK_dir)/Morfeas_NOX_if.o \
					 $(WORK_dir)/Morfeas_NOX_ws.o \
					 $(WORK_dir)/Morfeas_NOX_DBus.o \
					 $(WORK_dir)/Morfeas_JSON.o \
					 $(WORK_dir)/Morfeas_RPi_Hat.o \
					 $(WORK_dir)/Morfeas_Logger.o \
					 $(WORK_dir)/SDAQ_drv.o \
					 $(WORK_dir)/NOX_func.o \
					 $(WORK_dir)/MTI_func.o \
					 $(WORK_dir)/Morfeas_IPC.o \
					 $(WORK_dir)/Morfeas_info.o

all: $(BUILD_dir)/Morfeas_daemon \
	 $(BUILD_dir)/Morfeas_opc_ua \
	 $(BUILD_dir)/Morfeas_SDAQ_if \
	 $(BUILD_dir)/Morfeas_IOBOX_if \
	 $(BUILD_dir)/Morfeas_MTI_if \
	 $(BUILD_dir)/Morfeas_NOX_if
# MDAQ: device type retired — Morfeas_MDAQ_if is NOT built.

#Compilation of Morfeas applications
$(BUILD_dir)/Morfeas_daemon: $(Morfeas_daemon_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_opc_ua: $(Morfeas_opc_ua_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_SDAQ_if: $(Morfeas_SDAQ_if_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

# MDAQ: device type retired.
# $(BUILD_dir)/Morfeas_MDAQ_if: $(Morfeas_MDAQ_if_DEP) $(HEADERS)
# 	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_IOBOX_if: $(Morfeas_IOBOX_if_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_MTI_if: $(Morfeas_MTI_if_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_dir)/Morfeas_NOX_if: $(Morfeas_NOX_if_DEP) $(HEADERS)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

#Compilation of Morfeas_IPC
$(WORK_dir)/Morfeas_IPC.o: $(SRC_dir)/IPC/Morfeas_IPC.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_opc_ua_config.o: $(SRC_dir)/Morfeas_opc_ua/Morfeas_opc_ua_config.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_opc_ua
$(WORK_dir)/Morfeas_opc_ua.o: $(SRC_dir)/Morfeas_opc_ua/Morfeas_opc_ua.c
	gcc $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_DBus_method_caller.o: $(SRC_dir)/Morfeas_opc_ua/Morfeas_DBus_method_caller.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_daemon
$(WORK_dir)/Morfeas_daemon.o: $(SRC_dir)/Morfeas_daemon.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies of the Morfeas_SDAQ_if
$(WORK_dir)/Morfeas_SDAQ_if.o: $(SRC_dir)/Morfeas_SDAQ/Morfeas_SDAQ_if.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/SDAQ_drv.o: $(CANif_DEP_SRC_dir)/SDAQ_drv.c
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

#Dependencies of the Morfeas_MTI_if
$(WORK_dir)/Morfeas_MTI_if.o: $(SRC_dir)/Morfeas_MTI/Morfeas_MTI_if.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_MTI_DBus.o: $(SRC_dir)/Morfeas_MTI/Morfeas_MTI_DBus.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/MTI_func.o: $(SRC_dir)/Morfeas_MTI/MTI_func.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_MTI_nodeset.o: $(SRC_dir)/Morfeas_MTI/Morfeas_MTI_nodeset.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Dependencies for the Morfeas_NOX_if
$(WORK_dir)/Morfeas_NOX_if.o: $(SRC_dir)/Morfeas_NOX/Morfeas_NOX_if.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_NOX_DBus.o: $(SRC_dir)/Morfeas_NOX/Morfeas_NOX_DBus.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/NOX_func.o: $(SRC_dir)/Morfeas_NOX/NOX_func.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_NOX_nodeset.o: $(SRC_dir)/Morfeas_NOX/Morfeas_NOX_nodeset.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_NOX_ws.o: $(SRC_dir)/Morfeas_NOX/Morfeas_NOX_ws.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

#Supplementary dependencies
$(WORK_dir)/Morfeas_RPi_Hat.o: $(SRC_dir)/Morfeas_RPi_Hat/Morfeas_RPi_Hat.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_Logger.o: $(SRC_dir)/Supplementary/Morfeas_Logger.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_JSON.o: $(SRC_dir)/Supplementary/Morfeas_JSON.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_XML.o: $(SRC_dir)/Supplementary/Morfeas_XML.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_run_check.o: $(SRC_dir)/Supplementary/Morfeas_run_check.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/Morfeas_info.o: $(SRC_dir)/Supplementary/Morfeas_info.c
	$(GCC_opt) $(D_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

tree:
	mkdir -p $(BUILD_dir) $(WORK_dir)

delete-tree:
	rm -f -r $(WORK_dir) $(BUILD_dir)

clean:
	rm -f $(WORK_dir)/* $(BUILD_dir)/*

install:
	@echo "\nInstallation of executable Binaries..."
	@install $(BUILD_dir)/* -v -t /usr/local/bin/
	@echo "Instalation of D-Bus configuration"
	cp -r -n ./dbus/* /etc/dbus-1/system.d/
	@echo "\nInstallation of Systemd service for Morfeas_daemon..."
	cp -r -n ./systemd/* /etc/systemd/system/
	@echo "\nIf you want to run the Morfeas-System at boot, execute:"
	@echo  "# systemctl enable Morfeas_system.service"

uninstall:
	@echo  "\nStop Morfeas_system service..."
	@systemctl stop Morfeas_system.service
	@echo  "Remove related binaries and Systemd unit files..."
	@rm /usr/local/bin/Morfeas_*
	@rm -r /etc/systemd/system/Morfeas_system*
	@echo  "\nRemove D-Bus configuration..."
	@rm /etc/dbus-1/system.d/Morfeas_system.conf
	@echo  "\nRemove LogBooks..."
	@rm -r /var/tmp/Morfeas_*

###############################################################################
# Project's own tests (not the vendored cJSON/open62541 self-tests).         #
###############################################################################
TESTS_dir = tests

Morfeas_XML_test_DEP = $(WORK_dir)/Morfeas_run_check.o \
						$(WORK_dir)/Morfeas_XML.o \
						$(WORK_dir)/MTI_func.o \
						$(WORK_dir)/NOX_func.o \
						$(WORK_dir)/Morfeas_IPC.o

$(WORK_dir)/opcua_config_parser_test.o: $(TESTS_dir)/opcua_config_parser_test.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(BUILD_dir)/opcua_config_parser_test: $(WORK_dir)/opcua_config_parser_test.o $(Morfeas_XML_test_DEP)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

test-core-a1: tree $(BUILD_dir)/opcua_config_parser_test
	./$(BUILD_dir)/opcua_config_parser_test

# Shared semantic daemon-config corpus. The same JSON fixtures are consumed
# by this production C validator and Web's logConfigValidationTest.php.
$(WORK_dir)/daemon_config_fixture_test.o: $(TESTS_dir)/daemon_config_fixture_test.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(BUILD_dir)/daemon_config_fixture_test: $(WORK_dir)/daemon_config_fixture_test.o $(Morfeas_XML_test_DEP)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

test-core-d: tree $(BUILD_dir)/daemon_config_fixture_test
	./$(BUILD_dir)/daemon_config_fixture_test $(TESTS_dir)/fixtures/daemon_config_validation_cases.json

# Address-cache logic is exercised through the production Morfeas_SDAQ_if.c
# with only its main() symbol renamed for the test link.
$(WORK_dir)/Morfeas_SDAQ_if_cache_testmain.o: $(SRC_dir)/Morfeas_SDAQ/Morfeas_SDAQ_if.c
	$(GCC_opt) $(CFLAGS) -Dmain=Morfeas_SDAQ_if_unused_main $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/sdaq_address_cache_test.o: $(TESTS_dir)/sdaq_address_cache_test.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

Morfeas_SDAQ_if_cache_test_DEP = $(WORK_dir)/sdaq_address_cache_test.o \
						  $(WORK_dir)/Morfeas_SDAQ_if_cache_testmain.o \
						  $(WORK_dir)/Morfeas_run_check.o \
						  $(WORK_dir)/Morfeas_JSON.o \
						  $(WORK_dir)/Morfeas_RPi_Hat.o \
						  $(WORK_dir)/SDAQ_drv.o \
						  $(WORK_dir)/MTI_func.o \
						  $(WORK_dir)/NOX_func.o \
						  $(WORK_dir)/Morfeas_IPC.o \
						  $(WORK_dir)/Morfeas_Logger.o \
						  $(WORK_dir)/Morfeas_info.o

$(BUILD_dir)/sdaq_address_cache_test: $(Morfeas_SDAQ_if_cache_test_DEP)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

test-core-sdaq-cache: tree $(BUILD_dir)/sdaq_address_cache_test
	./$(BUILD_dir)/sdaq_address_cache_test

$(WORK_dir)/nox_lifetime_test.o: $(TESTS_dir)/nox_lifetime_test.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

$(BUILD_dir)/nox_lifetime_test: $(WORK_dir)/nox_lifetime_test.o
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

test-core-nox: tree $(BUILD_dir)/nox_lifetime_test
	./$(BUILD_dir)/nox_lifetime_test

# One command for every maintained project regression suite. Keep individual
# targets for focused development, but CI/review must not rely on remembering
# which of them exist.
test-core-all: test-core-a1 test-core-d test-core-o test-core-sdaq-cache test-core-nox

# Integration test for the SDAQ Unit browse gate. It links an embedded
# UA_Server and compiles Morfeas_opc_ua.c with its main renamed because the
# test supplies its own entry point. The production build remains unchanged.
$(WORK_dir)/Morfeas_opc_ua_testmain.o: $(SRC_dir)/Morfeas_opc_ua/Morfeas_opc_ua.c
	gcc $(CFLAGS) -Dmain=Morfeas_opc_ua_unused_main $^ -c -o $@ $(LDLIBS)

$(WORK_dir)/sdaq_offline_browse_gate_test.o: $(TESTS_dir)/sdaq_offline_browse_gate_test.c
	$(GCC_opt) $(CFLAGS) $^ -c -o $@ $(LDLIBS)

Morfeas_opc_ua_test_DEP = $(WORK_dir)/sdaq_offline_browse_gate_test.o \
						  $(WORK_dir)/Morfeas_run_check.o \
						  $(WORK_dir)/Morfeas_opc_ua_testmain.o \
						  $(WORK_dir)/Morfeas_opc_ua_config.o \
						  $(WORK_dir)/SDAQ_drv.o \
						  $(WORK_dir)/MTI_func.o \
						  $(WORK_dir)/NOX_func.o \
						  $(WORK_dir)/Morfeas_DBus_method_caller.o \
						  $(WORK_dir)/Morfeas_IPC.o \
						  $(WORK_dir)/Morfeas_SDAQ_nodeset.o \
						  $(WORK_dir)/Morfeas_MDAQ_nodeset.o \
						  $(WORK_dir)/Morfeas_IOBOX_nodeset.o \
						  $(WORK_dir)/Morfeas_MTI_nodeset.o \
						  $(WORK_dir)/Morfeas_NOX_nodeset.o \
						  $(WORK_dir)/Morfeas_XML.o \
						  $(WORK_dir)/Morfeas_JSON.o \
						  $(WORK_dir)/Morfeas_info.o

$(BUILD_dir)/sdaq_offline_browse_gate_test: $(Morfeas_opc_ua_test_DEP)
	$(GCC_opt) $(CFLAGS) $^ -o $@ $(LDLIBS)

test-core-o: tree $(BUILD_dir)/sdaq_offline_browse_gate_test
	./$(BUILD_dir)/sdaq_offline_browse_gate_test

.PHONY: all clean delete-tree test-core-a1 test-core-d test-core-o test-core-sdaq-cache test-core-nox test-core-all
