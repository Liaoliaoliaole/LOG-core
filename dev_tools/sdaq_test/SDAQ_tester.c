#include "sdaq_drv.h"

//global variables
volatile unsigned char running=1;
volatile unsigned int i=0;
int socket_num;

struct can_frame frame_tx,frame_rx;

//application functions
//void logger(const char msg[]);

void sig_handler(int signo)
{
	if (signo == SIGINT)
	{  
		running=0;
	}
}


  
// A normal C function that is executed as a thread  
// when its name is specified in pthread_create() 
void * CAN_socket_RX(void *vargp) 
{ 
	while(running)
		i=read(socket_num, &frame_rx, sizeof(frame_rx));
	return NULL;
} 
   
int main(int argc, char *argv[])
{
	const int disable_loopback = 0;
	int row,col;
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr;	
	pthread_t CAN_socket_RX_Thread_id; 
	
	if(argc != 2)
	{
		printf("Argument Error\n");
		exit(1);
	}
	
	if (signal(SIGINT, sig_handler) == SIG_ERR)
	{
		printf("\ncan't catch Signals\n");
		exit(1);
	}
	
	
	if((socket_num = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
		perror("Error while opening socket");
		exit(1);
	}
	strcpy(ifr.ifr_name, argv[1]);
	if(ioctl(socket_num, SIOCGIFINDEX, &ifr))
	{
		printf("CANif does not exist or other error\n");
		exit(1);
	}
	
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	//Disable Loopback
	setsockopt(socket_num, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &disable_loopback, sizeof(disable_loopback)); 
	
	// Add timeout option to the CAN Socket
	tv.tv_sec = 20;
	tv.tv_usec = 0;
	setsockopt(socket_num, SOL_CAN_RAW, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	
	//bind CAN Socket to address
	if(bind(socket_num, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	{
		perror("Error in socket bind");
		exit(1);
	}
	
	pthread_create(&CAN_socket_RX_Thread_id, NULL, CAN_socket_RX, NULL); 
    //pthread_join(CAN_socket_RX_Thread_id, NULL); 
	
	
	Stop(socket_num, 0);
	SetDeviceAddress(socket_num,0xD7493B25,0);				 
	initscr();				/* start the curses mode */
	curs_set(0);
	while(running)
	{
		
		getmaxyx(stdscr,row,col);		/* get the number of rows and columns */
		mvprintw(row-2,0,"This screen has %d rows and %d columns\n",row,col);
		printw("Try resizing your window(if possible) and then run this program again");
		mvprintw(row/2,col/2,"%d",i);
		i++;
		refresh();
		sleep(1);
	}
	endwin();
	//QueryDeviceInfo(socket_num,0);
	return 0;
}
