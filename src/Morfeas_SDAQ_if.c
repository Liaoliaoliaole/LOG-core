/*
Program: Morfeas-SDAQ-if. A controlling software for SDAQ-CAN Devices, part of morfeas_core project.
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
#define YELLOW_LED 19
#define RED_LED 13
#define LogBooks_dir "/var/tmp/Morfeas_LogBooks/"
#define SYNC_INTERVAL 10//seconds
#define LIFE_TIME 15 // Value In seconds, define the time that a SDAQ_info_entry node defined as off-line and removed from the list
#define MAX_CANBus_FPS 3401.4 //Maximum amount of frames per sec for 500Kbaud

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
#include "Morfeas_JSON.h"
#include "Morfeas_IPC.h"// including -> "Types.h"

static struct Morfeas_SDAQ_if_flags{
	unsigned run : 1;
	unsigned led_existent :1;
	unsigned Clean_flag :1;
	unsigned bus_info :1;
}flags = {.run=1,0};

//Global variables
static struct timespec tstart;
static int CAN_socket_num;

/* Local function (declaration)
 * Return value: EXIT_FAILURE(1) of failure or EXIT_SUCCESS(0) on success. Except of other notice
 */
void quit_signal_handler(int signum);//SIGINT handler function
void CAN_if_timer_handler(int signum);//sync timer handler function
void print_usage(char *prog_name);//print the usage manual

	/*Morfeas_SDAQ-if functions*/
//Error Warning Leds controlling function
void led_init();
void led_stat(struct Morfeas_SDAQ_if_stats *stats);
//Logbook read and write from file;
void LogBook_file(struct Morfeas_SDAQ_if_stats *stats, char *read_write_or_append);
//Function to clean-up list_SDAQs from non active SDAQ, also send IPC msg to opc_ua for each dead SDAQ
int clean_up_list_SDAQs(struct Morfeas_SDAQ_if_stats *stats);
//Function that found and return the status of a node from the list_SDAQ with SDAQ_address == address. Used in FSM
struct SDAQ_info_entry * find_SDAQ(unsigned char address, struct Morfeas_SDAQ_if_stats *stats);
//Function that add or refresh SDAQ to lists list_SDAQ and LogBook, Return the data of node or NULL. Used in FSM
struct SDAQ_info_entry * add_or_refresh_SDAQ_to_lists(int socket_fd, sdaq_can_id *sdaq_id_dec, sdaq_status *status_dec, struct Morfeas_SDAQ_if_stats *stats);
//Function for Updating "Device Info" of a SDAQ. Used in FSM
int update_info(unsigned char address, sdaq_info *info_dec, struct Morfeas_SDAQ_if_stats *stats);
//Function for Updating "Calibration Date" of a SDAQ's channel. Used in FSM
int add_update_channel_date(unsigned char address, unsigned char channel, sdaq_calibration_date *date_dec, struct Morfeas_SDAQ_if_stats *stats);
//Function that find and return the amount of incomplete (with out all info and dates) nodes.
int incomplete_SDAQs(struct Morfeas_SDAQ_if_stats *stats);
//Function for Updating Time_diff (from debugging message) of a SDAQ. Used in FSM, also send IPC msg t opc_ua.
int update_Timediff(unsigned char address, sdaq_sync_debug_data *ts_dec, struct Morfeas_SDAQ_if_stats *stats);
//Function for construction of message for registration or update of a SDAQ
int IPC_SDAQ_reg_update(int FIFO_fd, char connected_to_BUS[10], unsigned char address, sdaq_status *SDAQ_status, unsigned char amount);

	/*GSList related functions*/
void free_SDAQ_info_entry(gpointer node);//used with g_slist_free_full to free the data of each node of list_SDAQs
void free_LogBook_entry(gpointer node);//used with g_slist_free_full to free the data of each node of list LogBook

int main(int argc, char *argv[])
{
	//Directory pointer variables
	DIR *dir;
	//Operational variables
	char *logstat_path = argv[2];
	unsigned long msg_cnt=0;
	struct SDAQ_info_entry *SDAQ_data;
	//Variables for IPC
	IPC_message IPC_msg;
	//Variables for Socket CAN and SDAQ_decoders
	int RX_bytes;
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr;
	struct can_frame frame_rx;
	struct can_filter RX_filter;
	sdaq_can_id *sdaq_id_dec;
	sdaq_status *status_dec = (sdaq_status *)frame_rx.data;
	sdaq_info *info_dec = (sdaq_info *)frame_rx.data;
	sdaq_sync_debug_data *ts_dec = (sdaq_sync_debug_data *)frame_rx.data;
	sdaq_calibration_date *date_dec = (sdaq_calibration_date *)frame_rx.data;
	sdaq_meas *meas_dec = (sdaq_meas *)frame_rx.data;

	//data_and stats of the Morfeas-SDAQ_IF
	struct Morfeas_SDAQ_if_stats stats = {0};
	//Timers related Variables
	struct itimerval timer;

	if(argc == 1)
	{
		print_usage(argv[0]);
		exit(1);
	}

	//Check the existence of the LogBooks directory
	dir = opendir(LogBooks_dir);
	if (dir)
		closedir(dir);
	else
	{
		printf("Making LogBooks_dir \n");
		if(mkdir(LogBooks_dir, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH))
		{
			perror("Error at LogBook file creation!!!");
			exit(EXIT_FAILURE);
		}
	}
	//CAN Socket Opening
	if((CAN_socket_num = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
		perror("Error while opening socket");
		exit(EXIT_FAILURE);
	}
	//Link interface name to socket
	strcpy(ifr.ifr_name, argv[1]); // get value from CAN-IF arguments
	if(ioctl(CAN_socket_num, SIOCGIFINDEX, &ifr))
	{
		perror("CAN-IF");
		exit(EXIT_FAILURE);
	}
	stats.CAN_IF_name = argv[1];
	//Logstat.json
	if(!logstat_path)
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
	sdaq_id_dec = (sdaq_can_id *)&RX_filter.can_id;//Set encoder to filter.can_id
	memset(sdaq_id_dec, 0, sizeof(sdaq_can_id));
	sdaq_id_dec->flags = 4;//set the EFF
	sdaq_id_dec->protocol_id = PROTOCOL_ID; // Received Messages with protocol_id == PROTOCOL_ID
	sdaq_id_dec->payload_type = 0x80; //  Received Messages with payload_type & 0x80 == TRUE, aka Master <- SDAQ.
	//load filter's can_mask member
	sdaq_id_dec = (sdaq_can_id *)&RX_filter.can_mask; //Set encoder to filter.can_mask
	memset(sdaq_id_dec, 0, sizeof(sdaq_can_id));
	sdaq_id_dec->flags = 4;//Received only messages with extended ID (29bit)
	sdaq_id_dec->protocol_id = -1; // Protocol_id field marked for examination
	sdaq_id_dec->payload_type = 0x80; // + The most significant bit of Payload_type field marked for examination.
	setsockopt(CAN_socket_num, SOL_CAN_RAW, CAN_RAW_FILTER, &RX_filter, sizeof(RX_filter));

	// Add timeout option to the CAN Socket
	tv.tv_sec = 10;//interval time that a SDAQ send a Status/ID frame.
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
	//Link signal SIGALRM to timer's handler
	signal(SIGALRM, CAN_if_timer_handler);
	//Link signal SIGINT and SIGPIPE to quit_signal_handler
	signal(SIGINT, quit_signal_handler);
	signal(SIGPIPE, quit_signal_handler);
	//initialize the indication LEDs of the Morfeas-proto (sysfs implementation)
	led_init(stats.CAN_IF_name);

	//Load the LogBook file to LogBook List
	sprintf(stats.LogBook_file_path,"%sMorfeas_SDAQ_if_%s_LogBook",LogBooks_dir,stats.CAN_IF_name);
	LogBook_file(&stats, "r");

	//----Make of FIFO file----//
	mkfifo(Data_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	//Open FIFO for Write
	stats.FIFO_fd = open(Data_FIFO, O_WRONLY);
	//Register handler to Morfeas_OPC-UA Server
	IPC_Handler_reg_op(stats.FIFO_fd, SDAQ, stats.CAN_IF_name, 0);

	//Initialize Sync timer expired time
	memset (&timer, 0, sizeof(struct itimerval));
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = timer.it_interval.tv_sec;
	timer.it_value.tv_usec = timer.it_interval.tv_usec;
	//Get start time
	clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
	//start timer
	setitimer(ITIMER_REAL, &timer, NULL);

	//-----Actions on the bus-----//
	sdaq_id_dec = (sdaq_can_id *)&(frame_rx.can_id);//point ID decoder to ID field from frame_rx
	//Stop any measuring activity on the bus
	Stop(CAN_socket_num, Broadcast);
	while(flags.run)//FSM of Morfeas_SDAQ_if
	{
		RX_bytes=read(CAN_socket_num, &frame_rx, sizeof(frame_rx));
		if(RX_bytes==sizeof(frame_rx))
		{
			switch(sdaq_id_dec->payload_type)
			{
				case Measurement_value:
					if((SDAQ_data = find_SDAQ(sdaq_id_dec->device_addr, &stats)))
					{
						time(&(SDAQ_data->last_seen));
						//Send measurement through IPC
						IPC_msg.SDAQ_meas.IPC_msg_type = IPC_SDAQ_meas;
						memccpy(IPC_msg.SDAQ_meas.connected_to_BUS,stats.CAN_IF_name,'\0',connected_to_BUS_str_size);
						IPC_msg.SDAQ_meas.connected_to_BUS[connected_to_BUS_str_size-1] = '\0';
						IPC_msg.SDAQ_meas.SDAQ_serial_number = SDAQ_data->SDAQ_status.dev_sn;
						IPC_msg.SDAQ_meas.channel = sdaq_id_dec->channel_num;
						memcpy(&(IPC_msg.SDAQ_meas.SDAQ_channel_meas), meas_dec, sizeof(sdaq_meas));
						IPC_msg_TX(stats.FIFO_fd, &IPC_msg);
					}
					break;
				case Sync_Info:
					update_Timediff(sdaq_id_dec->device_addr, ts_dec, &stats);
					break;
				case Device_status:
					if((SDAQ_data = add_or_refresh_SDAQ_to_lists(CAN_socket_num, sdaq_id_dec, status_dec, &stats)))
					{
						if(!status_dec->status)//SDAQ of sdaq_id_dec->device_addr not measuring, no error and on normal mode
						{
							if(!SDAQ_data->info_collection_status)//set QueryDeviceInfo on entries without filled info
							{
								QueryDeviceInfo(CAN_socket_num,SDAQ_data->SDAQ_address);
								SDAQ_data->info_collection_status = 1;
							}
							else if(SDAQ_data->info_collection_status == 3 && !incomplete_SDAQs(&stats))
								Start(CAN_socket_num, sdaq_id_dec->device_addr);
						}
						IPC_SDAQ_reg_update(stats.FIFO_fd, stats.CAN_IF_name, SDAQ_data->SDAQ_address, status_dec, stats.detected_SDAQs);
					}
					else
						printf("\n\t\tMaximum amount of addresses is reached\n");
					break;
				case Device_info:
					update_info(sdaq_id_dec->device_addr, info_dec, &stats);
					break;
				case Calibration_Date:
					add_update_channel_date(sdaq_id_dec->device_addr, sdaq_id_dec->channel_num, date_dec, &stats);
					break;
			}
			msg_cnt++;//increase message counter
		}
		if(flags.Clean_flag)
		{
			clean_up_list_SDAQs(&stats);
			//logstat_json(logstat_path,&stats);
			flags.Clean_flag = 0;
		}
		if(flags.bus_info)
		{
			//Calculate CANBus utilization
			stats.Bus_util = 100.0*(msg_cnt/MAX_CANBus_FPS);
			msg_cnt = 0;
			flags.bus_info = 0;
			//transfer bus utilization to opc_ua
			IPC_msg.BUS_info.IPC_msg_type = IPC_CAN_BUS_info;
			sprintf(IPC_msg.BUS_info.connected_to_BUS,"%s",stats.CAN_IF_name);
			IPC_msg.BUS_info.BUS_utilization = stats.Bus_util;
			IPC_msg.BUS_info.voltage = stats.Bus_voltage;
			IPC_msg.BUS_info.amperage = stats.Bus_amperage;
			IPC_msg.BUS_info.shunt_temp = stats.Shunt_temp;
			IPC_msg_TX(stats.FIFO_fd, &IPC_msg);
			logstat_json(logstat_path,&stats);
		}
		led_stat(&stats);
	}
	printf("\nExiting...\n");
	// save LogBook list to a file before destroy it
	LogBook_file(&stats,"w");
	//free all lists
	g_slist_free_full(stats.list_SDAQs, free_SDAQ_info_entry);
	g_slist_free_full(stats.LogBook, free_LogBook_entry);
	//Stop any measuring activity on the bus
	Stop(CAN_socket_num,Broadcast);
	close(CAN_socket_num);//Close CAN_socket
	//Remove Registeration handler to Morfeas_OPC_UA Server
	IPC_Handler_reg_op(stats.FIFO_fd, SDAQ, stats.CAN_IF_name, 1);
	close(stats.FIFO_fd);
	return EXIT_SUCCESS;
}

void led_init(char *CAN_IF_name)
{
	char path[35];
	char buffer[3];
	ssize_t bytes_written;
	int sysfs_fd, i, pin;
	if(!strcmp(CAN_IF_name, "can0") || !strcmp(CAN_IF_name, "can1"))
	{	//init GPIO on sysfs
		for(i=0; i<2; i++)
		{
			sysfs_fd = open("/sys/class/gpio/export", O_WRONLY);
			if(sysfs_fd < 0)
			{
				fprintf(stderr, "LEDs are Not supported!\n");
				return;
			}
			pin = i ? YELLOW_LED : RED_LED;
			bytes_written = snprintf(buffer, 3, "%d", pin);
			write(sysfs_fd, buffer, bytes_written);
			close(sysfs_fd);
		}
		sleep(1);
		//Set direction of GPIOs
		for(i=0; i<2; i++)
		{
			pin = i ? YELLOW_LED : RED_LED;
			snprintf(path, 35, "/sys/class/gpio/gpio%d/direction", pin);
			sysfs_fd = open(path, O_WRONLY);
			if(sysfs_fd < 0)
			{
				fprintf(stderr, "LEDs are Not supported! (Direction File Error!!!)\n");
				return;
			}
			if (write(sysfs_fd, "out", 3)<0)
			{
				fprintf(stderr, "Failed to set direction!\n");
				return;
			}
			close(sysfs_fd);

		}
		flags.led_existent = 1;
	}
}

static int GPIOWrite(int pin, int value)
{
	static const char s_values_str[] = "01";
	char path[30];
	int fd;
	snprintf(path, 30, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd)
	{
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}
	if (1 != write(fd, &s_values_str[!value ? 0 : 1], 1))
	{
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}
	close(fd);
	return(0);
}

void led_stat(struct Morfeas_SDAQ_if_stats *stats)
{
	static struct{
		unsigned Bus_util : 1;
		unsigned Max_dev_num : 1;
	}leds_status = {0};
	if(flags.led_existent)
	{
		if(stats->Bus_util>95.0)
		{
			GPIOWrite(YELLOW_LED, 1);
			leds_status.Bus_util = 1;
		}
		else if(stats->Bus_util<=80.0 && leds_status.Bus_util)
		{
			GPIOWrite(YELLOW_LED, 0);
			leds_status.Bus_util = 0;
		}

		if(stats->detected_SDAQs>=60)
		{
			GPIOWrite(RED_LED, 1);
			leds_status.Max_dev_num = 1;
		}
		else
		{
			if(leds_status.Max_dev_num)
			{
				GPIOWrite(RED_LED, 0);
				leds_status.Max_dev_num = 0;
			}
		}
	}
}

inline void quit_signal_handler(int signum)
{
	if(signum == SIGPIPE)
		fprintf(stderr,"IPC: Force Termination!!!\n");
	flags.run = 0;
	return;
}

inline void CAN_if_timer_handler (int signum)
{
	static unsigned char timer_ring_cnt = SYNC_INTERVAL;
	timer_ring_cnt--;
	if(!timer_ring_cnt)
	{
		unsigned short time_seed;
		struct timespec time_rep = {0,0};
		clock_gettime(CLOCK_MONOTONIC_RAW, &time_rep);
		time_seed = (time_rep.tv_nsec - tstart.tv_nsec)/1000000;
		time_seed += (time_rep.tv_sec - tstart.tv_sec)*1000;
		if(time_seed>=60000)
		{
			clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
			time_seed -= 60000;
		}
		// Clean up cycle trig
		if((time_seed%20000)<=100) //approximately every 20 sec
			flags.Clean_flag=1;//trig a clean up of list_SDAQ.
		//printf("timeseed = %hu\n",time_seed);
		//Send Synchronization with time_seed to all SDAQs
		Sync(CAN_socket_num, time_seed);
		timer_ring_cnt = SYNC_INTERVAL;//reset timer_ring_cnt
	}
	flags.bus_info = 1;
	return;
}

void print_usage(char *prog_name)
{
	const char preamp[] = {
	"Program: Morfeas_SDAQ_CAN_if  Copyright (C) 12019-12020  Sam Harry Tzavaras\n"
    "This program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
    "This is free software, and you are welcome to redistribute it\n"
    "under certain conditions; for details see LICENSE.\n"
	};
	const char exp[] = {
	"\tCAN-IF: The name of the CAN-Bus adapter\n\n"
	};
	printf("%s\nUsage: %s CAN-IF [/path/to/logstat/directory] \n\n%s\n", preamp, prog_name,exp);
	return;
}

/*Lists related function implementation*/
//SDAQ_info_entry allocator
struct SDAQ_info_entry* new_SDAQ_info_entry()
{
    struct SDAQ_info_entry *new_node = (struct SDAQ_info_entry *) g_slice_alloc0(sizeof(struct SDAQ_info_entry));
    return new_node;
}
//Channel_date_entry allocator
struct Channel_date_entry* new_SDAQ_Channel_date_entry()
{
    struct Channel_date_entry *new_node = (struct Channel_date_entry *) g_slice_alloc0(sizeof(struct Channel_date_entry));
    return new_node;
}
//LogBook_entry allocator
struct LogBook_entry* new_LogBook_entry()
{
    struct LogBook_entry *new_node = (struct LogBook_entry *) g_slice_alloc0(sizeof(struct LogBook_entry));
    return new_node;
}

//free a node from list SDAQ_Channels_cal_dates
void free_channel_cal_dates_entry(gpointer node)
{
    g_slice_free(struct Channel_date_entry, node);
}
//free a node from list SDAQ_info
void free_SDAQ_info_entry(gpointer node)
{
	struct SDAQ_info_entry *node_dec = node;
	g_slist_free_full(node_dec->SDAQ_Channels_cal_dates, free_channel_cal_dates_entry);
	g_slice_free(struct SDAQ_info_entry, node);
}
//free a node from List LogBook
void free_LogBook_entry(gpointer node)
{
	g_slice_free(struct LogBook_entry, node);
}

/*
	Comparing function used in g_slist_find_custom, comp arg address to node's Address.
*/
gint SDAQ_info_entry_find_address (gconstpointer node, gconstpointer arg)
{
	const unsigned char *arg_t = arg;
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *) node;
	return node_dec->SDAQ_address == *arg_t ? 0 : 1;
}

/*
	Comparing function used in g_slist_find_custom, comp arg S/N to node's S/N.
*/
gint SDAQ_info_entry_find_serial_number (gconstpointer node, gconstpointer arg)
{
	const unsigned int *arg_t = arg;
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *) node;
	return node_dec->SDAQ_status.dev_sn == *arg_t ? 0 : 1;
}

/*
	Comparing function used in g_slist_find_custom, comp arg S/N to node's S/N.
*/
gint LogBook_entry_find_serial_number (gconstpointer node, gconstpointer arg)
{
	const unsigned int *arg_t = arg;
	struct LogBook_entry *node_dec = (struct LogBook_entry *) node;
	return node_dec->SDAQ_sn == *arg_t ? 0 : 1;
}

/*
	Comparing function used in g_slist_find_custom, comp arg address to node's Address.
*/
gint LogBook_entry_find_address (gconstpointer node, gconstpointer arg)
{
	const unsigned char *arg_t = arg;
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *) node;
	return node_dec->SDAQ_address == *arg_t ? 0 : 1;
}

/*
	Comparing function used in g_slist_insert_sorted,
*/
gint SDAQ_info_entry_cmp (gconstpointer a, gconstpointer b)
{
	if(((struct SDAQ_info_entry *)a)->SDAQ_address != ((struct SDAQ_info_entry *)b)->SDAQ_address)
		return (((struct SDAQ_info_entry *)a)->SDAQ_address <= ((struct SDAQ_info_entry *)b)->SDAQ_address) ?  0 : 1;
	else
		return (((struct SDAQ_info_entry *)a)->SDAQ_status.dev_sn <= ((struct SDAQ_info_entry *)b)->SDAQ_status.dev_sn) ?  0 : 1;
}

//Logbook read and write from file;
void LogBook_file(struct Morfeas_SDAQ_if_stats *stats, char *read_write_or_append)
{
	FILE *fp;
	GSList *LogBook_node = stats->LogBook;
	struct LogBook_entry *node_data;
	size_t read_bytes = sizeof(struct LogBook_entry);
	if(!strcmp(read_write_or_append, "r"))
	{
		fp=fopen(stats->LogBook_file_path,read_write_or_append);
		if(fp)
		{
			while(read_bytes == sizeof(struct LogBook_entry))
			{
				if(!(node_data = new_LogBook_entry()))
				{
					fprintf(stderr,"Memory Error!!!\n");
					exit(EXIT_FAILURE);
				}
				read_bytes = fread(node_data, 1, sizeof(struct LogBook_entry), fp);
				if(read_bytes == sizeof(struct LogBook_entry))
					stats->LogBook = g_slist_append(stats->LogBook, node_data);
			}
			fclose(fp);
		}
	}
	else if(!strcmp(read_write_or_append, "w"))
	{
		if(LogBook_node)//check if list LogBook have elements
		{
			fp=fopen(stats->LogBook_file_path,read_write_or_append);
			if(fp)
			{
				//Store all the nodes of list LogBook in file
				while(LogBook_node)
				{
					node_data = LogBook_node->data;
					if(node_data)
						fwrite (node_data, 1, sizeof(struct LogBook_entry), fp);
					LogBook_node = LogBook_node -> next;//next node
				}
				fclose(fp);
			}
		}
	}
	else if(!strcmp(read_write_or_append, "a"))
	{
		if(stats->list_SDAQs)//check if list_SDAQs have elements
		{
			while(LogBook_node->next)
				LogBook_node = LogBook_node -> next;//next node
			fp=fopen(stats->LogBook_file_path,read_write_or_append);
			if(fp)
			{
				node_data = LogBook_node->data;
				//Store last node of list LogBook in file
				fwrite (node_data, 1, sizeof(struct LogBook_entry), fp);
				fclose(fp);
			}
		}
	}
}
//Function that find and return the amount of incomplete (incomplete info and/or dates) nodes.
int incomplete_SDAQs(struct Morfeas_SDAQ_if_stats *stats)
{
	unsigned int incomp_amount=0;
	GSList *list_SDAQs = stats->list_SDAQs;
	struct SDAQ_info_entry *node_data;
	while(list_SDAQs)
	{
		node_data = list_SDAQs->data;
		if(node_data->info_collection_status<3)
			incomp_amount++;
		list_SDAQs = list_SDAQs->next;
	}
	return incomp_amount;
}

short time_diff_cal(unsigned short dev_time, unsigned short ref_time)
{
	short ret = dev_time > ref_time ? dev_time - ref_time : ref_time - dev_time;
	if(ret<0)
		ret = 60000 - dev_time - ref_time;
	return ret;
}
/*Function for Updating Time_diff (from debugging message) of a SDAQ. Used in FSM*/
int update_Timediff(unsigned char address, sdaq_sync_debug_data *ts_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	IPC_message IPC_msg;
	GSList *list_node = NULL;
	struct SDAQ_info_entry *sdaq_node;
	if (stats->list_SDAQs)
	{
		list_node = g_slist_find_custom(stats->list_SDAQs, &address, SDAQ_info_entry_find_address);
		if(list_node)
		{
			sdaq_node = list_node->data;
			sdaq_node->Timediff = time_diff_cal(ts_dec->dev_time, ts_dec->ref_time);
			time(&(sdaq_node->last_seen));
			//Send timediff over IPC
			IPC_msg.SDAQ_timediff.IPC_msg_type = IPC_SDAQ_timediff;
			sprintf(IPC_msg.SDAQ_timediff.connected_to_BUS,"%s",stats->CAN_IF_name);
			IPC_msg.SDAQ_timediff.SDAQ_serial_number = sdaq_node->SDAQ_status.dev_sn;
			IPC_msg.SDAQ_timediff.Timediff = sdaq_node->Timediff;
			IPC_msg_TX(stats->FIFO_fd, &IPC_msg);
		}
		else
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/*
	Comparing function used in g_slist_find_custom, comp arg channel to node's channel.
*/
gint SDAQ_Channels_cal_dates_entry_find_address (gconstpointer node, gconstpointer arg)
{
	const unsigned char *arg_t = arg;
	struct Channel_date_entry *node_dec = (struct Channel_date_entry *) node;
	return node_dec->Channel == *arg_t ? 0 : 1;
}
/*
	Comparing function used in g_slist_insert_sorted,
*/
gint SDAQ_Channels_cal_dates_entry_cmp (gconstpointer a, gconstpointer b)
{
	return (((struct Channel_date_entry *)a)->Channel <= ((struct Channel_date_entry *)b)->Channel) ?  0 : 1;
}
/*Function for Updating "Device Info" of a SDAQ. Used in FSM*/
int update_info(unsigned char address, sdaq_info *info_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	IPC_message IPC_msg;
	GSList *list_node = NULL;
	struct SDAQ_info_entry *sdaq_node;
	if (stats->list_SDAQs)
	{
		list_node = g_slist_find_custom(stats->list_SDAQs, &address, SDAQ_info_entry_find_address);
		if(list_node)
		{
			sdaq_node = list_node->data;
			memcpy(&(sdaq_node->SDAQ_info), info_dec, sizeof(sdaq_info));
			time(&(sdaq_node->last_seen));
			sdaq_node->info_collection_status = 2;
			//Send info through IPC
			IPC_msg.SDAQ_info.IPC_msg_type = IPC_SDAQ_info;
			sprintf(IPC_msg.SDAQ_info.connected_to_BUS,"%s",stats->CAN_IF_name);
			IPC_msg.SDAQ_info.SDAQ_serial_number = sdaq_node->SDAQ_status.dev_sn;
			memcpy(&(IPC_msg.SDAQ_info.SDAQ_info_data), info_dec, sizeof(sdaq_info));
			IPC_msg_TX(stats->FIFO_fd, &IPC_msg);
		}
		else
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
//Function that found and return the status of a node from the list_SDAQ with SDAQ_address == address, Used in FSM
struct SDAQ_info_entry * find_SDAQ(unsigned char address, struct Morfeas_SDAQ_if_stats *stats)
{
	GSList *list_node = g_slist_find_custom(stats->list_SDAQs, &address, SDAQ_info_entry_find_address);
	if(list_node)
		return (struct SDAQ_info_entry*)(list_node->data);
	return NULL;
}
/*Function for Updating "Calibration Date" of a SDAQ's channel. Used in FSM*/
int add_update_channel_date(unsigned char address, unsigned char channel, sdaq_calibration_date *date_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	IPC_message IPC_msg;
	GSList *list_node = NULL, *date_list_node = NULL;
	struct SDAQ_info_entry *sdaq_node;
	struct Channel_date_entry *sdaq_Channels_cal_dates_node;
	if (stats->list_SDAQs)
	{
		list_node = g_slist_find_custom(stats->list_SDAQs, &address, SDAQ_info_entry_find_address);
		if(list_node)
		{
			sdaq_node = list_node->data;
			date_list_node = g_slist_find_custom(sdaq_node->SDAQ_Channels_cal_dates, &channel, SDAQ_Channels_cal_dates_entry_find_address);
			if(date_list_node)//channel is already in to the SDAQ_Channels_cal_dates list: Update CH_date.
			{
				sdaq_Channels_cal_dates_node = date_list_node->data;
				memcpy(&(sdaq_Channels_cal_dates_node->CH_date), date_dec, sizeof(sdaq_calibration_date));
			}
			else//Channel is not in the list
			{
				sdaq_Channels_cal_dates_node = new_SDAQ_Channel_date_entry();
				if(sdaq_Channels_cal_dates_node)
				{
					sdaq_Channels_cal_dates_node->Channel = channel;
					memcpy(&(sdaq_Channels_cal_dates_node->CH_date), date_dec, sizeof(sdaq_calibration_date));
					sdaq_node->SDAQ_Channels_cal_dates = g_slist_insert_sorted(sdaq_node->SDAQ_Channels_cal_dates,
																			   sdaq_Channels_cal_dates_node,
																			   SDAQ_Channels_cal_dates_entry_cmp);
				}
				else
				{
					fprintf(stderr,"Memory error!!!\n");
					exit(EXIT_FAILURE);
				}
			}
			time(&(sdaq_node->last_seen));
			//Send calibration data via IPC
			IPC_msg.SDAQ_cal_date.IPC_msg_type = IPC_SDAQ_cal_date;
			sprintf(IPC_msg.SDAQ_cal_date.connected_to_BUS,"%s",stats->CAN_IF_name);
			IPC_msg.SDAQ_cal_date.SDAQ_serial_number = sdaq_node->SDAQ_status.dev_sn;
			IPC_msg.SDAQ_cal_date.channel = channel;
			memcpy(&(IPC_msg.SDAQ_cal_date.SDAQ_cal_date), date_dec, sizeof(sdaq_calibration_date));
			IPC_msg_TX(stats->FIFO_fd, &IPC_msg);
			//if this is the last calibration date message, mark entry as "info complete"
			if(channel == sdaq_node->SDAQ_info.num_of_ch)
			{
				sdaq_node->info_collection_status = 3;
				return EXIT_SUCCESS;
			}
		}
		else
			return EXIT_FAILURE;
	}
	return EXIT_FAILURE;
}
//Function that add or refresh SDAQ to lists list_SDAQ and LogBook, called if status message received. Used in FSM
struct SDAQ_info_entry * add_or_refresh_SDAQ_to_lists(int socket_fd, sdaq_can_id *sdaq_id_dec, sdaq_status *status_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	unsigned char address_test;
	struct SDAQ_info_entry *list_SDAQ_node_data;
	struct LogBook_entry *LogBook_node_data;
	GSList *check_is_in_list_SDAQ = NULL, *check_is_in_LogBook =NULL;

	check_is_in_LogBook = g_slist_find_custom(stats->LogBook, &(status_dec->dev_sn), LogBook_entry_find_serial_number);
	check_is_in_list_SDAQ = g_slist_find_custom(stats->list_SDAQs, &(status_dec->dev_sn), SDAQ_info_entry_find_serial_number);
	if(check_is_in_list_SDAQ)//SDAQ is in list_SDAQ
	{
		list_SDAQ_node_data = check_is_in_list_SDAQ->data;
		time(&(list_SDAQ_node_data->last_seen));//update last_seen for the SDAQ entry
		if(list_SDAQ_node_data->SDAQ_address != sdaq_id_dec->device_addr)//if TRUE, set back to the node_data->SDAQ_address
			SetDeviceAddress(socket_fd, list_SDAQ_node_data->SDAQ_status.dev_sn, list_SDAQ_node_data->SDAQ_address);
		return list_SDAQ_node_data;
	}
	else if(check_is_in_LogBook)//SDAQ is not in list_SDAQ, but is recorded in LogBook (Old Known entry)
	{
		LogBook_node_data = check_is_in_LogBook->data;
		check_is_in_list_SDAQ = g_slist_find_custom(stats->list_SDAQs, &(LogBook_node_data->SDAQ_address), SDAQ_info_entry_find_address);
		if(!check_is_in_list_SDAQ)//If TRUE, make new entry to list_SDAQ with address from LogBook and then configured SDAQ
		{
			//Make new entry to list_SDAQ with address from LogBook
			list_SDAQ_node_data = new_SDAQ_info_entry();
			if(list_SDAQ_node_data)
			{
				list_SDAQ_node_data->SDAQ_address = LogBook_node_data->SDAQ_address;
				memcpy(&(list_SDAQ_node_data->SDAQ_status), status_dec, sizeof(sdaq_status));
				time(&(list_SDAQ_node_data->last_seen));
				stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, list_SDAQ_node_data, SDAQ_info_entry_cmp);
				stats->detected_SDAQs++;
				//Configure SDAQ with Address from LogBook
				SetDeviceAddress(socket_fd, status_dec->dev_sn, LogBook_node_data->SDAQ_address);
				return list_SDAQ_node_data;
			}
			else
			{
				fprintf(stderr,"Memory error!\n");
				exit(EXIT_FAILURE);
			}
		}
		else //Address from recorded on LogBook is currently used
		{
			//Try to find an available address
			for(address_test=1;address_test<Parking_address;address_test++)
			{
				if(!g_slist_find_custom(stats->list_SDAQs, &address_test, SDAQ_info_entry_find_address))
				{
					list_SDAQ_node_data = new_SDAQ_info_entry();
					if(list_SDAQ_node_data)
					{
						//Load SDAQ data on new list_SDAQ entry
						list_SDAQ_node_data->SDAQ_address = address_test;
						memcpy(&(list_SDAQ_node_data->SDAQ_status), status_dec, sizeof(sdaq_status));
						time(&(list_SDAQ_node_data->last_seen));
						stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, list_SDAQ_node_data, SDAQ_info_entry_cmp);
						//Update LogBook with new address
						LogBook_node_data->SDAQ_address = address_test;
						SetDeviceAddress(socket_fd, status_dec->dev_sn, address_test);
						LogBook_file(stats, "w");
						stats->detected_SDAQs++;
						return list_SDAQ_node_data;
					}
					else
					{
						fprintf(stderr,"Memory error!\n");
						exit(EXIT_FAILURE);
					}
				}
			}
			//if not any address available set SDAQ to park
			if(sdaq_id_dec->device_addr != Parking_address)
				SetDeviceAddress(socket_fd, status_dec->dev_sn, Parking_address);
			return NULL;
		}
	}
	else //completely unknown SDAQ
	{
		//check if the current address of the SDAQ is not conflict with any other in list_SDAQ, if not use it as it's pre addressed
		address_test = sdaq_id_dec->device_addr;
		if(g_slist_find_custom(stats->list_SDAQs, &address_test, SDAQ_info_entry_find_address) || address_test==Parking_address)
		{
			if(g_slist_length(stats->list_SDAQs)<Parking_address)
			{
				//Try to find an available address
				for(address_test=1;address_test<Parking_address;address_test++)
				{
					if(!g_slist_find_custom(stats->list_SDAQs, &address_test, SDAQ_info_entry_find_address))
					{
						list_SDAQ_node_data = new_SDAQ_info_entry();
						LogBook_node_data = new_LogBook_entry();
						if(list_SDAQ_node_data)
						{
							//Load SDAQ data on new list_SDAQ entry
							list_SDAQ_node_data->SDAQ_address = address_test;
							memcpy(&(list_SDAQ_node_data->SDAQ_status), status_dec, sizeof(sdaq_status));
							time(&(list_SDAQ_node_data->last_seen));
							stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, list_SDAQ_node_data, SDAQ_info_entry_cmp);
							//Update LogBook with new address
							LogBook_node_data->SDAQ_address = address_test;
							LogBook_node_data->SDAQ_sn = status_dec->dev_sn;
							stats->LogBook = g_slist_append(stats->LogBook, LogBook_node_data);
							LogBook_file(stats, "a");
							SetDeviceAddress(socket_fd, status_dec->dev_sn, address_test);
							stats->detected_SDAQs++;
							return list_SDAQ_node_data;
						}
						else
						{
							fprintf(stderr,"Memory error!\n");
							exit(EXIT_FAILURE);
						}
					}
				}
			}
			//if not any address available set SDAQ to park
			if(sdaq_id_dec->device_addr != Parking_address)
				SetDeviceAddress(socket_fd, status_dec->dev_sn, Parking_address);
			return 0;
		}
		else //register the pre-address SDAQ as it is
		{
			list_SDAQ_node_data = new_SDAQ_info_entry();
			LogBook_node_data = new_LogBook_entry();
			if(list_SDAQ_node_data && LogBook_node_data)
			{
				//Load SDAQ data on new list_SDAQ entry
				list_SDAQ_node_data->SDAQ_address = address_test;
				memcpy(&(list_SDAQ_node_data->SDAQ_status), status_dec, sizeof(sdaq_status));
				time(&(list_SDAQ_node_data->last_seen));
				stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, list_SDAQ_node_data, SDAQ_info_entry_cmp);
				//Update LogBook with new address
				LogBook_node_data->SDAQ_address = address_test;
				LogBook_node_data->SDAQ_sn = status_dec->dev_sn;
				stats->LogBook = g_slist_append(stats->LogBook, LogBook_node_data);
				LogBook_file(stats, "a");
				SetDeviceAddress(socket_fd, status_dec->dev_sn, address_test);
				stats->detected_SDAQs++;
				return list_SDAQ_node_data;
			}
			else
			{
				fprintf(stderr,"Memory error!\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}
//Function thet cleaning the list_SDAQ from dead entries
int clean_up_list_SDAQs(struct Morfeas_SDAQ_if_stats *stats)
{
	IPC_message IPC_msg;
	struct SDAQ_info_entry *sdaq_node;
	GSList *check_node = NULL;
	time_t now=time(NULL);
	if(stats->list_SDAQs)//check if list_SDAQs have elements
	{
		check_node = stats->list_SDAQs;
		//check for dead SDAQs
		while(check_node)
		{
			if(check_node->data)
			{
				sdaq_node = check_node->data;
				if((now - sdaq_node->last_seen) > LIFE_TIME)
				{
					stats->detected_SDAQs--;
					//Send info of the removed SDAQ through IPC
					IPC_msg.SDAQ_clean.IPC_msg_type = IPC_SDAQ_clean_up;
					sprintf(IPC_msg.SDAQ_clean.connected_to_BUS,"%s",stats->CAN_IF_name);
					IPC_msg.SDAQ_clean.SDAQ_serial_number = sdaq_node->SDAQ_status.dev_sn;
					IPC_msg.SDAQ_clean.t_amount = stats->detected_SDAQs;
					IPC_msg_TX(stats->FIFO_fd, &IPC_msg);
					//SDAQ free allocated memory operation
					free_SDAQ_info_entry(check_node->data);
					check_node->data = NULL;
				}
			}
			check_node = check_node -> next;//next node
		}
		//Delete empty nodes from the list
		stats->list_SDAQs = g_slist_remove_all(stats->list_SDAQs, NULL);
	}
	return EXIT_SUCCESS;
}

//Function for construction of message for registration or update of a SDAQ
int IPC_SDAQ_reg_update(int FIFO_fd, char connected_to_BUS[10], unsigned char address, sdaq_status *SDAQ_status, unsigned char amount)
{
	IPC_message IPC_reg_msg;
	//Send SDAQ registration over IPC
	IPC_reg_msg.SDAQ_reg_update.IPC_msg_type = IPC_SDAQ_register_or_update;
	memccpy(&(IPC_reg_msg.SDAQ_reg_update.connected_to_BUS), connected_to_BUS, '\0', 10);
	IPC_reg_msg.SDAQ_reg_update.connected_to_BUS[9] = '\0';
	IPC_reg_msg.SDAQ_reg_update.address = address;
	memcpy(&(IPC_reg_msg.SDAQ_reg_update.SDAQ_status), SDAQ_status,  sizeof(sdaq_status));
	IPC_reg_msg.SDAQ_reg_update.t_amount = amount;
	return IPC_msg_TX(FIFO_fd, &IPC_reg_msg);
}
