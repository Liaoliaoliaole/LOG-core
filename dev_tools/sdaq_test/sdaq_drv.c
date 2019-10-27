#include "sdaq_drv.h"

int Start(int socket_fd,unsigned char dev_address)
{
	

}
int Stop(int socket_fd,unsigned char dev_address);
int Sync(int socket_fd,unsigned char dev_address, short time_seed);
int QueryDeviceInfo(int socket_fd,unsigned char dev_address);
int QueryCalibrationData(int socket_fd,unsigned char dev_address);
int WriteCalibrationDate(int socket_fd,unsigned char dev_address,time_t valid_until,unsigned char NumOfPoints);
//int WriteCalibrationPoints();
