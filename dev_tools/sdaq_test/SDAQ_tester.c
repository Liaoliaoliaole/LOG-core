#define AVG_INTERVAL 1

#include "sdaq_drv.h"

struct thread_arguments_passer{
	int socket_num;
	unsigned char dev_addr;
	WINDOW *meas_win,*status_win,*info_win;
};

//global variables
volatile char running=1;
pthread_mutex_t display_access = PTHREAD_MUTEX_INITIALIZER;

//application functions
WINDOW *create_newwin(int height, int width, int starty, int startx);
//void logger(const char msg[]);

//threaded function. Act as CAN-bus message Receiver and decoder for SDAQ devices
void * CAN_socket_RX(void *varg_pt) 
{ 
	unsigned char avg_cnt,i,amount_of_inputs=1;
	float meas_value[16]={0.0};
	//passed arguments decoder
	struct thread_arguments_passer *arg = (struct thread_arguments_passer *) varg_pt; 
	//local variables for CAN Socket frame and SDAQ messages decoders
	struct can_frame frame_rx;
	int RX_bytes;
	sdaq_can_identifier *id_dec;
	sdaq_status *status_dec;
	sdaq_meas *meas_dec;
	sdaq_info *info_dec;
	
	while(running>0)
	{		
		RX_bytes=read(arg->socket_num, &frame_rx, sizeof(frame_rx));
		if(RX_bytes==sizeof(frame_rx))
		{
			id_dec = (sdaq_can_identifier *)&(frame_rx.can_id);
			if(arg->dev_addr==id_dec->device_addr)
			{
				pthread_mutex_lock(&display_access);
				switch(id_dec->payload_type)
				{
					case Measurement_value: 
						//wclear(arg->meas_win);
						meas_dec = (sdaq_meas *)frame_rx.data;
						mvwprintw(arg->meas_win,2,2,"Measurements:");
						if(!(meas_dec->status))
							meas_value[(id_dec->channel_num)-1]+=meas_dec->meas;
						else
							meas_value[(id_dec->channel_num)-1]=0.0;
						avg_cnt++;
						if (avg_cnt>=AVG_INTERVAL*amount_of_inputs)
						{
							for(i=0;i<amount_of_inputs;i++)
							{
								if(meas_value[i]!=0.0)
									mvwprintw(arg->meas_win,i+3,3,"CH%2d = %4.3f%s "
														,i,meas_value[i]/AVG_INTERVAL,unit_str[meas_dec->unit]);
								else
									mvwprintw(arg->meas_win,i+3,3,"CH%2d = No sensor ",i);
								meas_value[i]=0.0;
							}
							wrefresh(arg->meas_win);
							avg_cnt=0;
						}
						break;
					case Device_status: 
						//wclear(arg->status_win);
						status_dec = (sdaq_status *)frame_rx.data;
						mvwprintw(arg->status_win,2,2,"Device_status & S/N:"); 
						mvwprintw(arg->status_win,3,3,"Dev serial number = %d",status_dec->dev_sn);
						mvwprintw(arg->status_win,4,3,"Dev status = %d",status_dec->status);
						mvwprintw(arg->status_win,5,3,"Dev type = %s (%d)",dev_type_str[status_dec->device_type],
																		  status_dec->device_type);
						wrefresh(arg->status_win);
						if(!(status_dec->status & 0x01))
						{
							wclear(arg->meas_win);
							wrefresh(arg->meas_win);
						}
						break;
					case Device_info: 
						//wclear(arg->meas_win);
						info_dec = (sdaq_info *)frame_rx.data;
						mvwprintw(arg->info_win,2,2,"Device_info:");
						mvwprintw(arg->info_win,3,3,"Device type:(%d)",info_dec->dev_type);
						mvwprintw(arg->info_win,4,3,"Firmware rev:%d",info_dec->firm_rev);
						mvwprintw(arg->info_win,5,3,"Hardware rev:%d",info_dec->hw_rev);
						mvwprintw(arg->info_win,6,3,"Number of channels:%d",info_dec->num_of_ch);
						mvwprintw(arg->info_win,7,3,"Samplerate:%d sps",info_dec->sample_rate);
						wrefresh(arg->info_win);
						amount_of_inputs=info_dec->num_of_ch;
						break;
					/*
					case Calibration_Date: 
						//wclear(arg->meas_win); 
						mvwprintw(arg->meas_win,1,1,"Calibration_Date"); 
						wrefresh(arg->meas_win);
						break;
					*/
					default: break; 
				}
				refresh();
				pthread_mutex_unlock(&display_access);
			}
		}
		else
		{
			mvprintw(0,0,"Socket Timeout");
			refresh();
		}
	}
	return NULL;
} 
   
int main(int argc, char *argv[])
{
	//variables for ncurses
	int row,col,last_row=0,last_col=0;
	char user_pressed_key,dev_addr=0;
	//variables for Socket CAN
	struct timeval tv;
	struct ifreq ifr;
	struct sockaddr_can addr;	
	int socket_num;
	const int disable_loopback = 0;
	//variables for threads
	pthread_t CAN_socket_RX_Thread_id; 
	struct thread_arguments_passer thread_arg;
	
	if(argc != 3)
	{
		printf("Argument Error\n");
		exit(1);
	}
	dev_addr = atoi(argv[2]);
	if(dev_addr<1||dev_addr>=Parking_address)
	{
		printf("Device address Out of range\n");
		exit(1);
	}
	thread_arg.dev_addr=dev_addr;
	//CAN Socket Opening
	if((socket_num = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) 
	{
		perror("Error while opening socket");
		exit(1);
	}
	thread_arg.socket_num=socket_num;
	//link inerface name to socket
	strcpy(ifr.ifr_name, argv[1]); // get name from main arguments
	if(ioctl(socket_num, SIOCGIFINDEX, &ifr))
	{
		printf("CANBUS interface name does not exist\n");
		exit(1);
	}
	
	//Disable Loopback
	setsockopt(socket_num, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &disable_loopback, sizeof(disable_loopback)); 
	// Add timeout option to the CAN Socket
	tv.tv_sec = 25;
	tv.tv_usec = 0;
	setsockopt(socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	
	//bind CAN Socket to address
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if(bind(socket_num, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
	{
		perror("Error in socket bind");
		exit(1);
	}
	//Stop any measurements on CAN-bus
	Stop(socket_num, 0);				 
	
	//Init Measurement mode
	initscr(); // start the curses mode
	raw();//getch without return
	noecho();//disable echo
	cbreak();//exit on break
	curs_set(0);//hide cursor
	getmaxyx(stdscr,row,col);
	mvprintw(row-2,0,"Function Buttons:\n",row,col);
	printw("'q' quit 1 Start_Measuring 2 Stop_Measuring ");
	refresh();
	thread_arg.status_win = newwin(6,100, 0, 0);//create window for measurements
	thread_arg.info_win   = newwin(7,100, 6, 0);//create window for measurements
	thread_arg.meas_win   = newwin(17,100, 13, 0);//create window for measurements
	
	//mount the CAN-bus receiver on a thread, and load arguments 
	pthread_create(&CAN_socket_RX_Thread_id, NULL, CAN_socket_RX, &thread_arg);
	usleep(10000);
	QueryDeviceInfo(socket_num,dev_addr);
	while(running>0)
	{
		getmaxyx(stdscr,row,col);
		if(last_row!=row||last_col!=col)//reset display in case of terminal resize 
		{
			if(col<50&&row<50)//check if the terminal is smaller that the requirement 
				running = -1;
			else
			{
				pthread_mutex_lock(&display_access);
				clear();
				mvprintw(row-2,0,"Function Buttons:\n");
				printw("'q' quit 1 Start_Measuring 2 Stop_Measuring 3 Query Info ");
				refresh();
				pthread_mutex_unlock(&display_access);
				QueryDeviceInfo(socket_num,dev_addr);
				last_col=col;
				last_row=row;
			}
		}
		user_pressed_key=getch();// get the user's entrance 
		switch(user_pressed_key)
		{
			case '1': Start(socket_num,dev_addr); break;
			case '2': Stop(socket_num,dev_addr);break;
			case '3': QueryDeviceInfo(socket_num,dev_addr);break;
			case 'q': running=0; break;
			case 'C': last_row=last_col=0;
			default : break;
		}
	}
	endwin();
	if(running<0)
		printf("Terminal need to be at least %dx%d\n",30,100);
	//Stop any measurements on CAN-bus
	Stop(socket_num, 0);
	return 0;
}
