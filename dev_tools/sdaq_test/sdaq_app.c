#include "sdaq_drv.h"

//global variables


//application functions
void logger(const char msg[]);

int main(int argc, char *argv[])
{
	struct ifreq ifr;
	struct sockaddr_can addr;	
	int socket_num;
	
	if((socket_num = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
		perror("Error while opening socket");
		return -1;
	}
	strcpy(ifr.ifr_name, "can0");
	ioctl(socket_num, SIOCGIFINDEX, &ifr);
	
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	//Disable Loopback
	const int disable_loopback = 0;
	setsockopt(socket_num, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &disable_loopback, sizeof(disable_loopback)); 
	//bind CAN Socket
	if(bind(socket_num, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	{
		perror("Error in socket bind");
		return -2;
	}
	
	Start(socket_num, 0);
	sleep(10);
	Stop(socket_num, 0);
	//QueryDeviceInfo(socket_num,0);
	return 0;
}
