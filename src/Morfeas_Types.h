/*
File "Morfeas_Types.h" part of Morfeas project, contain the shared Datatypes.
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
#define ISO_channel_name_size 20

//Default MODBus Slave address
#define default_slave_address 10

//Defs for IOBOX_handler
#define IOBOX_start_reg 0
#define IOBOX_imp_reg 125

//Defs for MDAQ_handler
#define MDAQ_start_reg 100
#define MDAQ_imp_reg 90

#include <gmodule.h>
#include <glib.h>
//Include SDAQ Driver header
#include "sdaq-worker/src/SDAQ_drv.h"
//Include MTI data type header
#include "Morfeas_MTI/MTI_Types.h"

//Array with strings of the Supported Interface_names.
extern char *Morfeas_IPC_handler_type_name[];
extern char *MTI_charger_state_str[];
extern char *MTI_Data_rate_str[];
extern char *MTI_Tele_dev_type_str[];
extern char *MTI_RM_dev_type_str[];

/*Structs for IOBOX_handler*/
struct Power_Supply{
	float Vout,Iout;
};
struct RXs{
	float CH_value[16];
	unsigned short index;
	unsigned char status;
	unsigned char success;
};
struct Morfeas_IOBOX_if_stats{
	char *IOBOX_IPv4_addr;
	char *dev_name;
	int error;
	float Supply_Vin;
	struct Power_Supply Supply_meas[4];
	struct RXs RX[4];
	unsigned int counter;
};

/*Structs for MDAQ_handler*/
struct MDAQ_Channel{
	float value[4];
	unsigned char warnings;
};
struct Morfeas_MDAQ_if_stats{
	char *MDAQ_IPv4_addr;
	char *dev_name;
	int error;
	unsigned int meas_index;
	float board_temp;
	struct MDAQ_Channel meas[8];
	unsigned int counter;
};

//--- Enumerators for MTI ---//
enum MTI_Dev_type_enum{
	Tele_TC16 = 2,
	Tele_TC8,
	RM_SW_MUX,
	Tele_quad,
	Tele_TC4
};

/*Structs for MTI_handler*/ 
struct MTI_status_struct{
	unsigned char Radio_CH;
	unsigned char Modem_data_rate;
	unsigned char Tele_Device_type;
	float MTI_batt_volt;
	float MTI_batt_capacity;
	unsigned char MTI_charge_status;
	float MTI_CPU_temp;
	struct{
		unsigned char pb1:1;
		unsigned char pb2:1;
		unsigned char pb3:1;
	}buttons_state;
	float PWM_gen_out_freq;
	float PWM_outDuty_CHs[4];
};
struct MTI_Radio_config_status_struct{
	unsigned RX_channel:7;
	unsigned short Data_rate:2;
	unsigned short Tele_dev_type:3;
	unsigned short Specific_reg[5];
};
//Structs for MTI related telemetry device
struct TC4_data_struct{
	unsigned short packet_index;
	unsigned RX_status:2;
	unsigned char RX_Success_ratio;
	unsigned Data_isValid:1;
	float CHs[4];
	float Refs[2];
};
struct TC8_data_struct{
	unsigned short packet_index;
	unsigned RX_status:2;
	unsigned char RX_Success_ratio;
	unsigned Data_isValid:1;
	float CHs[8];
	float Refs[8];
};
struct TC16_data_struct{
	unsigned short packet_index;
	unsigned RX_status:2;
	unsigned char RX_Success_ratio;
	unsigned Data_isValid:1;
	float CHs[16];
};
struct QUAD_data_struct{
	unsigned short packet_index;
	unsigned RX_status:2;
	unsigned char RX_Success_ratio;
	unsigned Data_isValid:1;
	float CHs[2];
};
struct RMSW_MUX_Mini_data_struct{
	unsigned dev_type:2;
	unsigned short dev_id;
	unsigned char last_mesg;
	unsigned short switch_status;
	float dev_temp;
	float input_voltage;
	float meas_data[4];
};
struct RM_devs_data_struct{
	unsigned char amount;
	struct RMSW_MUX_Mini_data_struct det_devs_data[32];
};
//Morfeas_MTI_if_stats stats struct, used in Morfeas_MTI_if
struct Morfeas_MTI_if_stats{
	char *MTI_IPv4_addr;
	char *dev_name;
	int error;
	struct MTI_status_struct MTI_status;
	struct MTI_Radio_config_status_struct MTI_Radio_config;
	union MTI_Telemetry_data{
		struct TC4_data_struct as_TC4;
		struct TC8_data_struct as_TC8;
		struct TC16_data_struct as_TC16;
		struct QUAD_data_struct as_QUAD;
		struct RM_devs_data_struct as_RMSWs;
	} Tele_data;
	unsigned int counter;
};

/*Structs for SDAQ_handler*/
//Morfeas_SDAQ-if stats struct, used in Morfeas_SDAQ_if
struct Morfeas_SDAQ_if_stats{
	char LogBook_file_path[100];
	int FIFO_fd;
	char *CAN_IF_name;
	unsigned char port;
	float Bus_util;
	//Electrics and last calibration date for Morfeas_Rpi_hat
	unsigned int Morfeas_RPi_Hat_last_cal;
	float Bus_voltage;
	float Bus_amperage;
	float Shunt_temp;
	unsigned char detected_SDAQs;// Amount of online SDAQ.
	GSList *list_SDAQs;// List with SDAQ status, info and last seen timestamp.
	GSList *LogBook;//List of the LogBook file
};
// Data of a current_measurements node
struct Channel_curr_meas{
	float meas;
	unsigned char unit;
	unsigned char status;
};
// Data of a list_SDAQs node, used in Morfeas_SDAQ_if
struct SDAQ_info_entry{
	unsigned char SDAQ_address;
	short Timediff;
	unsigned short Last_Timestamp;
	sdaq_status SDAQ_status;
	sdaq_info SDAQ_info;
	GSList *SDAQ_Channels_cal_dates;
	GSList *SDAQ_Channels_acc_meas;
	struct Channel_curr_meas *SDAQ_Channels_curr_meas;
	time_t last_seen;
	unsigned info_collection_status : 2;//3 = All info collected, 2 = Only Dev_info collected, 1 = Dev_info requested, 0 = Nothing has been collected
};
// Data of a SDAQ_cal_dates node
struct Channel_date_entry{
	unsigned char Channel;
	sdaq_calibration_date CH_date;
};
// Data of a SDAQ_average_meas node
struct Channel_acc_meas_entry{
	unsigned char Channel;
	unsigned char status;
	unsigned char unit_code;
	float meas_acc;
	unsigned short cnt;
};
// Data entry of a LogBook file, used in Morfeas_SDAQ_if
struct LogBook_entry{
	unsigned int SDAQ_sn;
	unsigned char SDAQ_address;
}__attribute__((packed, aligned(1)));
// struct of LogBook entry and it's Checksum, used in Morfeas_SDAQ_if
struct LogBook{
	struct LogBook_entry payload;
	unsigned char checksum;
}__attribute__((packed, aligned(1)));
//Data of the List Links, used in Morfeas_opc_ua
struct Link_entry{
	char ISO_channel_name[ISO_channel_name_size];
	char interface_type[10];
	unsigned char interface_type_num;
	unsigned int identifier;
	unsigned char channel;
	unsigned char receiver_or_value;
};
/*struct for system_stats*/
struct system_stats{
	float CPU_Util,RAM_Util,CPU_temp,Disk_Util;
	unsigned int Up_time;
};
