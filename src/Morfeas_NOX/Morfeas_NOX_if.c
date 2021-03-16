/*
File: Morfeas_NOX_if.c, Implementation of Morfeas NOX (CANBus) handler, Part of Morfeas_project.
Copyright (C) 12021-12022  Sam harry Tzavaras

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#define VERSION "0.1" /*Release Version of Morfeas_NOX_if software*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <libsocketcan.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

//Include Functions implementation header
#include "../Supplementary/Morfeas_run_check.h"
#include "../Supplementary/Morfeas_JSON.h"
#include "../IPC/Morfeas_IPC.h" //<-#include -> "Morfeas_Types.h"
#include "../Supplementary/Morfeas_Logger.h"
#include "../Morfeas_RPi_Hat/Morfeas_RPi_Hat.h"

/* Local function (declaration)
 * Return value: EXIT_FAILURE(1) of failure or EXIT_SUCCESS(0) on success. Except of other notice
 */
void print_usage(char *prog_name);//print the usage manual

int main(int argc, char *argv[])
{
	//Operational variables
	int c;
	unsigned char i2c_bus_num = I2C_BUS_NUM;
	unsigned long msg_cnt=0;
	//Variables for CANBus Port Electric
	struct tm Morfeas_RPi_Hat_last_cal = {0};
	struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config port_meas_config = {0};
	struct Morfeas_RPi_Hat_Port_meas port_meas = {0};
	//Variables for IPC
	IPC_message IPC_msg = {0};
	//Variables for Socket CAN and SDAQ_decoders
	int RX_bytes;
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr = {0};
	struct can_frame frame_rx;
	struct can_filter RX_filter;
	//Timers related Variables
	struct itimerval timer;
	
	if(argc == 1)
	{
		print_usage(argv[0]);
		exit(1);
	}
	opterr = 1;
	while((c = getopt (argc, argv, "hvb:")) != -1)
	{
		switch(c)
		{
			case 'h'://help
				print_usage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'v'://Version
				printf(VERSION"\n");
				exit(EXIT_SUCCESS);
			case 'b':
				i2c_bus_num=atoi(optarg);
				break;
			case '?':
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	
	return EXIT_SUCCESS;
}

void print_usage(char *prog_name)
{
	const char preamp[] = {
	"Program: Morfeas_NOX_if  Copyright (C) 12019-12021  Sam Harry Tzavaras\n"
    "This program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
    "This is free software, and you are welcome to redistribute it\n"
    "under certain conditions; for details see LICENSE.\n"
	};
	const char exp[] = {
	"\tCAN-IF: The name of the CAN-Bus adapter\n"
	"\tOptions:\n"
	"\t         -h : Print Help\n"
	"\t         -v : Print Version\n"
	"\t         -b : I2C Bus number for Morfeas_RPI_hat (Default: 1)\n"
	};
	printf("%s\nUsage: %s [options] CAN-IF [/path/to/logstat/directory] \n\n%s\n", preamp, prog_name, exp);
	return;
}