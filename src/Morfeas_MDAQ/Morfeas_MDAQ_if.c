/*
File: Morfeas_MDAQ_if.c, Implementation of Morfeas MDAQ (MODBus) handler, Part of Morfeas_project.
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
#define VERSION "0.1" /*Release Version of Morfeas_MDAQ_if*/

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
#include <arpa/inet.h>

#include <modbus.h>

#include "../IPC/Morfeas_IPC.h" //<- #include "Morfeas_Types.h"
#include "../Supplementary/Morfeas_run_check.h"
#include "../Supplementary/Morfeas_JSON.h"
#include "../Supplementary/Morfeas_Logger.h"

//Global variables
static volatile unsigned char handler_run = 1;

//Print the Usage manual
void print_usage(char *prog_name);

//Local Functions
static void stopHandler(int signum)
{
	if(signum == SIGPIPE)
		fprintf(stderr,"IPC: Force Termination!!!\n");
	handler_run = 0;
}
// MDAQ_status_to_IPC function. Send Status of MDAQ to Morfeas_opc_ua via IPC
void MDAQ_status_to_IPC(int FIFO_fd, struct Morfeas_MDAQ_if_stats *stats, int status);

int main(int argc, char *argv[])
{
	//MODBus related variables
	modbus_t *ctx;
	int rc, offset;
	//Apps variables
	char *path_to_logstat_dir;
	//Variables for IPC
	IPC_message IPC_msg = {0};
	struct Morfeas_MDAQ_if_stats stats = {0};
	unsigned short MDAQ_regs[MDAQ_imp_reg];
	//FIFO file descriptor
	int FIFO_fd;
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
	stats.MDAQ_IPv4_addr = argv[optind];
	stats.dev_name = argv[++optind];
	path_to_logstat_dir = argv[++optind];
	//Check arguments
	if(!stats.MDAQ_IPv4_addr || !stats.dev_name)
	{
		fprintf(stderr, "Mandatory Argument missing!!!\n");
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if(!is_valid_IPv4(stats.MDAQ_IPv4_addr))
	{
		fprintf(stderr, "Argument of Device name missing!!!\n");
		exit(EXIT_FAILURE);
	}
	//Check if other instance of this program already runs with same MDAQ_IPv4_addr
	if(check_already_run_with_same_arg(argv[0], stats.MDAQ_IPv4_addr))
	{
		fprintf(stderr, "%s for IPv4:%s Already Running!!!\n", argv[0], stats.MDAQ_IPv4_addr);
		exit(EXIT_SUCCESS);
	}
	//Install stopHandler as the signal handler for SIGINT, SIGTERM and SIGPIPE signals.
	signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    signal(SIGPIPE, stopHandler);

	//Print welcome message
	Logger("---- Morfeas_MDAQ_if Started ----\n",argv[0]);
	if(!path_to_logstat_dir)
		Logger("Argument for path to logstat directory Missing, %s will run in Compatible mode !!!\n",argv[0]);

	//----Make of FIFO file----//
	mkfifo(Data_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	//Register handler to Morfeas_OPC-UA Server
	Logger("Morfeas_MDAQ_if (%s) Send Registration message to OPC-UA via IPC....\n",stats.dev_name);
	//Open FIFO for Write
	FIFO_fd = open(Data_FIFO, O_WRONLY);
	IPC_Handler_reg_op(FIFO_fd, MDAQ, stats.dev_name, 0);
	Logger("Morfeas_MDAQ_if (%s) Registered on OPC-UA\n",stats.dev_name);

	//Make MODBus socket for connection
	ctx = modbus_new_tcp(stats.MDAQ_IPv4_addr, MODBUS_TCP_DEFAULT_PORT);
	//Set Slave address
	if(modbus_set_slave(ctx, default_slave_address))
	{
		fprintf(stderr, "Can't set slave address !!!\n");
		modbus_free(ctx);
		return EXIT_FAILURE;
	}
	//Attempt connection to MDAQ
	while(modbus_connect(ctx) && handler_run)
	{
		sleep(1);
		MDAQ_status_to_IPC(FIFO_fd, &stats, errno);
		Logger("Connection Error (%d): %s\n", errno, modbus_strerror(errno));
	}
	MDAQ_status_to_IPC(FIFO_fd, &stats, 0);
	Logger("Connected to MDAQ %s(%s)\n", stats.MDAQ_IPv4_addr, stats.dev_name);
		//--- main application loop ---//
	//Load dev name and IPv4 address to IPC_msg
	IPC_msg.MDAQ_data.IPC_msg_type = IPC_MDAQ_data;
	memccpy(IPC_msg.MDAQ_data.Dev_or_Bus_name, stats.dev_name,'\0',Dev_or_Bus_name_str_size);
	IPC_msg.MDAQ_data.Dev_or_Bus_name[Dev_or_Bus_name_str_size-1] = '\0';
	inet_pton(AF_INET, stats.MDAQ_IPv4_addr, &(IPC_msg.MDAQ_data.MDAQ_IPv4));
	while(handler_run)
	{
		rc = modbus_read_input_registers(ctx, MDAQ_start_reg, MDAQ_imp_reg, MDAQ_regs);
		if (rc <= 0)
		{
			MDAQ_status_to_IPC(FIFO_fd, &stats, errno);
			Logger("Error (%d) on MODBus Register read: %s\n",errno, modbus_strerror(errno));
			//Attempt to reconnection
			while(modbus_connect(ctx) && handler_run)
				sleep(1);
			Logger("Recover from Last Error\n");
			MDAQ_status_to_IPC(FIFO_fd, &stats, 0);
		}
		else
		{
			// --- Scale measurements and send them to Morfeas_opc_ua via IPC --- //
			//Load MDAQ Board Data
			IPC_msg.MDAQ_data.meas_index = modbus_get_float_cdab(&MDAQ_regs[0]);//30101
			IPC_msg.MDAQ_data.board_temp = modbus_get_float_cdab(&MDAQ_regs[6]);//30107

			//Load MDAQ Channels Data 
			offset = 10;
			for(int i=0; i<8; i++)
			{
				for(int j=0; j<4; j++)
				{
					IPC_msg.MDAQ_data.meas[i].value[j] = modbus_get_float_cdab(&MDAQ_regs[offset]);
					offset+=2;
				}
				IPC_msg.MDAQ_data.meas[i].warnings = (unsigned char)modbus_get_float_cdab(&MDAQ_regs[offset]);
				offset+=2;
			}
			//Send measurements to Morfeas_opc_ua
			//IPC_msg_TX(FIFO_fd, &IPC_msg);

			if(stats.counter >= 10)
			{
				logstat_MDAQ(path_to_logstat_dir, &stats);
				stats.counter = 0;
			}
			else
			{
				//Load MDAQ Board Data to stats
				stats.meas_index = IPC_msg.MDAQ_data.meas_index;
				stats.board_temp += IPC_msg.MDAQ_data.board_temp;
				//Load MDAQ Channels Data to stats
				for(int i=0; i<8; i++)
				{
					stats.meas[i].warnings = IPC_msg.MDAQ_data.meas[i].warnings;
					for(int j=0; j<4; j++)
					{
						//check is value is valid before add to the accumulator
						if(!((stats.meas[i].warnings>>j)&1))
							stats.meas[i].value[j] += IPC_msg.MDAQ_data.meas[i].value[j];
						else
							stats.meas[i].value[j] += stats.meas[i].value[j];
					}
				}
				stats.counter++;
			}
		}
		usleep(100000);
	}
	//Close MODBus connection and De-allocate memory
	modbus_close(ctx);
	modbus_free(ctx);
	//Remove Registeration handler to Morfeas_OPC_UA Server
	IPC_Handler_reg_op(FIFO_fd, MDAQ, stats.dev_name, 1);
	Logger("Morfeas_MDAQ_if (%s) Removed from OPC-UA\n",stats.dev_name);
	close(FIFO_fd);
	//Delete logstat file
	if(path_to_logstat_dir)
		delete_logstat_MDAQ(path_to_logstat_dir, &stats);
	return EXIT_SUCCESS;
}

//print the Usage manual
void print_usage(char *prog_name)
{
	const char preamp[] = {
	"\tProgram: Morfeas_MDAQ_if  Copyright (C) 12019-12020  Sam Harry Tzavaras\n"
    "\tThis program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
    "\tThis is free software, and you are welcome to redistribute it\n"
    "\tunder certain conditions; for details see LICENSE.\n"
	};
	const char manual[] = {
		"\tDev_name: A string that related to the configuration of the MDAQ\n\n"
		"\t    IPv4: The IPv4 address of MDAQ\n\n"
		"Options:\n"
		"           -h : Print help.\n"
		"           -V : Version.\n"
	};
	printf("%s\nUsage: %s IPv4 Dev_name [/path/to/logstat/directory] [Options]\n\n%s",preamp, prog_name, manual);
	return;
}

// MDAQ_status function. Send Status of MDAQ to Morfeas_opc_ua via IPC
void MDAQ_status_to_IPC(int FIFO_fd, struct Morfeas_MDAQ_if_stats *stats, int status)
{
	//Variables for IPC
	IPC_message IPC_msg = {0};
	//scale measurements and send them to Morfeas_opc_ua via IPC
	IPC_msg.MDAQ_report.IPC_msg_type = IPC_MDAQ_report;
	memccpy(IPC_msg.MDAQ_report.Dev_or_Bus_name, stats->dev_name,'\0',Dev_or_Bus_name_str_size);
	IPC_msg.MDAQ_report.Dev_or_Bus_name[Dev_or_Bus_name_str_size-1] = '\0';
	IPC_msg.MDAQ_report.status = status;
	//Send status report
	IPC_msg_TX(FIFO_fd, &IPC_msg);
}

