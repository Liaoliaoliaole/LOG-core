#include "sdaq_drv.h"

/*
typedef struct{
	unsigned flags : 3;//EFF/RTR/ERR flags
	unsigned priority : 3;
	unsigned protocol_id : 6;
	unsigned payload_type: 8;
	unsigned device_addr : 6;
	unsigned channel_num : 6;
}sdaq_can_identifier; 
*/

//Synchronize the SDAQ devices. Requested by broadcast only.
int Sync(int socket_fd, short time_seed)
{
	sdaq_can_identifier *sdaq_id_ptr;
	struct can_frame frame_tx;
	sdaq_id_ptr = (sdaq_can_identifier *)&(frame_tx.can_id);
	memset(sdaq_id_ptr, 0, sizeof(sdaq_can_identifier));
	//construct identifier for synchronization measure message
	sdaq_id_ptr->flags = 4;//set the EFF
	sdaq_id_ptr->protocol_id = PROTOCOL_ID;
	sdaq_id_ptr->payload_type = 0x01;//Payload type for synchronization command
	sdaq_id_ptr->device_addr = 0;//TX from broadcast only
	frame_tx.can_dlc = sizeof(short);//Payload size
	*((short *)frame_tx.data) = time_seed;
	if(write(socket_fd, &frame_tx, sizeof(struct can_frame))>0)
		return 1;
	return 0;	
}
//Request start of measure from the SDAQ device. For all dev_addr=0
int Start(int socket_fd,unsigned char dev_address)
{
	sdaq_can_identifier *sdaq_id_ptr;
	struct can_frame frame_tx;
	sdaq_id_ptr = (sdaq_can_identifier *)&(frame_tx.can_id);
	memset(sdaq_id_ptr, 0, sizeof(sdaq_can_identifier));
	//construct identifier for start measure message
	sdaq_id_ptr->flags=4;//set the EFF
	sdaq_id_ptr->protocol_id=PROTOCOL_ID;
	sdaq_id_ptr->payload_type=0x02;//Payload type for start measure command
	sdaq_id_ptr->device_addr=dev_address;
	frame_tx.can_dlc = 0;//No payload
	if(write(socket_fd, &frame_tx, sizeof(struct can_frame))<0)
		return 1;
	return 0;
}
//Request stop of measure from the SDAQ device. For all dev_addr=0
int Stop(int socket_fd,unsigned char dev_address)
{
	sdaq_can_identifier *sdaq_id_ptr;
	struct can_frame frame_tx;
	sdaq_id_ptr = (sdaq_can_identifier *)&(frame_tx.can_id);
	memset(sdaq_id_ptr, 0, sizeof(sdaq_can_identifier));
	//construct identifier for stop measure message
	sdaq_id_ptr->flags=4;//set the EFF
	sdaq_id_ptr->protocol_id=PROTOCOL_ID;
	sdaq_id_ptr->payload_type=0x03;//Payload type for stop measure command
	sdaq_id_ptr->device_addr=dev_address;
	frame_tx.can_dlc = 0;//No payload
	if(write(socket_fd, &frame_tx, sizeof(struct can_frame))<0)
		return 1;
	return 0;
}
//request change of device address with the specific serial number.
int SetDeviceAddress(int socket_fd,unsigned int dev_SN, unsigned char new_dev_address)
{
	sdaq_can_identifier *sdaq_id_ptr;
	struct can_frame frame_tx;
	sdaq_id_ptr = (sdaq_can_identifier *)&(frame_tx.can_id);
	memset(sdaq_id_ptr, 0, sizeof(sdaq_can_identifier));
	//construct identifier for change of device address message
	sdaq_id_ptr->flags=4;//set the EFF
	sdaq_id_ptr->protocol_id=PROTOCOL_ID;
	sdaq_id_ptr->payload_type=0x06;//Payload type for change of device address command
	sdaq_id_ptr->device_addr=0;//TX from broadcast only
	frame_tx.can_dlc = sizeof(unsigned int) + sizeof(unsigned char);//Payload size
	*((int *)frame_tx.data) = htonl(dev_SN); //Endianness correction 
	*(frame_tx.data + sizeof(unsigned int)) = new_dev_address;
	if(write(socket_fd, &frame_tx, sizeof(struct can_frame))<0)
		return 1;
	return 0;	
}
//request device info. Device answer with 3 messages: Device ID/status, Device Info and Calibration Date. 
int QueryDeviceInfo(int socket_fd,unsigned char dev_address)
{
	sdaq_can_identifier *sdaq_id_ptr;
	struct can_frame frame_tx;
	sdaq_id_ptr = (sdaq_can_identifier *)&(frame_tx.can_id);
	memset(sdaq_id_ptr, 0, sizeof(sdaq_can_identifier));
	//construct identifier for device info request command
	sdaq_id_ptr->flags=4;//set the EFF
	sdaq_id_ptr->protocol_id = PROTOCOL_ID;
	sdaq_id_ptr->payload_type=0x07;//Payload type for device info request command
	sdaq_id_ptr->device_addr=dev_address;
	frame_tx.can_dlc = 0;//No payload
	if(write(socket_fd, &frame_tx, sizeof(struct can_frame))<0)
		return 1;
	return 0;	
}
