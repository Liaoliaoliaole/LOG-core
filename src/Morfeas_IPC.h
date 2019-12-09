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

enum Morfeas_IPC_msg_type{
	Handler_register = 1, 
	Handler_unregister = 2, 
	//SDAQ_related IPC messages//
	SDAQ_register = 3,
	SDAQ_clean_up = 4,
	SDAQ_info = 5,
	SDAQ_timediff = 6,
	SDAQ_meas = 7
};

enum Morfeas_IPC_handler_type{
	SDAQ,
	//future implementation 
	MDAQ,
	IOBOX,
	MTI //Mobile Telemetry Interface
};

#pragma pack(push, 1)//use pragma pack() to pack the following structs to 1 byte size (aka no zero padding)
	//---Bus Handlers related---//
typedef struct Handler_register_unregister_struct{
	unsigned char handler_type;
	char handling_BUS_name[20];
	unsigned unreg : 1;
}Handler_reg_op_msg;

	//------SDAQ related------//
typedef struct SDAQ_register_msg_struct{
	char connected_to_BUS[10];
	unsigned char address;
	sdaq_status SDAQ_status;
}SDAQ_register_msg;

typedef struct SDAQ_clean_registeration_msg_struct{
	char connected_to_BUS[10];
	unsigned int SDAQ_serial_number;
}SDAQ_clean_msg;

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
	char anchor_str[20];
	sdaq_meas SDAQ_channel_meas;
}SDAQ_meas_msg;
#pragma pack(pop)//Disable packing

typedef union{
	Handler_reg_op_msg handler_reg;
	SDAQ_register_msg SDAQ_reg;
	SDAQ_clean_msg SDAQ_clean;
	SDAQ_info_msg SDAQ_info;
	SDAQ_timediff_msg SDAQ_timediff;
	SDAQ_meas_msg SDAQ_meas;
}IPC_msg;