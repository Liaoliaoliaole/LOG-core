/*   
Program: SDAQ_worker. A controlling software for SDAQ-CAN Devices.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <gmodule.h>
#include <glib.h>
#include <sys/time.h>
#include <signal.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//Include SDAQ Driver header
#include "sdaq-worker/src/SDAQ_drv.h"
//Include Functions implementation header
//#include "sdaq-worker/src/Modes.h"

//Morfeas_SDAQ-if struct
struct Morfeas_SDAQ_if_stats{
	char *CAN_IF_name;
	float Bus_util;
	unsigned char detected_SDAQs;
	GSList *list_SDAQs;
};

struct SDAQ_info_entry{
	sdaq_status entry_status;
	sdaq_info entry_info;
};

//Global variables
struct timespec tstart;
int socket_num, run=1;

/*Local function (declaration)*/
void CAN_if_timer_handler(int signum);
void quit_signal_handler(int signum);
void print_usage(char *prog_name);//print the usage manual
int SDAQs_discover(int socket_fd, struct Morfeas_SDAQ_if_stats *stats);//discover the online SDAQs and load them on stats
//lists related functions
void free_SDAQ_info_entry(gpointer node);//used with g_slist_free_full to free the data of each node

int main(int argc, char *argv[])
{
	//Variables for Socket CAN
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr;	
	struct can_filter RX_filter;
	sdaq_can_id *can_filter_enc;
	//data_and stats of the Morfeas-SDAQ_IF
	struct Morfeas_SDAQ_if_stats stats = {0};
	//Timers related Variables
	struct itimerval timer;	
	
	if(argc == 1)
	{
		print_usage(argv[0]);
		exit(1);
	}

	//CAN Socket Opening
	if((socket_num = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
		perror("Error while opening socket");
		exit(EXIT_FAILURE);
	}
	//Link interface name to socket
	strcpy(ifr.ifr_name, argv[optind]); // get value from CAN-IF arguments
	if(ioctl(socket_num, SIOCGIFINDEX, &ifr))
	{
		perror("CAN-IF");
		exit(EXIT_FAILURE);
	}
	/*Filter for CAN messages	-- SocketCAN Filters act as: <received_can_id> & mask == can_id & mask*/
	//load filter's can_id member
	can_filter_enc = (sdaq_can_id *)&RX_filter.can_id;//Set encoder to filter.can_id
	memset(can_filter_enc, 0, sizeof(sdaq_can_id));
	can_filter_enc->flags = 4;//set the EFF
	can_filter_enc->protocol_id = PROTOCOL_ID; // Received Messages with protocol_id == PROTOCOL_ID
	can_filter_enc->payload_type = 0x80; //  Received Messages with payload_type & 0x80 == TRUE, aka Master <- SDAQ.
	//load filter's can_mask member
	can_filter_enc = (sdaq_can_id *)&RX_filter.can_mask; //Set encoder to filter.can_mask
	memset(can_filter_enc, 0, sizeof(sdaq_can_id));
	can_filter_enc->flags = 4;//Received only messages with extended ID (29bit)
	can_filter_enc->protocol_id = -1; // Protocol_id field marked for examination 
	can_filter_enc->payload_type = 0x80; // + The most significant bit of Payload_type field marked for examination.  	
	setsockopt(socket_num, SOL_CAN_RAW, CAN_RAW_FILTER, &RX_filter, sizeof(RX_filter));
	
	// Add timeout option to the CAN Socket
	tv.tv_sec = 20;//interval time that a SDAQ send a Status/ID frame.
	tv.tv_usec = 0;
	setsockopt(socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	
	//Bind CAN Socket to address
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if(bind(socket_num, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	{
		perror("Error in socket bind");
		exit(EXIT_FAILURE);
	}
	//Link signal SIGALRM to timer's handler
	signal(SIGALRM, CAN_if_timer_handler);
	//Link signal SIGINT to quit_signal_handler
	signal(SIGINT, quit_signal_handler);
	
		//Actions on the bus 
	//Stop any measuring activity on the bus
	Stop(socket_num,Broadcast);
	SDAQs_discover(socket_num, &stats);
	
	//Initialize timer expired time 
	memset (&timer, 0, sizeof(struct itimerval));
	timer.it_interval.tv_sec = 10;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = timer.it_interval.tv_sec;
	timer.it_value.tv_usec = timer.it_interval.tv_usec;
	setitimer (ITIMER_REAL, &timer, NULL);
	//Get start time 
	clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
	
	//FSM of Morfeas_SDAQ_if
	while(run)
	{
		sleep(1);
	}
	
	g_slist_free_full(stats.list_SDAQs, free_SDAQ_info_entry);
	printf("\n%s quiting...\n",argv[0]);
	close(socket_num);
	return EXIT_SUCCESS;
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
	printf("timeseed = %hu\n",time_seed);
	Sync(socket_num, time_seed);
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
	printf("%s\nUsage: %s CAN-IF \n\n%s\n", preamp, prog_name,exp);
	return;
}

//Lists related function implementation 
struct SDAQ_info_entry* new_SDAQ_info_entry()
{
    struct SDAQ_info_entry *new_SDAQ = (struct SDAQ_info_entry *) g_slice_alloc(sizeof(struct SDAQ_info_entry));
    return new_SDAQ;
}

void free_SDAQ_info_entry(gpointer SDAQentry_node) 
{
    g_slice_free(struct SDAQ_info_entry, SDAQentry_node);
}
   
/*
	Comparing function used in g_slist_find_custom, 
	Controlled by target switch variable. 
*/
gint SDAQentry_find (gconstpointer node, gconstpointer arg)
{
	const unsigned int *arg_t = arg;
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *) node;
	return node_dec->entry_status.dev_sn == (unsigned int) *arg_t ? 0 : 1; 
}

/*return a list with all the SDAQs on bus, sort by address*/
GSList * find_SDAQs(int socket_fd)
{
	//internal List with SDAQs
	GSList *ret_list=NULL;
	
	//CAN Socket and SDAQ related variables
	struct can_frame frame_rx;
	int RX_bytes;
	sdaq_can_id *id_dec = (sdaq_can_id *)&(frame_rx.can_id);
	sdaq_status *status_dec = (sdaq_status *)(frame_rx.data);
	sdaq_info *info_dec = (sdaq_info *)(frame_rx.data);
	//Time related Variables
	time_t proc_start, proc_after;
	//Variables for select
	int retval;
	struct timeval tv;
	fd_set ready_for_read;
	
	//take time before start
	time(&proc_start);
	//Query device info from every device
	QueryDeviceInfo(socket_fd,Broadcast);
	do{
		/* Set Watch SocketCAN to see when it's available for reading. */
		FD_ZERO(&ready_for_read); //init ready_for_read
		FD_SET(socket_fd, &ready_for_read); //link Socket_num with ready_for_read
		tv.tv_sec = 0;
		tv.tv_usec = 100000;//100ms	
		//wait socket_num to be ready for read, or expired after timeout
		retval = select(socket_fd+1, &ready_for_read, NULL, NULL, &tv);
		if(retval == -1)
		{
			perror("select()");
			exit(EXIT_FAILURE);
		}
		else if (retval)
		{
			RX_bytes = read(socket_fd, &frame_rx, sizeof(frame_rx));
			if(RX_bytes==sizeof(frame_rx))
			{
				switch(id_dec->payload_type)
				{
					case Device_status:	
						// check if node with same Serial number exist in the list. if no, do store.
						if(g_slist_find_custom(ret_list,(gconstpointer)&(status_dec->dev_sn),SDAQentry_find)==NULL)  
						{ 
							struct SDAQ_info_entry *new_sdaq = new_SDAQ_info_entry();
							if (new_sdaq) 
							{
								/*
								// set SDAQ info data
								new_sdaq->serial_number = status_dec->dev_sn;
								new_sdaq->address = id_dec->device_addr; 
								new_sdaq->dev_type = dev_type_str[status_dec->dev_type];
								//printf("New SDAQ discovered with %d at address : %d\n",new_sdaq->serial_number,new_sdaq->address);
								ret_list = g_slist_insert_sorted(ret_list, new_sdaq, SDAQentry_cmp);
								*/
							} 
							else
							{
								fprintf(stderr,"Memory error\n");
								exit(EXIT_FAILURE);
							}
						}
						break;
				}
			}
		}
		//take time after scan process
		time(&proc_after);
	}while((proc_after - proc_start) < 3); //work for 3 seconds
	return (GSList *) ret_list;
}

//
int SDAQs_discover(int socket_fd, struct Morfeas_SDAQ_if_stats *stats)
{
	return 0;
}


