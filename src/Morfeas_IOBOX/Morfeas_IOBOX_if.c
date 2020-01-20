/*
File: Morfeas_IOBOX_if.c, Implementation of Morfeas IOBOX (MODBus) handler, Part of Morfeas_project.
Copyright (C) 12019-12020  Sam harry Tzavaras

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
#define VERSION "0.5" /*Release Version of Morfeas_IOBOX_if*/

#define IOBOX_imp_reg 125
#define IOBOX_slave_address 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <modbus.h>

#include "../Supplementary/Morfeas_run_check.h"
#include "../Supplementary/Morfeas_Logger.h"

//Global variables
static volatile unsigned char handler_run = 1;

//Print the Usage manual
void print_usage(char *prog_name);


static void stopHandler(int signum)
{
	if(signum == SIGPIPE)
		fprintf(stderr,"IPC: Force Termination!!!\n");
	handler_run = 0;
}

int main(int argc, char *argv[])
{
	//MODBus related variables
	modbus_t *ctx;
	int rc, i, j, offset;
	//Apps variables
	char *IOBOX_IPv4_addr, *dev_name, *path_to_logstat_dir;
	unsigned short *IOBOX_regs;
	//Check for call without arguments
	if(argc == 1)
	{
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	//Get options
	int c;
	while ((c = getopt (argc, argv, "hV")) != -1)
	{
		switch (c)
		{
			case 'h'://help
				print_usage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'V'://Version
				printf(VERSION"\n");
				exit(EXIT_SUCCESS);
			case '?':
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	//Get arguments
	IOBOX_IPv4_addr = argv[optind];
	dev_name = argv[++optind];
	path_to_logstat_dir = argv[++optind];
	//Check arguments
	if(!IOBOX_IPv4_addr || !dev_name)
	{
		fprintf(stderr, "Mandatory Argument missing!!!\n");
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if(!is_valid_IPv4(IOBOX_IPv4_addr))
	{
		fprintf(stderr, "Argument of Device name missing!!!\n");
		exit(EXIT_FAILURE);
	}
	//Check if other instance of this program already runs with same IOBOX_IPv4_addr
	if(check_already_run_with_same_arg(argv[0], IOBOX_IPv4_addr))
	{
		fprintf(stderr, "%s for IPv4:%s Already Running!!!\n", argv[0], IOBOX_IPv4_addr);
		exit(EXIT_SUCCESS);
	}
	// Allocate and initialize the memory to store the IOBOX's registers
	if(!(IOBOX_regs = calloc(IOBOX_imp_reg+5, sizeof(unsigned short))))
	{
		fprintf(stderr, "Memory Error!!!\n");
		exit(EXIT_FAILURE);
	}
	//Install stopHandler as the signal handler for SIGINT, SIGTERM and SIGPIPE signals.
	signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    signal(SIGPIPE, stopHandler);

	//Print welcome message
	Logger("---- Morfeas_IOBOX_if Started ----\n",argv[0]);
	if(!path_to_logstat_dir)
		Logger("Argument for path to logstat directory Missing, %s will run in Compatible mode !!!\n",argv[0]);

	//Make MODBus socket for connection
	ctx = modbus_new_tcp(IOBOX_IPv4_addr, MODBUS_TCP_DEFAULT_PORT);
	//Set Slave address
	if(modbus_set_slave(ctx, IOBOX_slave_address))
	{
		fprintf(stderr, "Can't set slave address !!!\n");
		modbus_free(ctx);
		free(IOBOX_regs);
		return EXIT_FAILURE;
	}
	//Attempt connection to IOBOX
	while(modbus_connect(ctx) && handler_run)
	{
		Logger("Connection Error @ %s: %s\n", IOBOX_IPv4_addr, modbus_strerror(errno));
		sleep(1);
	}
	if(!handler_run)
	{
		modbus_free(ctx);
		free(IOBOX_regs);
		return EXIT_FAILURE;
	}
	//main application loop
	while(handler_run)
	{
		rc = modbus_read_registers(ctx, 0, IOBOX_imp_reg, IOBOX_regs);
		if (rc <= 0)
		{
			Logger("Error (%d) on MODBus Register read: %s\n",errno, modbus_strerror(errno));
			//Attempt to reconnection
			while(modbus_connect(ctx) && handler_run)
				sleep(1);
		}
		else
		{
			printf("----  Wireless Inductive Power Supply ----\n");
			j=1;
			printf("Uin=%.3f\n\n", IOBOX_regs[0]*0.01);
			offset = 1;
			while(offset<13)
			{
				printf("%uUout=%.3fV\n", j, IOBOX_regs[offset++]/100.0);
				printf("%uIout=%.3fA\n", j, IOBOX_regs[offset++]/100.0);
				printf("%uIout_filtered=%.3fA\n\n", j, IOBOX_regs[offset++]/100.0);
				j++;
			}
			/*
			printf("----  RX 1 ----\n");
			offset = 25;
			for(i=0;i<16;i++)
			{
				printf("CH%d -> %.3f°C\n", i+1, IOBOX_regs[i+offset]/16.0);
			}
			printf("Packet index = %hu\n", IOBOX_regs[21+offset]);
			printf("Status = %hu\n", IOBOX_regs[22+offset]);
			printf("Succsess Ration = %hu\n", IOBOX_regs[23+offset]);

			printf("----  RX 2 ----\n");
			offset = 50;
			for(i=0;i<16;i++)
			{
				printf("CH%d -> %.3f°C\n", i+1, IOBOX_regs[i+offset]/16.0);
			}
			printf("Packet index = %hu\n", IOBOX_regs[21+offset]);
			printf("Status = %hu\n", IOBOX_regs[22+offset]);
			printf("Succsess Ration = %hu\n", IOBOX_regs[23+offset]);

			printf("----  RX 3 ----\n");
			offset = 75;
			for(i=0;i<16;i++)
			{
				printf("CH%d -> %.3f°C\n", i+1, IOBOX_regs[i+offset]/16.0);
			}
			printf("Packet index = %hu\n", IOBOX_regs[21+offset]);
			printf("Status = %hu\n", IOBOX_regs[22+offset]);
			printf("Succsess Ration = %hu\n", IOBOX_regs[23+offset]);

			printf("----  RX 4 ----\n");
			offset = 100;
			for(i=0;i<16;i++)
			{
				printf("CH%d -> %.3f°C\n", i+1, IOBOX_regs[i+offset]/16.0);
			}
			printf("Packet index = %hu\n", IOBOX_regs[21+offset]);
			printf("Status = %hu\n", IOBOX_regs[22+offset]);
			printf("Succsess Ration = %hu\n", IOBOX_regs[23+offset]);
			*/
		}
		usleep(100000);
	}
	//Close MODBus connection and De-allocate memory
	modbus_close(ctx);
	modbus_free(ctx);
	free(IOBOX_regs);
	return EXIT_SUCCESS;
}

//print the Usage manual
void print_usage(char *prog_name)
{
	const char preamp[] = {
	"\tProgram: Morfeas_IOBOX_if  Copyright (C) 12019-12020  Sam Harry Tzavaras\n"
    "\tThis program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
    "\tThis is free software, and you are welcome to redistribute it\n"
    "\tunder certain conditions; for details see LICENSE.\n"
	};
	const char manual[] = {
		"\tDev_name: A string that related to the configuration of the IOBOX\n\n"
		"\t    IPv4: The IPv4 address of IOBOX\n\n"
		"Options:\n"
		"           -h : Print help.\n"
		"           -V : Version.\n"
	};
	printf("%s\nUsage: %s IPv4 Dev_name [/path/to/logstat/directory] [Options]\n\n%s",preamp, prog_name, manual);
	return;
}









