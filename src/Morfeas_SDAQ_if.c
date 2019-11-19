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

//Header for cJSON
#include <cjson/cJSON.h>
//Include SDAQ Driver header
#include "sdaq-worker/src/SDAQ_drv.h"
//Include Functions implementation header
//#include "sdaq-worker/src/Modes.h"

//Morfeas_SDAQ-if struct
struct Morfeas_SDAQ_if_stats{
	char *CAN_IF_name;
	float Bus_util;
	unsigned char detected_SDAQs;// Amount of online SDAQ.
	unsigned char conflicts;// Amount of SDAQ with conflict addresses
	GSList *list_SDAQs;// List with SDAQ status, info and last seen timestamp.
};
// Data of list_SDAQs nodes
struct SDAQ_info_entry{
	unsigned char SDAQ_address;
	sdaq_status SDAQ_status;
	sdaq_info SDAQ_info;
	time_t last_seen;
};

//Global variables
struct timespec tstart;
int CAN_socket_num, run=1;

/* Local function (declaration)
 * Return value: EXIT_FAILURE(1) of failure or EXIT_SUCCESS(0) on success.
 */
void quit_signal_handler(int signum);//SIGINT handler function
void CAN_if_timer_handler(int signum);//sync timer handler function
void print_usage(char *prog_name);//print the usage manual
int find_SDAQs(int socket_fd, struct Morfeas_SDAQ_if_stats *stats);//discover the online SDAQs and load them on stats, used on start.
int autoconfig_full(int socket_fd, struct Morfeas_SDAQ_if_stats *stats);//Auto-configure all the online SDAQ, used on start.
/*Function for handle detection of new SDAQ on park. Used in FSM*/
int autoconfig_new_SDAQ(int socket_fd, unsigned int serial_number, struct Morfeas_SDAQ_if_stats *stats);
/*Function for handle detection of new SDAQ with non parking address. Used in FSM*/
int add_or_refresh_list_SDAQs(int socket_fd, unsigned char address, sdaq_status *status_dec, struct Morfeas_SDAQ_if_stats *stats);

//lists related functions
void free_SDAQ_info_entry(gpointer node);//used with g_slist_free_full to free the data of each node
void printf_SDAQentry(gpointer node, gpointer arg_pass);

int main(int argc, char *argv[])
{
	//Variables for Socket CAN
	int RX_bytes;
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr;
	struct can_frame frame_rx;
	struct can_filter RX_filter;
	sdaq_can_id *sdaq_id_dec;
	sdaq_status *status_dec = (sdaq_status *)frame_rx.data;
	/*
	sdaq_meas *meas_dec = (sdaq_meas *)frame_rx.data;
	sdaq_info *info_dec = (sdaq_info *)frame_rx.data;
	sdaq_sync_debug_data *ts_dec = (sdaq_sync_debug_data *)frame_rx.data;
	*/
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

		/*Actions on the bus*/
	//Stop any measuring activity on the bus
	Stop(CAN_socket_num,Broadcast);
	//find the online SDAQ
	find_SDAQs(CAN_socket_num, &stats);
	if(!autoconfig_full(CAN_socket_num, &stats))//make autoconfig for all SDAQ in park
		Start(CAN_socket_num,Broadcast);//start measurements on success
	//Initialize timer expired time
	memset (&timer, 0, sizeof(struct itimerval));
	timer.it_interval.tv_sec = 10;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = timer.it_interval.tv_sec;
	timer.it_value.tv_usec = timer.it_interval.tv_usec;
	//Get start time
	clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
	//start timer
	setitimer(ITIMER_REAL, &timer, NULL);

	printf("%s Status\n",argv[0]);
	printf("Connect to interface %s\n",argv[1]);
	printf("Initial search found %d devices\n",stats.detected_SDAQs);
	printf("With %d conflicts\n",stats.conflicts);
	g_slist_foreach(stats.list_SDAQs, printf_SDAQentry, NULL);
	printf("\n\n");

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
					if(sdaq_id_dec->device_addr == Parking_address)
					{
						printf("\n\nNew SDAQ found with S/N : %u\n",status_dec->dev_sn);
						autoconfig_new_SDAQ(CAN_socket_num, status_dec->dev_sn, &stats);
						printf("New SDAQ_list:\n");
						g_slist_foreach(stats.list_SDAQs, printf_SDAQentry, NULL);
						printf("Conflicts = %d\n",stats.conflicts);
						if(!stats.conflicts)
							Start(CAN_socket_num,Broadcast);
						else
							Stop(CAN_socket_num,Broadcast);
					}
					else
					{
						if(add_or_refresh_list_SDAQs(CAN_socket_num, sdaq_id_dec->device_addr, status_dec, &stats))
						{
							printf("\n\n SDAQ request update from address: %hhu with S/N : %u\n", sdaq_id_dec->device_addr, status_dec->dev_sn);
							printf("Updated SDAQ_list:\n");
							g_slist_foreach(stats.list_SDAQs, printf_SDAQentry, NULL);
						}
					}
					break;
				case Sync_Info:
					break;
				case Device_info:
					break;
				//case Calibration_Date:
			}
		}
	}

	g_slist_free_full(stats.list_SDAQs, free_SDAQ_info_entry);
	printf("\n%s quiting...\n",argv[0]);
	//Stop any measuring activity on the bus
	Stop(CAN_socket_num,Broadcast);
	close(CAN_socket_num);
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
	printf("%s\nUsage: %s CAN-IF \n\n%s\n", preamp, prog_name,exp);
	return;
}

//Lists related function implementation
struct SDAQ_info_entry* new_SDAQ_info_entry()
{
    struct SDAQ_info_entry *new_SDAQ = (struct SDAQ_info_entry *) g_slice_alloc(sizeof(struct SDAQ_info_entry));
    return new_SDAQ;
}
//free a SDAQ_info_entry_node
void free_SDAQ_info_entry(gpointer node)
{
    g_slice_free(struct SDAQ_info_entry, node);
}

/*
	Comparing function used in g_slist_find_custom, comp arg address to node's Address.
*/
gint SDAQ_info_entry_find_address (gconstpointer node, gconstpointer arg)
{
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *) node;
	const unsigned char *arg_t = arg;
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
	Comparing function used in g_slist_insert_sorted,
*/
gint SDAQ_info_entry_cmp (gconstpointer a, gconstpointer b)
{
	if(((struct SDAQ_info_entry *)a)->SDAQ_address != ((struct SDAQ_info_entry *)b)->SDAQ_address)
		return (((struct SDAQ_info_entry *)a)->SDAQ_address <= ((struct SDAQ_info_entry *)b)->SDAQ_address) ?  0 : 1;
	else
		return (((struct SDAQ_info_entry *)a)->SDAQ_status.dev_sn <= ((struct SDAQ_info_entry *)b)->SDAQ_status.dev_sn) ?  0 : 1;
}

void printf_SDAQentry(gpointer node, gpointer arg_pass)
{
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *)node;
	char str_address[12], *last_seen_date = ctime (&node_dec->last_seen);
	if(node_dec->SDAQ_address!=Parking_address)
		sprintf(str_address,"%d",node_dec->SDAQ_address);
	else
		sprintf(str_address,"in_Park");
	if(node)
    	printf("%13s with S/N: %010d at Address: %s last_seen @ %s",
												  dev_type_str[node_dec->SDAQ_status.dev_type],
												  node_dec->SDAQ_status.dev_sn,
												  str_address,
												  last_seen_date);
}

/*return a list with all the SDAQs on bus, sort by address*/
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
		/* Set Watch SocketCAN to see when it's available for reading. */
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
	/*
	 * This code check all the list_SDAQs for nodes that does not have address == addr_t
	 * The first available address loaded on the first unregistered (with parking) node of list_park
	 * The w_ptr used as indexing pointer on the list_Park
	 */
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

int autoconfig_new_SDAQ(int socket_fd, unsigned int serial_number ,struct Morfeas_SDAQ_if_stats *stats)
{
	struct SDAQ_info_entry *new_sdaq;
	GSList *t_lst = NULL, *conflict_lst =NULL;
	int RX_bytes;
	time_t start_of_loop, end_of_loop;
	//CAN-Bus Local variables
	struct can_frame frame_rx;
	sdaq_can_id *id_dec = (sdaq_can_id *)&(frame_rx.can_id);
	sdaq_status *status_dec = (sdaq_status *)frame_rx.data;
	/*
	 * The bellow code check the list_SDAQs for node with node_dec->SDAQ_status.dev_sn == serial_number, (result on t_lst)
	 * if t_lst isn't NULL, the data of the node is cleaned and then the node de-linked and frees it
	 * Otherwise handle the situation by make a new node for the new SDAQ.
	 */
	t_lst = g_slist_find_custom(stats->list_SDAQs, &serial_number, SDAQ_info_entry_find_serial_number);
	if(t_lst)
	{
		free_SDAQ_info_entry(t_lst->data);
		stats->list_SDAQs = g_slist_delete_link(stats->list_SDAQs, t_lst);
	}
	/*
	 * This code check all the list_SDAQs for nodes that does not have address == addr_t
	 * The first available address send it to the SDAQ with S/N == serial_number
	 */
	for(unsigned char addr_t=1;addr_t<Parking_address;addr_t++)
	{
		if(!g_slist_find_custom(stats->list_SDAQs,&addr_t,SDAQ_info_entry_find_address))
		{
			SetDeviceAddress(socket_fd, serial_number, addr_t);
			do{
				time(&start_of_loop);
					RX_bytes = read(socket_fd, &frame_rx, sizeof(frame_rx));
					if(RX_bytes==sizeof(frame_rx) &&
					   id_dec->payload_type == Device_status &&
					   id_dec->device_addr == addr_t)
					{
						stats->detected_SDAQs++;
						// set SDAQ info data
						new_sdaq = new_SDAQ_info_entry();
						new_sdaq->SDAQ_address = id_dec->device_addr;
						new_sdaq->SDAQ_status.dev_sn = status_dec->dev_sn;
						new_sdaq->SDAQ_status.status = status_dec->status;
						new_sdaq->SDAQ_status.dev_type = status_dec->dev_type;
						time(&(new_sdaq->last_seen));
						stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, new_sdaq, SDAQ_info_entry_cmp);
						conflict_lst = find_SDAQs_Conflicts(stats->list_SDAQs);
						stats->conflicts = g_slist_length(conflict_lst);
						g_slist_free(conflict_lst);
						return EXIT_SUCCESS;
					}
				time(&end_of_loop);
			}while((end_of_loop - start_of_loop)<3); //run for 3 seconds
		}
	}
	return EXIT_FAILURE;
}

int add_or_refresh_list_SDAQs(int socket_fd, unsigned char address, sdaq_status *status_dec, struct Morfeas_SDAQ_if_stats *stats)
{
	struct SDAQ_info_entry *new_sdaq;
	GSList *t_lst = NULL, *conflict_lst =NULL;


	t_lst = g_slist_find_custom(stats->list_SDAQs, &status_dec->dev_sn, SDAQ_info_entry_find_serial_number);
	if(t_lst)
	{
		new_sdaq = (struct SDAQ_info_entry *)t_lst->data;
		new_sdaq->SDAQ_address = address;
		memcpy(&new_sdaq->SDAQ_status, status_dec, sizeof(sdaq_status));
		time(&new_sdaq->last_seen);
		conflict_lst = find_SDAQs_Conflicts(stats->list_SDAQs);
		stats->conflicts = g_slist_length(conflict_lst);
		g_slist_free(conflict_lst);
		return EXIT_SUCCESS;
	}
	/*
	 * This code check all the list_SDAQs for nodes that does not have address == addr_t
	 * The first available address send it to the SDAQ with S/N == serial_number
	 *
	for(unsigned char addr_t=1;addr_t<Parking_address;addr_t++)
	{
		if(!g_slist_find_custom(stats->list_SDAQs,&addr_t,SDAQ_info_entry_find_address))
		{
			printf("New in list\n");
			SetDeviceAddress(socket_fd, serial_number, addr_t);
			do{
				time(&start_of_loop);
					RX_bytes = read(socket_fd, &frame_rx, sizeof(frame_rx));
					if(RX_bytes==sizeof(frame_rx) &&
					   id_dec->payload_type == Device_status &&
					   id_dec->device_addr == addr_t)
					{
						stats->detected_SDAQs++;
						// set SDAQ info data
						new_sdaq = new_SDAQ_info_entry();
						new_sdaq->SDAQ_address = id_dec->device_addr;
						new_sdaq->SDAQ_status.dev_sn = status_dec->dev_sn;
						new_sdaq->SDAQ_status.status = status_dec->status;
						new_sdaq->SDAQ_status.dev_type = status_dec->dev_type;
						time(&(new_sdaq->last_seen));
						stats->list_SDAQs = g_slist_insert_sorted(stats->list_SDAQs, new_sdaq, SDAQ_info_entry_cmp);
						conflict_lst = find_SDAQs_Conflicts(stats->list_SDAQs);
						stats->conflicts = g_slist_length(conflict_lst);
						g_slist_free(conflict_lst);
						return EXIT_SUCCESS;
					}
				time(&end_of_loop);
			}while((end_of_loop - start_of_loop)<3); //run for 3 seconds
		}
	}*/
	return EXIT_FAILURE;
}


