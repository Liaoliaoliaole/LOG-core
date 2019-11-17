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
#include "sdaq-worker/src/Modes.h"

//Morfeas_SDAQ-if struct
struct Morfeas_SDAQ_if_stats{
	char *CAN_IF_name;
	float Bus_util;
	unsigned char detected_SDAQs;
	unsigned char online_SDAQs;
};


//Global variables
struct timespec tstart;
int socket_num, run=1;

//Local function (declaration)
void CAN_if_timer_handler(int signum);
void quit_signal_handler(int signum);
void print_usage(char *prog_name);//print the usage manual


int main(int argc, char *argv[])
{
	//Variables for Socket CAN
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr;	
	struct can_filter RX_filter;
	sdaq_can_id *can_filter_enc;
	
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
	
	close(socket_num);
	printf("\n%s quiting...\n",argv[0]);
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

