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

#define MAX_CANBus_FPS 1700.7 //Maximum amount of frames per sec for 250Kbaud

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

enum bitrate_check_return_values{
	bitrate_check_success,
	bitrate_check_error,
	bitrate_check_invalid,
};

static volatile struct Morfeas_NOX_if_flags{
	unsigned run : 1;
	unsigned port_meas_exist :1;
	unsigned bus_info :1;
}flags = {.run=1,0};

/* Local function (declaration)
 * Return value: EXIT_FAILURE(1) of failure or EXIT_SUCCESS(0) on success. Except of other notice
 */
int CAN_if_bitrate_check(char *CAN_IF_name, int *bitrate);
void quit_signal_handler(int signum);//SIGINT handler function
void print_usage(char *prog_name);//print the usage manual

int main(int argc, char *argv[])
{
	//Directory pointer variables
	DIR *dir;
	//Operational variables
	int c;
	char *logstat_path;
	unsigned char i2c_bus_num = I2C_BUS_NUM;
	unsigned long msg_cnt=0;
	//Variables for CANBus Port Electric
	struct tm Morfeas_RPi_Hat_last_cal = {0};
	struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config port_meas_config = {0};
	struct Morfeas_RPi_Hat_Port_meas port_meas = {0};
	//Variables for IPC
	IPC_message IPC_msg = {0};
	//Variables for Socket CAN and NOX_id decoder
	int RX_bytes, CAN_socket_num;
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr = {0};
	struct can_frame frame_rx;
	struct can_filter RX_filter;
	NOx_can_id *NOx_id_dec;
	//Stats of the Morfeas-NOX_IF
	struct Morfeas_NOX_if_stats stats = {0};

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
	//Check if program already runs on same bus.
	if(check_already_run_with_same_arg(argv[0], argv[optind]))
	{
		fprintf(stderr, "%s for interface \"%s\" Already Running!!!\n", argv[0], argv[optind]);
		exit(EXIT_SUCCESS);
	}
	//Check if the size of the CAN-IF is bigger than the limit.
	stats.CAN_IF_name = argv[optind];
	if(strlen(stats.CAN_IF_name)>=Dev_or_Bus_name_str_size)
	{
		fprintf(stderr, "Interface name too big (>=%d)\n",Dev_or_Bus_name_str_size);
		exit(EXIT_FAILURE);
	}
	//Check bitrate of CAN-IF
	switch(CAN_if_bitrate_check(stats.CAN_IF_name, &CAN_socket_num))
	{
		case bitrate_check_invalid:
			fprintf(stderr, "Speed of CAN-IF(%s) is incompatible with UNI-NOx (%d != %d)!!!\n", stats.CAN_IF_name, CAN_socket_num, NOx_Bitrate);
			break;
		case bitrate_check_error:
			fprintf(stderr, "CAN_if_bitrate_check() failed !!!\n");
			break;
	}
	//CAN Socket Opening
	if((CAN_socket_num = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
		perror("Error while opening socket");
		exit(EXIT_FAILURE);
	}
	//Link interface name to socket
	strncpy(ifr.ifr_name, stats.CAN_IF_name, IFNAMSIZ-1); // get value from CAN-IF arguments
	if(ioctl(CAN_socket_num, SIOCGIFINDEX, &ifr))
	{
		char if_error[30];
		sprintf(if_error, "CAN-IF (%s)",ifr.ifr_name);
		perror(if_error);
		exit(EXIT_FAILURE);
	}
	//Logstat.json
	if(!(logstat_path = argv[optind+1]))
		fprintf(stderr,"No logstat_path argument. Running without logstat\n");
	else
	{
		dir = opendir(logstat_path);
		if (dir)
			closedir(dir);
		else
		{
			fprintf(stderr,"logstat_path is invalid!\n");
			return EXIT_FAILURE;
		}
	}

	/*Filter for CAN messages	-- SocketCAN Filters act as: <received_can_id> & mask == can_id & mask*/
	//load filter's can_id member
	NOx_id_dec = (NOx_can_id *)&RX_filter.can_id;//Set encoder to filter.can_id
	memset(NOx_id_dec, 0, sizeof(NOx_can_id));
	NOx_id_dec->flags = 4;//set the EFF
	NOx_id_dec->NOx_addr = NOx_filter; // Received Messages from NOx Sensors only
	//load filter's can_mask member
	NOx_id_dec = (NOx_can_id *)&RX_filter.can_mask; //Set encoder to filter.can_mask
	memset(NOx_id_dec, 0, sizeof(NOx_can_id));
	NOx_id_dec->flags = 4;//Received only messages with extended ID (29bit)
	NOx_id_dec->NOx_addr = NOx_mask; // Constant field of NOx_addr marked for examination
	setsockopt(CAN_socket_num, SOL_CAN_RAW, CAN_RAW_FILTER, &RX_filter, sizeof(RX_filter));

	// Add timeout option to the CAN Socket
	tv.tv_sec = 1;//Timeout after 1 second.
	tv.tv_usec = 0;
	setsockopt(CAN_socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	//Bind CAN Socket to address
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if(bind(CAN_socket_num, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("Error in socket bind");
		exit(EXIT_FAILURE);
	}

	//Link signal SIGINT, SIGTERM and SIGPIPE to quit_signal_handler
	signal(SIGINT, quit_signal_handler);
	signal(SIGTERM, quit_signal_handler);
	signal(SIGPIPE, quit_signal_handler);
	Logger("Morfeas_NOX_if (%s) Program Started\n",stats.CAN_IF_name);
	//Get SDAQ_NET Port config
	stats.port = get_port_num(stats.CAN_IF_name);
	if(stats.port>=0 && stats.port<=3)
	{
		//Init SDAQ_NET Port's CSA
		if(!MAX9611_init(stats.port, i2c_bus_num))
		{
			//Read Port's CSA Configuration from EEPROM
			if(!read_port_config(&port_meas_config, stats.port, i2c_bus_num))
			{
				flags.port_meas_exist = 1;
				Logger("Port's Last Calibration: %u/%u/%u\n", port_meas_config.last_cal_date.month,
															  port_meas_config.last_cal_date.day,
															  port_meas_config.last_cal_date.year+12000);
				//Convert date from port_meas_config to struct tm Morfeas_RPi_Hat_last_cal
				Morfeas_RPi_Hat_last_cal.tm_mon = port_meas_config.last_cal_date.month - 1;
				Morfeas_RPi_Hat_last_cal.tm_mday = port_meas_config.last_cal_date.day;
				Morfeas_RPi_Hat_last_cal.tm_year = port_meas_config.last_cal_date.year + 100;
				stats.Morfeas_RPi_Hat_last_cal = mktime(&Morfeas_RPi_Hat_last_cal);//Convert Morfeas_RPi_Hat_last_cal to time_t
			}
			else
				Logger(Morfeas_hat_error());
		}
		else
		{
			Logger(Morfeas_hat_error());
			Logger("SDAQnet Port CSA not found!!!\n");
		}
	}
	else
	{
		stats.Bus_voltage = NAN;
		stats.Bus_amperage = NAN;
		stats.Shunt_temp = NAN;
	}
	//----Make of FIFO file----//
	mkfifo(Data_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	/*
	//Register handler to Morfeas_OPC-UA Server
	Logger("Morfeas_NOX_if (%s) Send Registration message to OPC-UA via IPC....\n",stats.CAN_IF_name);
	//Open FIFO for Write
	stats.FIFO_fd = open(Data_FIFO, O_WRONLY);
	IPC_Handler_reg_op(stats.FIFO_fd, NOX, stats.CAN_IF_name, 0);
	Logger("Morfeas_NOX_if (%s) Registered on OPC-UA\n",stats.CAN_IF_name);
	*/

	//-----Actions on the bus-----//
	NOx_id_dec = (NOx_can_id *)&(frame_rx.can_id);
	while(flags.run)//FSM of Morfeas_NOX_if
	{
		RX_bytes=read(CAN_socket_num, &frame_rx, sizeof(frame_rx));
		if(RX_bytes==sizeof(frame_rx))
		{
			switch(NOx_id_dec->NOx_addr)
			{
				case NOx_low_addr:
					Logger("RX from NOx_low_addr\n");
					break;
				case NOx_high_addr:
					Logger("RX from NOx_high_addr\n");
					break;
			}
			msg_cnt++;//increase message counter
		}
		else if(RX_bytes<0)
			Logger("Timeout\n");
	}
	Logger("Morfeas_NOX_if (%s) Exiting...\n",stats.CAN_IF_name);
	close(CAN_socket_num);//Close CAN_socket
	/*
	//Remove Registeration handler to Morfeas_OPC_UA Server
	IPC_Handler_reg_op(stats.FIFO_fd, NOX, stats.CAN_IF_name, 1);
	Logger("Morfeas_NOX_if (%s) Removed from OPC-UA\n", stats.CAN_IF_name);
	close(stats.FIFO_fd);
	//Delete logstat file
	if(logstat_path)
		delete_logstat_NOX(logstat_path, &stats);
	*/
	return EXIT_SUCCESS;
}

inline void quit_signal_handler(int signum)
{
	if(signum == SIGPIPE)
		fprintf(stderr,"IPC: Force Termination!!!\n");
	else if(!flags.run && signum == SIGINT)
		raise(SIGABRT);
	flags.run = 0;
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

int CAN_if_bitrate_check(char *CAN_IF_name, int *bitrate)
{
	struct can_bittiming bt={0};
	if(!CAN_IF_name)
		return bitrate_check_error;
	if(can_get_bittiming(CAN_IF_name, &bt))
		return bitrate_check_error;
	*bitrate = bt.bitrate;
	if(bt.bitrate!=NOx_Bitrate)
		return bitrate_check_invalid;
	return bitrate_check_success;
}