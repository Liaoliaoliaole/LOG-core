/*
File: Morfeas_IPC.h, Declaration of functions, structs and union for IPC.
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
#include "Types.h"

extern size_t Morfeas_IPC_msg_size[];
extern char *Morfeas_IPC_handler_type_name[];

enum Morfeas_IPC_msg_type{
	IPC_Handler_register = 1,
	IPC_Handler_unregister,
	//SDAQ_related IPC messages//
	IPC_SDAQ_register_or_update,
	IPC_SDAQ_clean_up,
	IPC_SDAQ_info,
	IPC_SDAQ_timediff,
	IPC_SDAQ_meas,
	IPC_CAN_BUS_info,
	//Set MAX_num_type
	Morfeas_IPC_MAX_type = IPC_CAN_BUS_info
};

enum Morfeas_IPC_handler_type{
	SDAQ = 0,
	MDAQ,
	IOBOX,
	MTI //Mobile Telemetry Interface
};

#pragma pack(push, 1)//use pragma pack() to pack the following structs to 1 byte size (aka no zero padding)
	//---Bus Handlers related---//
typedef struct Handler_register_struct{
	char connected_to_BUS[10];
	unsigned char handler_type;
}Handler_reg_op_msg;

	//------SDAQ related------//
typedef struct SDAQ_register_msg_struct{
	char connected_to_BUS[10];
	unsigned char address;
	sdaq_status SDAQ_status;
	unsigned char reg;
	unsigned char t_amount;
}SDAQ_reg_update_msg;

typedef struct SDAQ_clean_registeration_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
	unsigned char t_amount;
}SDAQ_clear_msg;

typedef struct SDAQ_info_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
	sdaq_info SDAQ_info_data;
}SDAQ_info_msg;

typedef struct SDAQ_timediff_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
	short Timediff;
}SDAQ_timediff_msg;

typedef struct SDAQ_measure_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
	unsigned char channel;
	sdaq_meas SDAQ_channel_meas;
}SDAQ_meas_msg;

typedef struct CAN_BUS_info_msg_struct{
	char connected_to_BUS[10];
	float BUS_utilization;
}CAN_BUS_info_msg;

typedef union{
	Handler_reg_op_msg Handler_reg;
	SDAQ_reg_update_msg SDAQ_reg_update;
	SDAQ_clear_msg SDAQ_clean;
	SDAQ_info_msg SDAQ_info;
	SDAQ_timediff_msg SDAQ_timediff;
	SDAQ_meas_msg SDAQ_meas;
	CAN_BUS_info_msg BUS_info;
}IPC_message;
#pragma pack(pop)//Disable packing

	//----TX Functions----//
//function for TX, return the amount of bytes that transmitted through the FIFO, or 0 in failure
size_t IPC_msg_TX(const char *path_to_FIFO, IPC_message *IPC_msg_ptr, unsigned char type);
//Function for construction of message for registration of a Handler
size_t IPC_Handler_reg_op(const char *path_to_FIFO, unsigned char handler_type, char connected_to_BUS[10], unsigned char unreg);
	//----RX Functions----//
//function for RX, return the type of the received message or 0 in failure
//unsigned char IPC_msg_RX(const char *path_to_FIFO, IPC_message *IPC_msg_ptr);
unsigned char IPC_msg_RX(int FIFO_fd, IPC_message *IPC_msg_ptr);
