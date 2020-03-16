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

//Array with strings of the Supported Interface_names.
extern char *Morfeas_IPC_handler_type_name[];

//Structs for IOBOX_handler
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

//Structs for MDAQ_handler
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

	//Structs for MTI_handler
//--- Extracted from MODBus Input Registers(Read only) 32001... ---//
struct MTI_dev_status{
	float batt_volt;
	float batt_cap;
	float batt_state;
	float CPU_temp;
	float Button_state;
	float PWM_clock;
	float PWM_freq;
	float PWM_Channels[4];
};
struct MTI_16_temp_tele{
	float index;
	float rx_status;
	float success;
	float valid_data;
	float valid_data_cnt;
	float reserved[5];
	float channels[16];
};
struct MTI_4_temp_tele{
	float index;
	float rx_status;
	float success;
	float valid_data;
	float valid_data_cnt;
	float reserved[5];
	float channels[4];
	float ref_1_2;
	float ref_3_4;
};
struct MTI_mux_rmsw_tele{
	float dev_type;
	float dev_id;
	float last_mesg;
	float switch_status;
	float temp;
	float input_voltage;
	struct{
		float voltage;
		float amperage;
	}channels[2];
};
struct MTI_quad_tele{
	float index;
	float rx_status;
	float success;
	float sampling_rate;
	float drift_index;
	float reserved[5];
	float channels[2];
};

//--- Extracted from MODBus Holding Registers(R/W) 40001-40008 ---//
struct MTI_RX_config{
	unsigned short RX_channel;
	unsigned short Data_rate;
	unsigned short Tele_dev_type;
	unsigned short specific_reg[5];
};

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
// Data of a list_SDAQs node, used in Morfeas_SDAQ_if
struct SDAQ_info_entry{
	unsigned char SDAQ_address;
	short Timediff;
	sdaq_status SDAQ_status;
	sdaq_info SDAQ_info;
	GSList *SDAQ_Channels_cal_dates;
	GSList *SDAQ_Channels_acc_meas;
	time_t last_seen;
	unsigned info_collection_status : 2;//3 = All info collected, 2 = Only Dev_info collected, 1 = Dev_info requested, 0 = Nothing has been collected
};
// Data of a SDAQ_average_meas node
struct Channel_acc_meas_entry{
	unsigned char Channel;
	unsigned char status;
	unsigned char unit_code;
	float meas_acc;
	unsigned short cnt;
};
// Data of a SDAQ_cal_dates node
struct Channel_date_entry{
	unsigned char Channel;
	sdaq_calibration_date CH_date;
};
// Data of a list_SDAQs node, used in Morfeas_SDAQ_if
struct LogBook_entry{
	unsigned int SDAQ_sn;
	unsigned char SDAQ_address;
}__attribute__((packed, aligned(1)));
// Data entry of a LogBook file, used in Morfeas_SDAQ_if
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
//struct for system_stats
struct system_stats{
	float CPU_Util,RAM_Util,CPU_temp,Disk_Util;
	unsigned int Up_time;
};
