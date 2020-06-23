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
#define VERSION "0.1" /*Release Version of Morfeas_MTI_if*/

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

//Signal Handler Function
static void stopHandler(int signum)
{
	if(signum == SIGPIPE)
		fprintf(stderr,"IPC: Force Termination!!!\n");
	handler_run = 0;
}

//-- MTI Functions --//
//MTI function that request the MTI's status and load them to stats, return 0 on success
int get_MTI_status(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats);
//MTI function that request the MTI's RX configuration. Load configuration status stats and return "telemetry type". 
int get_MTI_Radio_config(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats);
//MTI function that request from MTI the telemetry data. Load this data to stats. Return 0 in success
int get_MTI_Tele_data(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats);

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
	char *path_to_logstat_dir;
	//Variables for IPC
	IPC_message IPC_msg = {0};
	struct Morfeas_MTI_if_stats stats = {0};
	
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
	//Install stopHandler as the signal handler for SIGINT, SIGTERM and SIGPIPE signals.
	signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    signal(SIGPIPE, stopHandler);

	//Print welcome message
	Logger("---- Morfeas_ΜΤΙ_if Started ----\n");
	Logger("libMODBus Version: %s\n",LIBMODBUS_VERSION_STRING);
	if(!path_to_logstat_dir)
		Logger("Argument for path to logstat directory Missing, %s will run in Compatible mode !!!\n",argv[0]);
	
	/*
	//----Make of FIFO file----//
	mkfifo(Data_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	//Register handler to Morfeas_OPC-UA Server
	Logger("Morfeas_MTI_if (%s) Send Registration message to OPC-UA via IPC....\n",stats.dev_name);
	//Open FIFO for Write
	FIFO_fd = open(Data_FIFO, O_WRONLY);
	IPC_Handler_reg_op(FIFO_fd, MTI, stats.dev_name, 0);
	Logger("Morfeas_MTI_if (%s) Registered on OPC-UA\n",stats.dev_name);
	*/
	
	//Make MODBus socket for connection
	ctx = modbus_new_tcp(stats.MTI_IPv4_addr, MODBUS_TCP_DEFAULT_PORT);
	//Set Slave address
	if(modbus_set_slave(ctx, default_slave_address))
	{
		fprintf(stderr, "Can't set slave address !!!\n");
		modbus_free(ctx);
		return EXIT_FAILURE;
	}
	//Attempt connection to MTI
	while(modbus_connect(ctx) && handler_run)
	{
		sleep(1);
		stats.error = errno;
		//MTI_status_to_IPC(FIFO_fd, &stats);
		Logger("Connection Error (%d): %s\n", errno, modbus_strerror(errno));
		//logstat_MTI(path_to_logstat_dir, &stats);
	}
	stats.error = 0;//load no error on stats
	
	
	while(handler_run)//MTI's FSM
	{
		printf("\n-------------------------------------------------------------------------------------\n");
		Logger("New read status request\n");
		if(!get_MTI_status(ctx, &stats))
		{
			printf("====== MTI Status =====\n");
			printf("MTI batt=%.2fV\n",stats.MTI_status.MTI_batt_volt);
			printf("MTI batt cap=%.2f%%\n",stats.MTI_status.MTI_batt_capacity);
			printf("MTI batt state=%s\n",MTI_charger_state_str[stats.MTI_status.MTI_charge_status]);
			printf("MTI cpu_temp=%.2f°C\n",stats.MTI_status.MTI_CPU_temp);
			printf("Bt1=%d\tBt2=%d\tBt3=%d\n",
				stats.MTI_status.buttons_state.pb1,
				stats.MTI_status.buttons_state.pb2,
				stats.MTI_status.buttons_state.pb3);
			printf("MTI PWM_gen_out_freq=%.2fHz\n",stats.MTI_status.PWM_gen_out_freq);
			for(int i=0; i<4; i++)
				printf("MTI PWM_outDuty_CH[%d]=%.2f%%\n",i,stats.MTI_status.PWM_outDuty_CHs[i]);
			printf("=======================\n");
		}
		else
			Logger("get_MTI_status request Failed!!!\n");
		
		Logger("New get_MTI_Radio_config request\n");
		if(get_MTI_Radio_config(ctx, &stats)>=0)
		{
			printf("=== RX configuration ==\n");
			printf("RX Frequency=%.3fGHz\n",(2400+stats.MTI_Radio_config.RX_channel)/1000.0);
			printf("Data_rate=%s\n",MTI_Data_rate_str[stats.MTI_Radio_config.Data_rate]);
			printf("Tele_dev_type=%s\n",MTI_Tele_dev_type_str[stats.MTI_Radio_config.Tele_dev_type]);
			for(int i=0; i<sizeof(stats.MTI_Radio_config.Specific_reg)/sizeof(short); i++)
				printf("SFR[%d]=%d(0x%x)\n", i, stats.MTI_Radio_config.Specific_reg[i], stats.MTI_Radio_config.Specific_reg[i]);
			printf("=======================\n");
		
			Logger("New get_MTI_Tele_data request\n");
			if(!get_MTI_Tele_data(ctx, &stats))
			{
				printf("\n===== Tele data =====\n");
				if(stats.MTI_Radio_config.Tele_dev_type!=RM_SW_MUX)
				{
					printf("Telemetry data is%s valid\n", stats.Tele_data.as_TC4.Data_isValid?"":" NOT");
					printf("Packet Index=%d\n", stats.Tele_data.as_TC4.packet_index);
					printf("RX Status=%d\n", stats.Tele_data.as_TC4.RX_status);
					printf("RX success Ratio=%d%%\n", stats.Tele_data.as_TC4.RX_Success_ratio);
				}
				printf("\n===== Data =====\n");
				switch(stats.MTI_Radio_config.Tele_dev_type)
				{
					case Tele_TC4:
						for(int i=0; i<sizeof(stats.Tele_data.as_TC4.CHs)/sizeof(*stats.Tele_data.as_TC4.CHs);i++)
							printf("CH%2d -> %.3f\n",i,stats.Tele_data.as_TC4.CHs[i]);
						for(int i=0; i<sizeof(stats.Tele_data.as_TC4.Refs)/sizeof(*stats.Tele_data.as_TC4.Refs);i++)
							printf("REF%2d -> %.3f\n",i,stats.Tele_data.as_TC4.Refs[i]);
						break;
					case Tele_TC8:
						for(int i=0; i<sizeof(stats.Tele_data.as_TC8.CHs)/sizeof(*stats.Tele_data.as_TC8.CHs);i++)
							printf("CH%2d -> %.3f\n",i,stats.Tele_data.as_TC8.CHs[i]);
						for(int i=0; i<sizeof(stats.Tele_data.as_TC8.Refs)/sizeof(*stats.Tele_data.as_TC8.Refs);i++)
							printf("REF%2d -> %.3f\n",i,stats.Tele_data.as_TC8.Refs[i]);
						break;
					case Tele_TC16:
						for(int i=0; i<sizeof(stats.Tele_data.as_TC16.CHs)/sizeof(*stats.Tele_data.as_TC16.CHs);i++)
							printf("CH%2d -> %.3f\n",i,stats.Tele_data.as_TC16.CHs[i]);
						break;
					case Tele_quad:
						for(int i=0; i<sizeof(stats.Tele_data.as_QUAD.CHs)/sizeof(*stats.Tele_data.as_QUAD.CHs);i++)
							printf("CH%2d -> %.3f\n",i,stats.Tele_data.as_QUAD.CHs[i]);
						break;
					case RM_SW_MUX:
					
						break;
				}
				printf("=======================\n");
			}
			else
				Logger("get_MTI_Tele_data request Failed!!!\n");
		}
		else
			Logger("get_MTI_Radio_config request Failed!!!\n");
		usleep(100000);
	}
	
	//Close MODBus connection and De-allocate memory
	modbus_close(ctx);
	modbus_free(ctx);
	/*
	//Remove Registered handler from Morfeas_OPC_UA Server
	IPC_Handler_reg_op(FIFO_fd, MTI, stats.dev_name, 1);
	Logger("Morfeas_MTI_if (%s) Removed from OPC-UA\n",stats.dev_name);
	close(FIFO_fd);
	//Delete logstat file
	if(path_to_logstat_dir)
		delete_logstat_MTI(path_to_logstat_dir, &stats);
	*/
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