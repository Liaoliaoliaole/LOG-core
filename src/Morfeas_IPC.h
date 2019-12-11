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
	IPC_Handler_unregister = 2,
	//SDAQ_related IPC messages//
	IPC_SDAQ_register_or_update = 3,
	IPC_SDAQ_clean_up = 4,
	IPC_SDAQ_info = 5,
	IPC_SDAQ_timediff = 6,
	IPC_SDAQ_meas = 7
};

enum Morfeas_IPC_handler_type{
	SDAQ = 0,
	MDAQ = 1,
	IOBOX = 2,
	MTI = 3 //Mobile Telemetry Interface
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
}SDAQ_reg_update_msg;

typedef struct SDAQ_clean_registeration_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
}SDAQ_clear_msg;

typedef struct SDAQ_info_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
	sdaq_info SDAQ_info;
}SDAQ_info_msg;

typedef struct SDAQ_timediff_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
	short Timediff;
}SDAQ_timediff_msg;

typedef struct SDAQ_measure_msg_struct{
	char connected_to_BUS[10];
	unsigned int serial_number;
	unsigned char channel;
	sdaq_meas SDAQ_channel_meas;
}SDAQ_meas_msg;

typedef union{
	Handler_reg_op_msg Handler_reg;
	SDAQ_reg_update_msg SDAQ_reg_update;
	SDAQ_clear_msg SDAQ_clean;
	SDAQ_info_msg SDAQ_info;
	SDAQ_timediff_msg SDAQ_timediff;
	SDAQ_meas_msg SDAQ_meas;
}IPC_msg;
#pragma pack(pop)//Disable packing

	//----TX Functions----//
//function for TX, return the amount of bytes that transmitted through the FIFO, or 0 in failure
int IPC_msg_TX(const char *path_to_FIFO, IPC_msg *IPC_msg_ptr, unsigned char type);
//Function for construction of message for registration of a Handler
int IPC_Handler_reg_op(const char *path_to_FIFO, unsigned char handler_type, char connected_to_BUS[10], unsigned char unreg);
//Function for construction of message for registration or update of a SDAQ
int IPC_SDAQ_reg_update(const char *path_to_FIFO, char connected_to_BUS[10], unsigned char address, sdaq_status *SDAQ_status);
	//----RX Functions----//
//function for RX, return the type of the received message or 0 in failure
int IPC_msg_RX(const char *path_to_FIFO, IPC_msg *IPC_msg_ptr);
