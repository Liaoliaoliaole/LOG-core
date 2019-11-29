/*
Program: Morfeas-SDAQ-if. A controlling software for SDAQ-CAN Devices.
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

#define LIFE_TIME 15 // Value In seconds, define the time that a SDAQ_info_entry node defined as off-line and removed from the list

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>

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
#include "Types.h"
#include "Morfeas_JSON.h"

//Global variables
static struct timespec tstart;
static int CAN_socket_num, run=1, led_existent=0, Clean_flag=0;

/* Local function (declaration)
 * Return value: EXIT_FAILURE(1) of failure or EXIT_SUCCESS(0) on success. except of other notice
 */
void quit_signal_handler(int signum);//SIGINT handler function
void CAN_if_timer_handler(int signum);//sync timer handler function
void print_usage(char *prog_name);//print the usage manual

	/*Morfeas_SDAQ-if functions*/
//Error Warning Leds controlling function
void led_init();
void led_stat(struct Morfeas_SDAQ_if_stats *stats);
//function to clean-up list_SDAQs from non active SDAQ
int clean_up_list_SDAQs(struct Morfeas_SDAQ_if_stats *stats);
/*
//discover the online SDAQs and load them on stats, used on start.
int find_SDAQs(int socket_fd, struct Morfeas_SDAQ_if_stats *stats);
//Auto-configure all the online SDAQ, used on start.
int autoconfig_full(int socket_fd, struct Morfeas_SDAQ_if_stats *stats);
*/
// Function for handle detection of new SDAQ on park. Return the new_address of the SDAQ or -1 in failure
int autoconfig_new_SDAQ(int socket_fd, sdaq_status *status_dec, struct Morfeas_SDAQ_if_stats *stats);
/*Function for handle detection of new SDAQ with non parking address or to update status of already existed. Used in FSM*/
int add_or_refresh_list_SDAQs(int socket_fd, unsigned char address, sdaq_status *status_dec, struct Morfeas_SDAQ_if_stats *stats);
/*Function for Updating "Device Info" of a SDAQ. Used in FSM*/
int update_info(unsigned char address, sdaq_info *info_dec, struct Morfeas_SDAQ_if_stats *stats);
/*Function for Updating "Calibration Date" of a SDAQ's channel. Used in FSM*/
int add_update_channel_date(unsigned char address, unsigned char channel, sdaq_calibration_date *date_dec, struct Morfeas_SDAQ_if_stats *stats);
/*Function for Updating Time_diff (from debugging message) of a SDAQ. Used in FSM*/
int update_Timediff(unsigned char address, sdaq_sync_debug_data *ts_dec, struct Morfeas_SDAQ_if_stats *stats);

	/*GSList related functions*/
void free_SDAQ_info_entry(gpointer node);//used with g_slist_free_full to free the data of each node of list_SDAQs
void free_LogBook_entry(gpointer node);//used with g_slist_free_full to free the data of each node of list LogBook
void printf_SDAQentry(gpointer node, gpointer arg_pass);

int main(int argc, char *argv[])
{
	//Operational variables
	char *logstat_path = argv[2];
	int Stop_flag=0;//this used to block the Stop message spamming in case of conflict.
	unsigned char new_SDAQ_addr;
	//Variables for Socket CAN
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
	//sdaq_meas *meas_dec = (sdaq_meas *)frame_rx.data;

	//data_and stats of the Morfeas-SDAQ_IF
	struct Morfeas_SDAQ_if_stats stats = {0};
	//Timers related Variables
	struct itimerval timer;

	if(argc == 1)
	{
		print_usage(argv[0]);
		exit(1);
	}
	if(argv[2]==NULL)
		fprintf(stderr,"No logstat_path argument. Running without logstat\n");
	else
	{
		DIR* dir = opendir(logstat_path);
		if (dir)
			closedir(dir);
		else
		{
			fprintf(stderr,"logstat_path is invalid!\n");
			return EXIT_FAILURE;
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
	//Link signal SIGINT to quit_signal_handler
	signal(SIGINT, quit_signal_handler);
	//initialize the indication LEDs of the Morfeas-proto (sysfs implementation)
	led_init(stats.CAN_IF_name);

	//Load the LogBook file to LogBook List
	/*TO-DO: Code for this here*/

		/*Actions on the bus*/
	//Stop any measuring activity on the bus
	Stop(CAN_socket_num, Broadcast);
	/*
	//find the online SDAQ
	find_SDAQs(CAN_socket_num, &stats);
	if(!autoconfig_full(CAN_socket_num, &stats))//do autoconfig for all SDAQ in park, answering with 0 if no conflict
	{
		sleep(2);//if success, wait all SDAQ to reboot
		Start(CAN_socket_num, Broadcast);//request start of measurements on all SDAQ
		QueryDeviceInfo(CAN_socket_num, Broadcast); //request Device Info from all SDAQ
	}
	else
		Stop_flag = 1;
	led_stat(&stats);
	logstat_json(logstat_path,&stats);

	printf("%s Status\n",argv[0]);
	printf("Connect to interface %s\n",argv[1]);
	printf("Initial search found %d devices\n",stats.detected_SDAQs);
	printf("With %d conflicts\n",stats.conflicts);
	g_slist_foreach(stats.list_SDAQs, printf_SDAQentry, NULL);
	printf("\n\n");
	*/
	//Initialize Sync timer expired time
	memset (&timer, 0, sizeof(struct itimerval));
	timer.it_interval.tv_sec = 10;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = timer.it_interval.tv_sec;
	timer.it_value.tv_usec = timer.it_interval.tv_usec;
	//Get start time
	clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
	//start timer
	setitimer(ITIMER_REAL, &timer, NULL);

	//FSM of Morfeas_SDAQ_if
	sdaq_id_dec = (sdaq_can_id *)&(frame_rx.can_id);//point ID decoder to ID field from frame_rx
	while(run)
	{
		RX_bytes=read(CAN_socket_num, &frame_rx, sizeof(frame_rx));
		if(RX_bytes==sizeof(frame_rx))
		{
			switch(sdaq_id_dec->payload_type)
			{
				case Measurement_value:
					break;
				case Device_status:
					clean_up_list_SDAQs(&stats);//clean up dead SDAQs
					if(sdaq_id_dec->device_addr == Parking_address) // message from parked SDAQ
					{
						new_SDAQ_addr = autoconfig_new_SDAQ(CAN_socket_num, status_dec, &stats);
						if(!stats.conflicts)
						{
							Stop_flag = 0;
							if(new_SDAQ_addr && new_SDAQ_addr < Parking_address)
							{
								Start(CAN_socket_num,new_SDAQ_addr);
								QueryDeviceInfo(CAN_socket_num, new_SDAQ_addr);
							}
						}
						else
						{
							Stop_flag = 1;
							Stop(CAN_socket_num,Broadcast);
						}
						led_stat(&stats);
						logstat_json(logstat_path,&stats);

						printf("\n\t\tOperation: Register new SDAQ with S/N : %u\n",status_dec->dev_sn);
						printf("New SDAQ_list:\n");
						g_slist_foreach(stats.list_SDAQs, printf_SDAQentry, NULL);
						printf("Amount of in list SDAQ %d\n",stats.detected_SDAQs);
						printf("\n\n");
					}
					else //message from pre-addressed SDAQ
					{
						if(!add_or_refresh_list_SDAQs(CAN_socket_num, sdaq_id_dec->device_addr, status_dec, &stats)) // Answer 0 if no conflict
						{
							if(!(status_dec->status & 1<<State)) //SDAQ of sdaq_id_dec->device_addr not measure
							{
								Start(CAN_socket_num,sdaq_id_dec->device_addr); // put SDAQ of sdaq_id_dec->device_addr on measure
								QueryDeviceInfo(CAN_socket_num, sdaq_id_dec->device_addr);
								Stop_flag = 0;
							}
							led_stat(&stats);
							logstat_json(logstat_path,&stats);
						}
						else// conflict detected
						{
							if(!Stop_flag)
							{
								Stop(CAN_socket_num,Broadcast);
								Stop_flag = 1;

								printf("\t\tOperation: Stop measure due to address conflict\n");
								printf("Conflicts = %d\n",stats.conflicts);
								printf("SDAQ_list:\n");
								g_slist_foreach(stats.list_SDAQs, printf_SDAQentry, NULL);
								printf("Amount of in list SDAQ %d\n",stats.detected_SDAQs);
								printf("\n\n");
							}
							led_stat(&stats);
							logstat_json(logstat_path,&stats);
						}
					}
					break;
				case Sync_Info:
					if(!update_Timediff(sdaq_id_dec->device_addr, ts_dec, &stats))
						logstat_json(logstat_path,&stats);
					break;
				case Device_info:
					if(!update_info(sdaq_id_dec->device_addr, info_dec, &stats))
						logstat_json(logstat_path,&stats);
					break;
				case Calibration_Date:
					if(!add_update_channel_date(sdaq_id_dec->device_addr, sdaq_id_dec->channel_num, date_dec, &stats))
						logstat_json(logstat_path,&stats);
					break;
			}
		}
		if(Clean_flag)
		{
			clean_up_list_SDAQs(&stats);
			led_stat(&stats);
			logstat_json(logstat_path,&stats);
			Clean_flag = 0;

			printf("\t\tOperation: Clean Up\n");
			printf("Conflicts = %d\n",stats.conflicts);
			printf("SDAQ_list:\n");
			g_slist_foreach(stats.list_SDAQs, printf_SDAQentry, NULL);
			printf("Amount of in list SDAQ %d\n",stats.detected_SDAQs);
			printf("\n\n");
		}
	}
	printf("\nExiting...\n");
	//free all lists
	g_slist_free_full(stats.list_SDAQs, free_SDAQ_info_entry);
	g_slist_free_full(stats.LogBook, free_LogBook_entry);
	//Stop any measuring activity on the bus
	Stop(CAN_socket_num,Broadcast);
	close(CAN_socket_num);
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
				fprintf(stderr, "LEDs are Not supported! (direction)\n");
				return;
			}
			if (write(sysfs_fd, "out", 3)<0)
			{
				fprintf(stderr, "Failed to set direction!\n");
				return;
			}
			close(sysfs_fd);

		}
		led_existent = 1;
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
	if(led_existent)
	{
		if(!strcmp(stats->CAN_IF_name,"can0"))
			!stats->conflicts ? GPIOWrite(RED_LED, 0) : GPIOWrite(RED_LED, 1);
		else if(!strcmp(stats->CAN_IF_name,"can1"))
			!stats->conflicts ? GPIOWrite(YELLOW_LED, 0) : GPIOWrite(YELLOW_LED, 1);
	}
}

void quit_signal_handler(int signum)
{
	run = 0;
	return;
}

void CAN_if_timer_handler (int signum)
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
		Clean_flag=1;//trig a clean up of list_SDAQ.
	//printf("timeseed = %hu\n",time_seed);
	//Send Synchronization with time_seed to all SDAQs
	Sync(CAN_socket_num, time_seed);
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
    struct SDAQ_info_entry *new_node = (struct SDAQ_info_entry *) g_slice_alloc(sizeof(struct SDAQ_info_entry));
    return new_node;
}
//Channel_date_entry allocator
struct Channel_date_entry* new_SDAQ_Channel_date_entry()
{
    struct Channel_date_entry *new_node = (struct Channel_date_entry *) g_slice_alloc(sizeof(struct Channel_date_entry));
    return new_node;
}
//LogBook_entry allocator
struct LogBook_entry* new_LogBook_entry()
{
    struct LogBook_entry *new_node = (struct LogBook_entry *) g_slice_alloc(sizeof(struct LogBook_entry));
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

//This function will be removed is only for debugging
void printf_SDAQentry(gpointer node, gpointer arg_pass)
{
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *)node;
	char str_address[12];
	if(node_dec->SDAQ_address!=Parking_address)
		sprintf(str_address,"%d",node_dec->SDAQ_address);
	else
		sprintf(str_address,"in_Park");
	if(node)
		printf("%13s with S/N: %010d at Address: %2s last_seen  %7.3f sec ago\n",
												  dev_type_str[node_dec->SDAQ_status.dev_type],
												  node_dec->SDAQ_status.dev_sn,
												  str_address,
												  difftime(time(NULL), node_dec->last_seen));
}

/*//return a list with all the SDAQs on bus, sort by address
int find_SDAQs(int socket_fd, struct Morfeas_SDAQ_if_stats *stats)
{
	//internal List with SDAQs
	GSList *search_list=stats->list_SDAQs=NULL;
	//CAN Socket and SDAQ related variables
	struct can_frame frame_rx;
	int RX_bytes;
	sdaq_can_id *id_dec = (sdaq_can_id *)&(frame_rx.can_id);
	sdaq_status *status_dec = (sdaq_status *)(frame_rx.data);
	//Time related Variables
	time_t proc_start, proc_after;
	//Variables for select
	int retval;
	struct timeval tv;
	fd_set ready;

	//take time before start
	time(&proc_start);
	//Query device info from every device
	QueryDeviceInfo(socket_fd,Broadcast);
	do{
		// Set Watch SocketCAN to see when it's available for reading.
		FD_ZERO(&ready); //init ready
		FD_SET(socket_fd, &ready); //link Socket_num with ready
		tv.tv_sec = 0;
		tv.tv_usec = 100000;//100ms
		//wait socket_num to be ready for read, or expired after timeout
		retval = select(socket_fd+1, &ready, NULL, NULL, &tv);
		if(retval == -1)
		{
			perror("select()");
			exit(EXIT_FAILURE);
		}
		else if (retval)
		{
			RX_bytes = read(socket_fd, &frame_rx, sizeof(frame_rx));
			if(RX_bytes==sizeof(frame_rx) && id_dec->payload_type == Device_status)
			{
				// check if node with same Serial number exist in the search_list. if no, do store.
				if(g_slist_find_custom(search_list,(gconstpointer)&(status_dec->dev_sn),SDAQ_info_entry_find_serial_number)==NULL)
				{
					struct SDAQ_info_entry *new_sdaq = new_SDAQ_info_entry();
					if (new_sdaq)
					{
						stats->detected_SDAQs++;
						// set SDAQ info data
						new_sdaq->SDAQ_address = id_dec->device_addr;
						new_sdaq->SDAQ_status.dev_sn = status_dec->dev_sn;
						new_sdaq->SDAQ_status.status = status_dec->status;
						new_sdaq->SDAQ_status.dev_type = status_dec->dev_type;
						time(&(new_sdaq->last_seen));
						search_list = g_slist_insert_sorted(search_list, new_sdaq, SDAQ_info_entry_cmp);
					}
					else
					{
						fprintf(stderr,"Memory error\n");
						exit(EXIT_FAILURE);
					}
				}
			}
		}
		//take time after scan process
		time(&proc_after);
	}while((proc_after - proc_start) < 3); //work for 3 seconds
	stats->list_SDAQs = search_list;
	return EXIT_SUCCESS;
}
*/

/*return a list with all the SDAQs nodes (from head) that have Parking address, sort by Serial number*/
GSList* find_SDAQs_inParking(GSList *head)
{
	GSList *t_lst = head, *ret_list=NULL;
	for(int i=g_slist_length(head);i;i--)//Run for all head nodes.
	{
		if(t_lst)
		{
			//look at the list t_lst (aka head, at first) for entrance with parking address
			t_lst = g_slist_find_custom(t_lst,(gconstpointer) &(Parking_address),SDAQ_info_entry_find_address);
			if(t_lst)
			{
				ret_list = g_slist_insert_sorted(ret_list, (gpointer) t_lst->data, SDAQ_info_entry_cmp);//sort by serial number
				t_lst = t_lst->next; //goto next node
			}
			else
				break; //Break the for loop if no Parking_address node found.
		}
		else
			break;  //Break the for loop if end of list is reached.
	}
	return ret_list;
}

/*return a list with all the SDAQs nodes (from head) that have same address (aka conflict)*/
GSList * find_SDAQs_Conflicts(GSList * head)
{
	GSList *ret_list=NULL; // function's return pointer
	volatile GSList *look_ptr, *start_ptr = head; //start_ptr pointer pointing the first node on list.
	unsigned char cur_address=0;
	if(g_slist_length(head)>1)
	{
		//Place start_ptr pointer at first SDAQ list node that does not have parking address
		while(start_ptr->next && ((((struct SDAQ_info_entry *)(start_ptr->data))->SDAQ_address)==Parking_address))
			start_ptr = start_ptr->next; //move start_ptr to then next node
		while(start_ptr->next)//Run until start_ptr pointer be at the end node of the list.
		{
			ret_list = g_slist_append (ret_list, (gpointer) start_ptr->data); //append node that looked by start pointer in ret_list, as a possible conflict.
			look_ptr = start_ptr->next;//look_ptr pointer pointing the next node after the start_ptr
			while(look_ptr)//Run until look_ptr pointer be NULL. aka, pass from the last node.
			{
				//Check if the address field of the start_ptr node is the same with the node that point the look_ptr. aka if true, conflict found.
				if(((struct SDAQ_info_entry *)(start_ptr->data))->SDAQ_address == ((struct SDAQ_info_entry *)(look_ptr->data))->SDAQ_address)
				{
					ret_list = g_slist_append(ret_list, look_ptr->data);
					cur_address = (((struct SDAQ_info_entry *)(look_ptr->data))->SDAQ_address);
				}
				//Avoid, look_ptr points nodes with Parking address
				do{
					look_ptr = look_ptr->next; //move look_ptr pointer to next node
				}while(look_ptr && (((struct SDAQ_info_entry *)(look_ptr->data))->SDAQ_address)==Parking_address);
			}
			//delete last appending on ret_list if does not have conflict address. aka above check with look_ptr give false.
			if(g_slist_last(ret_list)->data == start_ptr->data)
				ret_list = g_slist_delete_link(ret_list,g_slist_last(ret_list));
			//Avoid, start_ptr points nodes with already checked address and nodes with Parking address
			do{
				start_ptr = start_ptr->next;//move start_ptr to then next node
			}while(start_ptr->next && (((((struct SDAQ_info_entry *)(start_ptr->data))->SDAQ_address)==cur_address)
							   ||  ((((struct SDAQ_info_entry *)(start_ptr->data))->SDAQ_address)==Parking_address)));
		}
	}
	return ret_list;
}

// GFunc function used with g_slist_foreach. Arguments: SDAQ_new_address_list, pointer to socket number
void send_newaddress_to_SDAQs(gpointer node, gpointer arg_pass)
{
	//Configure with new address with arguments from the SDAQentry Node
	int socket_fd = *((int*)arg_pass);
	unsigned int serial_number = ((struct SDAQ_info_entry *) node)->SDAQ_status.dev_sn;
	unsigned char new_address = ((struct SDAQ_info_entry *) node)->SDAQ_address;
	SetDeviceAddress(socket_fd, serial_number, new_address);
	return;
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
	GSList *list_SDAQs = stats->list_SDAQs, *list_node = NULL;
	struct SDAQ_info_entry *sdaq_node;
	if (list_SDAQs)
	{
		list_node = g_slist_find_custom(list_SDAQs, &address, SDAQ_info_entry_find_address);
		if(list_node)
		{
			sdaq_node = list_node->data;
			sdaq_node->Timediff = time_diff_cal(ts_dec->dev_time, ts_dec->ref_time);
			time(&(sdaq_node->last_seen));
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
/*Function for Updating "Calibration Date" of a SDAQ's channel. Used in FSM*/
int add_update_channel_date(unsigned char address, unsigned char channel, sdaq_calibration_date *date_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	GSList *list_SDAQs = stats->list_SDAQs, *list_node = NULL, *date_list_node = NULL;
	struct SDAQ_info_entry *sdaq_node;
	struct Channel_date_entry *sdaq_Channels_cal_dates_node;

	if (list_SDAQs)
	{
		list_node = g_slist_find_custom(list_SDAQs, &address, SDAQ_info_entry_find_address);
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
		}
		else
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/*Function for Updating "Device Info" of a SDAQ. Used in FSM*/
int update_info(unsigned char address, sdaq_info *info_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	GSList *list_SDAQs = stats->list_SDAQs, *list_node = NULL;
	struct SDAQ_info_entry *sdaq_node;
	if (list_SDAQs)
	{
		list_node = g_slist_find_custom(list_SDAQs, &address, SDAQ_info_entry_find_address);
		if(list_node)
		{
			sdaq_node = list_node->data;
			memcpy(&sdaq_node->SDAQ_info, info_dec, sizeof(sdaq_info));
			time(&(sdaq_node->last_seen));
		}
		else
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/*
//autoconfig all the online SDAQ
int autoconfig_full(int socket_fd, struct Morfeas_SDAQ_if_stats *stats)
{
	unsigned char addr_t=1;
	int ret_val=EXIT_SUCCESS;
	GSList *list_SDAQs = stats->list_SDAQs;
	GSList *list_Park=NULL, *list_conflicts=NULL, *w_ptr=NULL;//list_work used as element pointer in addressing, and as list in verification.
	if (list_SDAQs)
	{	//build list_conflicts with all the nodes from list_SDAQs that have same address (Conflict)
		list_conflicts=find_SDAQs_Conflicts(list_SDAQs);
		if(list_conflicts) //Check for conflicts
		{
			stats->conflicts = g_slist_length(list_conflicts);
			g_slist_free(list_conflicts);
			return EXIT_FAILURE;
		}
		//build list_Park with SDAQs_nodes from list_SDAQs that are in Parking
		list_Park = find_SDAQs_inParking(list_SDAQs);
		if(!list_Park)//Check for no Parking SDAQs, if no dev with park then return 0
			return EXIT_SUCCESS;

	 // This code check all the list_SDAQs for nodes that does not have address == addr_t
	 // The first available address loaded on the first unregistered (with parking) node of list_park
	 // The w_ptr used as indexing pointer on the list_Park

		w_ptr = list_Park;
		while(w_ptr)
		{
			if(!g_slist_find_custom(list_SDAQs,&addr_t,SDAQ_info_entry_find_address))
			{
				((struct SDAQ_info_entry *)w_ptr->data)->SDAQ_address=addr_t;
				w_ptr = w_ptr -> next;//next node with parking address
			}
			addr_t++;
		}
		//Send the new addresses to Parked SDAQs
		g_slist_foreach(list_Park, send_newaddress_to_SDAQs, &socket_fd);
		//Sort the list by address
		stats->list_SDAQs = g_slist_sort(stats->list_SDAQs, SDAQ_info_entry_cmp);
		g_slist_free(list_Park);
	}
	return ret_val;
}
*/
// autoconfigure a new_SDAQ
int autoconfig_new_SDAQ(int socket_fd, sdaq_status *status_dec ,struct Morfeas_SDAQ_if_stats *stats)
{
	struct SDAQ_info_entry *sdaq_node;
	GSList *t_lst = NULL, *conflict_lst =NULL, *check_node = NULL;
	printf("pass -1 %d\n",status_dec->dev_sn);
	// The bellow code check the list_SDAQs for node with node_dec->SDAQ_status.dev_sn == serial_number, (result on t_lst)
	t_lst = g_slist_find_custom(stats->list_SDAQs, &(status_dec->dev_sn), SDAQ_info_entry_find_serial_number);
	//If SDAQ is in the list : configure it with the previous and update last_seen
	if(t_lst && !stats->conflicts)
	{
		sdaq_node = t_lst->data;
		time(&(sdaq_node->last_seen));
		SetDeviceAddress(socket_fd, status_dec->dev_sn, sdaq_node->SDAQ_address);
	}
	else // Otherwise, find a available number, configure it and add it to the list.
	{
		conflict_lst = find_SDAQs_Conflicts(stats->list_SDAQs);
		if(t_lst && stats->conflicts)
		{
			check_node = g_slist_find_custom(conflict_lst, &status_dec->dev_sn, SDAQ_info_entry_find_serial_number);
			if(check_node)
			{
				stats->detected_SDAQs--;
				stats->list_SDAQs = g_slist_delete_link(stats->list_SDAQs, g_slist_find(stats->list_SDAQs, check_node->data));
				free_SDAQ_info_entry(check_node->data);
				conflict_lst = g_slist_delete_link(conflict_lst, check_node);
			}
		}
		for(unsigned char addr_t=1;addr_t<Parking_address;addr_t++)
		{
			if(!g_slist_find_custom(stats->list_SDAQs,&addr_t,SDAQ_info_entry_find_address))
			{
				// set SDAQ info data
				sdaq_node = new_SDAQ_info_entry();
				if(sdaq_node)
				{
					sdaq_node->SDAQ_address = addr_t;
					memcpy(&sdaq_node->SDAQ_status, status_dec, sizeof(sdaq_status));
					time(&(sdaq_node->last_seen));
					stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, sdaq_node, SDAQ_info_entry_cmp);
					SetDeviceAddress(socket_fd, status_dec->dev_sn, addr_t);
					stats->detected_SDAQs++;
					//update number of conflict
					stats->conflicts = g_slist_length(conflict_lst);
					g_slist_free(conflict_lst);
					return addr_t;
				}
				else
				{
					fprintf(stderr,"Memory error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	g_slist_free(conflict_lst);
	return -1;
}

int add_or_refresh_list_SDAQs(int socket_fd, unsigned char address, sdaq_status *status_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	GSList *t_lst = NULL, *conflict_lst =NULL;
	struct SDAQ_info_entry *sdaq_node;
	//printf("SDAQ_report (S/N:%d)(Addr:%d)\n",status_dec->dev_sn ,address);
	/*Check if SDAQ is in the list*/
	t_lst = g_slist_find_custom(stats->list_SDAQs, &status_dec->dev_sn, SDAQ_info_entry_find_serial_number);
	if(t_lst) // if SDAQ is in the list, act accordingly.
	{
		sdaq_node = t_lst->data;
		if(sdaq_node->SDAQ_address != address) // in case that have different address.
			SetDeviceAddress(socket_fd, sdaq_node->SDAQ_status.dev_sn, sdaq_node->SDAQ_address); //restore it to the previous from list
			//printf("Force addressing!!!! (S/N:%d)(old_Addr:%d)(new_addr:%d)\n",sdaq_node->SDAQ_status.dev_sn ,address ,sdaq_node->SDAQ_address);
		time(&(sdaq_node->last_seen));
	}
	else // If SDAQ is not in to the list : New entry
	{
		stats->detected_SDAQs++;
		// set SDAQ info data
		sdaq_node = new_SDAQ_info_entry();
		sdaq_node->SDAQ_address = address;
		memcpy(&sdaq_node->SDAQ_status, status_dec, sizeof(sdaq_status));
		time(&(sdaq_node->last_seen));
		stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, sdaq_node, SDAQ_info_entry_cmp);
	}
	conflict_lst = find_SDAQs_Conflicts(stats->list_SDAQs);
	stats->conflicts = g_slist_length(conflict_lst);
	g_slist_free(conflict_lst);
	return !stats->conflicts ? EXIT_SUCCESS : EXIT_FAILURE;
}


int clean_up_list_SDAQs(struct Morfeas_SDAQ_if_stats *stats)
{
	GSList *check_node = NULL, *conflict_lst =NULL;
	time_t now=time(NULL), last_seen;
	if(stats->list_SDAQs)//check if list_SDAQs have elements
	{
		check_node = stats->list_SDAQs;
		//check for dead SDAQs
		while(check_node)
		{
			if(check_node->data)	
			{
				last_seen = ((struct SDAQ_info_entry *)check_node->data)->last_seen;
				if((now - last_seen) > LIFE_TIME)
				{
					stats->detected_SDAQs--;
					free_SDAQ_info_entry(check_node->data);
					check_node->data = NULL;
				}
			}
			check_node = check_node -> next;//next node
		}
		stats->list_SDAQs = g_slist_remove_all(stats->list_SDAQs, NULL);
		/*
		check_node = t_lst;
		for(i=)
		{
			printf("Cleanup operation i = %ld\n",i++);
			if(check_node)
			{
				printf("Check is valid\n");
				last_seen = ((struct SDAQ_info_entry *)check_node->data)->last_seen;
				if((now - last_seen) > LIFE_TIME)
				{
					stats->detected_SDAQs--;
					free_SDAQ_info_entry(check_node->data);
					stats->list_SDAQs = g_slist_delete_link(stats->list_SDAQs, check_node);
				}
			}
			check_node = check_node -> next;//next node
		}
		*/
		//Check for conflicts
		if(stats->detected_SDAQs)
		{
			conflict_lst = find_SDAQs_Conflicts(stats->list_SDAQs);
			stats->conflicts = g_slist_length(conflict_lst);
			g_slist_free(conflict_lst);
		}
		else
			stats->conflicts = 0;
	}
	return !stats->conflicts ? EXIT_SUCCESS : EXIT_FAILURE;
}
