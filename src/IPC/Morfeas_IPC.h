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
#define OK_status 0
#define REG 0
#define UNREG 1
#define Dev_or_Bus_name_str_size 20
#define Data_FIFO "/tmp/.Morfeas_handlers_FIFO"

#include "../Morfeas_Types.h"

enum Morfeas_IPC_msg_type{
	IPC_Handler_register = 1,
	IPC_Handler_unregister,
	//SDAQ_related IPC messages
	IPC_SDAQ_register_or_update,
	IPC_CAN_BUS_info,
	IPC_SDAQ_clean_up,
	IPC_SDAQ_info,
	IPC_SDAQ_cal_date,
	IPC_SDAQ_timediff,
	IPC_SDAQ_meas,
	//IOBOX_related IPC messages
	IPC_IOBOX_data,
	IPC_IOBOX_channels_reg,
	IPC_IOBOX_report,
	//MDAQ_related IPC messages
	IPC_MDAQ_data,
	IPC_MDAQ_channels_reg,
	IPC_MDAQ_report,
	//MTI_related IPC messages
	IPC_MTI_data,
	IPC_MTI_tree_reg,
	IPC_MTI_Update_Health,
	IPC_MTI_Update_Radio,
	IPC_MTI_report,
	//Set MIN/MAX_num_type, (Min and Max for each IPC_handler_type)
	//---SDAQ---//
	Morfeas_IPC_SDAQ_MIN_type = IPC_SDAQ_register_or_update,
	Morfeas_IPC_SDAQ_MAX_type = IPC_SDAQ_meas,
	//---IO-BOX---//
	Morfeas_IPC_IOBOX_MIN_type = IPC_IOBOX_data,
	Morfeas_IPC_IOBOX_MAX_type = IPC_IOBOX_report,
	//---MDAQ---//
	Morfeas_IPC_MDAQ_MIN_type = IPC_MDAQ_data,
	Morfeas_IPC_MDAQ_MAX_type = IPC_MDAQ_report,
	//---MTI---//
	Morfeas_IPC_MTI_MIN_type = IPC_MTI_data,
	Morfeas_IPC_MTI_MAX_type = IPC_MTI_report,
	//MAX number of any type of IPC message
	Morfeas_IPC_MAX_type = IPC_MTI_report
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
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned char handler_type;
}Handler_reg_op_msg;

  //------ SDAQ + SDAQnet Port related ------//
typedef struct SDAQ_register_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned char address;
	sdaq_status SDAQ_status;
	unsigned char t_amount;
}SDAQ_reg_update_msg;

typedef struct SDAQ_clean_registeration_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int SDAQ_serial_number;
	unsigned char t_amount;
}SDAQ_clear_msg;

typedef struct SDAQ_info_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int SDAQ_serial_number;
	sdaq_info SDAQ_info_data;
}SDAQ_info_msg;

typedef struct SDAQ_cal_date_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int SDAQ_serial_number;
	unsigned char channel;
	sdaq_calibration_date SDAQ_cal_date;
}SDAQ_cal_date_msg;

typedef struct SDAQ_timediff_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int SDAQ_serial_number;
	short Timediff;
}SDAQ_timediff_msg;

typedef struct SDAQ_measure_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int SDAQ_serial_number;
	unsigned char Amount_of_channels;
	unsigned short Last_Timestamp;
	struct Channel_curr_meas SDAQ_channel_meas[SDAQ_MAX_AMOUNT_OF_CHANNELS-1];
}SDAQ_meas_msg;

typedef struct CAN_BUS_info_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	float BUS_utilization;
	unsigned char Electrics;//Boolean: false -> No Electrics info
	float amperage;
	float voltage;
	float shunt_temp;
}CAN_BUS_info_msg;

	//------ IO-BOX related ------//
typedef struct IOBOX_data_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int IOBOX_IPv4;
	float Supply_Vin;
	struct Power_Supply Supply_meas[4];
	struct RXs RX[4];
}IOBOX_data_msg;

typedef struct IOBOX_channels_reg_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int IOBOX_IPv4;
}IOBOX_channels_reg_msg;

typedef struct IOBOX_report_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int IOBOX_IPv4;
	int status;
}IOBOX_report_msg;

	//------ MDAQ related ------//
typedef struct MDAQ_data_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int MDAQ_IPv4;
	unsigned int meas_index;
	float board_temp;
	struct MDAQ_Channel meas[8];
}MDAQ_data_msg;

typedef struct MDAQ_channels_reg_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int MDAQ_IPv4;
}MDAQ_channels_reg_msg;

typedef struct MDAQ_report_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int MDAQ_IPv4;
	int status;
}MDAQ_report_msg;

	//------ MTI related ------//
typedef struct MTI_tree_reg_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int MTI_IPv4;
}MTI_tree_reg_msg;

typedef struct MTI_report_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int MTI_IPv4;
	int status;
}MTI_report_msg;

typedef struct MTI_Update_Health_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	float cpu_temp;
	float batt_capacity;
	float batt_voltage;
	unsigned char batt_state; 
}MTI_Update_Health_msg;

typedef struct MTI_Update_Radio_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int MTI_IPv4;
	unsigned char RF_channel;
	unsigned Data_rate:2;
	unsigned Tele_dev_type:3;
	unsigned new_config:1;
}MTI_Update_Radio_msg;

/*typedef struct MTI_data_msg_struct{
	unsigned char IPC_msg_type;
	char Dev_or_Bus_name[Dev_or_Bus_name_str_size];
	unsigned int MTI_IPv4;
	unsigned int meas_index;
	float board_temp;
	struct MTI_Channel meas[8];
}MTI_data_msg;*/
#pragma pack(pop)//Disable packing

//--IPC_MESSAGE--//
typedef union{
	Handler_reg_op_msg Handler_reg;
	//SDAQ + CANBus related
	SDAQ_reg_update_msg SDAQ_reg_update;
	SDAQ_clear_msg SDAQ_clean;
	SDAQ_info_msg SDAQ_info;
	SDAQ_cal_date_msg SDAQ_cal_date;
	SDAQ_timediff_msg SDAQ_timediff;
	SDAQ_meas_msg SDAQ_meas;
	CAN_BUS_info_msg BUS_info;
	//IO-BOX related
	IOBOX_data_msg IOBOX_data;
	IOBOX_channels_reg_msg IOBOX_channels_reg;
	IOBOX_report_msg IOBOX_report;
	//MDAQ related
	MDAQ_data_msg MDAQ_data;
	MDAQ_channels_reg_msg MDAQ_channels_reg;
	MDAQ_report_msg MDAQ_report;
	//MTI related
	MTI_tree_reg_msg MTI_tree_reg;
	MTI_report_msg MTI_report;
	MTI_Update_Health_msg MTI_Update_Health;
	MTI_Update_Radio_msg MTI_Update_Radio;
	//MTI_data_msg MTI_data;
}IPC_message;

//Function that convert interface_type_string to interface_type_num
int if_type_str_2_num(const char * if_type_str);

	//----RX Functions----//
//function for RX, return the type of the received message or 0 in failure
unsigned char IPC_msg_RX(int FIFO_fd, IPC_message *IPC_msg_ptr);

	//----TX Functions----//
//function for TX, return the amount of bytes that transmitted through the FIFO, or 0 in failure
size_t IPC_msg_TX(int FIFO_fd, IPC_message *IPC_msg_ptr);
//Function for construction of message for registration of a Handler
size_t IPC_Handler_reg_op(int FIFO_fd, unsigned char handler_type, char *Dev_or_Bus_name, unsigned char unreg);

