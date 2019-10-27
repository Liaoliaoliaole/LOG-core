#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#define PROTOCOL_ID 0x35

typedef struct{
	unsigned flags : 3;//EFF/RTR/ERR flags
	unsigned priority : 3;
	unsigned protocol_id : 6;
	unsigned payload_type: 8;
	unsigned device_addr : 6;
	unsigned channel_num : 6;
}sdaq_can_identifier; 


int Start(int socket_fd,unsigned char dev_address);
int Stop(int socket_fd,unsigned char dev_address);
int Sync(int socket_fd,unsigned char dev_address, short time_seed);
int QueryDeviceInfo(int socket_fd,unsigned char dev_address);
int QueryCalibrationData(int socket_fd,unsigned char dev_address);
int WriteCalibrationDate(int socket_fd,unsigned char dev_address,time_t valid_until,unsigned char NumOfPoints);
//int WriteCalibrationPoints();
