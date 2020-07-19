/*
File: Morfeas_MTI_if.c, Implementation of Morfeas MTI (MODBus) handler, Part of Morfeas_project.
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
#define VERSION "0.5" /*Release Version of Morfeas_MTI_if*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>
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

//Enumerator for the FSM's state
enum Morfeas_MTI_FSM_States{
	error,
	get_status_config_data/*,
	get_status,
	get_config,
	get_data,
	sleep
	*/
};

//Global variables
unsigned char handler_run = 1;
pthread_mutex_t MTI_access = PTHREAD_MUTEX_INITIALIZER;

//Print the Usage manual
void print_usage(char *prog_name);

//Signal Handler Function
static void stopHandler(int signum)
{
	if(signum == SIGPIPE)
		fprintf(stderr,"IPC: Force Termination!!!\n");
	handler_run = 0;
}

	//--- MTI's Read Functions ---//
//MTI function that request the MTI's status and load them to stats. Return 0 on success.
int get_MTI_status(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats);
//MTI function that request the MTI's RX configuration and load it to stats. Return 0 in success.
int get_MTI_Radio_config(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats);
//MTI function that request from MTI the telemetry data and load them to stats. Return 0 in success.
int get_MTI_Tele_data(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats);

//--- D-Bus Listener function ---//
void * MTI_DBus_listener(void *varg_pt);//Thread function.

//--- Local functions ---//
/*
// MTI_status_to_IPC function. Send Status of MTI to Morfeas_opc_ua via IPC
void MTI_status_to_IPC(int FIFO_fd, struct Morfeas_MTI_if_stats *stats);
//Function that register MTI Channels to Morfeas_opc_ua via IPC
void IPC_Channels_reg(int FIFO_fd, struct Morfeas_MTI_if_stats *stats);
*/

int main(int argc, char *argv[])
{
	//MODBus related variables
	modbus_t *ctx;
	//Apps variables
	char *path_to_logstat_dir, state = get_status_config_data;
	struct Morfeas_MTI_if_stats stats = {0};
	//Variables for threads
	pthread_t DBus_listener_Thread_id;
	struct MTI_DBus_thread_arguments_passer passer = {&ctx, &stats};
	//Variables for IPC
	IPC_message IPC_msg = {0};
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
	stats.MTI_IPv4_addr = argv[optind];
	stats.dev_name = argv[++optind];
	path_to_logstat_dir = argv[++optind];
	//Check arguments
	if(!stats.MTI_IPv4_addr || !stats.dev_name)
	{
		fprintf(stderr, "Mandatory Argument(s) missing!!!\n");
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if(!is_valid_IPv4(stats.MTI_IPv4_addr))
	{
		fprintf(stderr, "Argument for IPv4 is invalid!!!\n");
		exit(EXIT_FAILURE);
	}
	//Check if other instance of this program already runs with same MTI_IPv4_addr
	if(check_already_run_with_same_arg(argv[0], stats.MTI_IPv4_addr))
	{
		fprintf(stderr, "%s for IPv4:%s Already Running!!!\n", argv[0], stats.MTI_IPv4_addr);
		exit(EXIT_SUCCESS);
	}
	//Check if other instance of this program already runs with same Device Name
	if(check_already_run_with_same_arg(argv[0], stats.dev_name))
	{
		fprintf(stderr, "%s with Dev_name:%s Already Running!!!\n", argv[0], stats.dev_name);
		exit(EXIT_SUCCESS);
	}
	//Install stopHandler as the signal handler for SIGINT, SIGTERM and SIGPIPE signals.
	signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    signal(SIGPIPE, stopHandler);

	//Print welcome message
	Logger("---- Morfeas_ΜΤΙ_if Started ----\n");
	Logger("libMODBus Version: %s\n",LIBMODBUS_VERSION_STRING);
	if(!path_to_logstat_dir)
		Logger("Argument for path to logstat directory Missing, %s will run in Compatible mode !!!\n",argv[0]);

	//----Make of FIFO file----//
	mkfifo(Data_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	//Open FIFO for Write
	FIFO_fd = open(Data_FIFO, O_WRONLY);
	/*
	//Register handler to Morfeas_OPC-UA Server
	Logger("Morfeas_MTI_if (%s) Send Registration message to OPC-UA via IPC....\n",stats.dev_name);
	IPC_Handler_reg_op(FIFO_fd, MTI, stats.dev_name, 0);
	Logger("Morfeas_MTI_if (%s) Registered on OPC-UA\n",stats.dev_name);
	*/

	//Make MODBus socket for connection
	ctx = modbus_new_tcp(stats.MTI_IPv4_addr, MODBUS_TCP_DEFAULT_PORT);
	//Set Slave address
	if(modbus_set_slave(ctx, default_slave_address))
	{
		fprintf(stderr, "Can't set slave address!!!\n");
		modbus_free(ctx);
		return EXIT_FAILURE;
	}

	//Attempt connection to MTI
	while(modbus_connect(ctx) && handler_run)
	{
		stats.error = errno;
		//MTI_status_to_IPC(FIFO_fd, &stats);
		Logger("Connection Error (%d): %s\n", errno, modbus_strerror(errno));
		logstat_MTI(path_to_logstat_dir, &stats);
		sleep(1);
	}
	if(!handler_run)
		goto Exit;
	//Print Connection success message
	Logger("Connected to MTI %s(%s)\n", stats.MTI_IPv4_addr, stats.dev_name);
	stats.error = 0;//load no error on stats
	//MTI_status_to_IPC(FIFO_fd, &stats);//send status report to Morfeas_opc_ua via IPC

	//Start D-Bus listener function in a thread
	pthread_create(&DBus_listener_Thread_id, NULL, MTI_DBus_listener, &passer);

	while(handler_run)//Application's FSM
	{
		switch(state)
		{
			case error:
				stats.error = errno;//load errno to stats
				//MTI_status_to_IPC(FIFO_fd, &stats);//send status report to Morfeas_opc_ua via IPC
				logstat_MTI(path_to_logstat_dir, &stats);//report error on logstat
				Logger("Error (%d) on MODBus Register read: %s\n",errno, modbus_strerror(errno));
				//Attempt to reconnect
				while(modbus_connect(ctx) && handler_run)
				{
					stats.error = errno;
					pthread_mutex_unlock(&MTI_access);
						sleep(1);
					pthread_mutex_lock(&MTI_access);
				}
				Logger("Recover from last Error\n");
				stats.error = 0;//load no error on stats
				//MTI_status_to_IPC(FIFO_fd, &stats);//send status report to Morfeas_opc_ua via IPC
				logstat_MTI(path_to_logstat_dir, &stats);//report error on logstat
				state = get_status_config_data;
				break;
			case get_status_config_data:
				if(!get_MTI_status(ctx, &stats))
				{
					if(!get_MTI_Radio_config(ctx, &stats))
					{
						if(stats.MTI_Radio_config.Tele_dev_type >= Dev_type_min && stats.MTI_Radio_config.Tele_dev_type <= Dev_type_max)// Execute if MTI's transceiver is enabled
						{
							if(get_MTI_Tele_data(ctx, &stats))
							{
								state = error;
								break;
							}
						}
						//IPC_msg_TX(FIFO_fd, &IPC_msg);//Send measurements to Morfeas_opc_ua
						if(stats.counter >= 10)
						{
							logstat_MTI(path_to_logstat_dir, &stats);
							stats.counter = 0;
						}
						else
							stats.counter++;
						break;
					}
				}
				state = error;
				break;
		}
		usleep(100000);
	}

	pthread_join(DBus_listener_Thread_id, NULL);// wait DBus_listener thread to end
	pthread_detach(DBus_listener_Thread_id);//deallocate DBus_listener thread's memory
Exit:
	//Close MODBus connection and deallocate memory
	modbus_close(ctx);
	modbus_free(ctx);
	//Remove Registered handler from Morfeas_OPC_UA Server
	/*
	IPC_Handler_reg_op(FIFO_fd, MTI, stats.dev_name, 1);
	Logger("Morfeas_MTI_if (%s) Removed from OPC-UA\n",stats.dev_name);
	*/
	close(FIFO_fd);
	//Delete logstat file
	if(path_to_logstat_dir)
		delete_logstat_MTI(path_to_logstat_dir, &stats);
	return EXIT_SUCCESS;
}

//print the Usage manual
void print_usage(char *prog_name)
{
	const char preamp[] = {
	"\tProgram: Morfeas_MTI_if  Copyright (C) 12019-12020  Sam Harry Tzavaras\n"
    "\tThis program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
    "\tThis is free software, and you are welcome to redistribute it\n"
    "\tunder certain conditions; for details see LICENSE.\n"
	};
	const char manual[] = {
		"\tDev_name: A string that related to the configuration of the MTI\n\n"
		"\t    IPv4: The IPv4 address of MTI\n\n"
		"Options:\n"
		"           -h : Print help.\n"
		"           -V : Version.\n"
	};
	printf("%s\nUsage: %s IPv4 Dev_name [/path/to/logstat/directory] [Options]\n\n%s",preamp, prog_name, manual);
	return;
}
