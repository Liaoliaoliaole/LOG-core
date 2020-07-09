/*
File: MTI_types.h, Contain the Declaration for data types that related to MTI, Part of Morfeas_project.
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

#define MTI_MODBUS_MAX_READ_REGISTERS (MODBUS_MAX_READ_REGISTERS-1) //Correction for the wrong MTI's MODBus implementation

/*MTI's ModBus regions Offsets*/
//In holding registers region
#define MTI_CONFIG_OFFSET 0
//In Read registers region
#define MTI_RMSWs_DATA_OFFSET 25 //short registers
#define MTI_PULSE_GEN_OFFSET 1050 //int registers
#define MTI_STATUS_OFFSET 2000 //float registers
#define MTI_TELE_DATA_OFFSET 2050 //float registers

//--- From MODBus Input Registers(Read only) 32001... ---//
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
	float channels[6];
};
struct MTI_quad_tele{
	float index;
	float rx_status;
	float success;
	float samplerate;
	float drift_index;
	float reserved[5];
	float Channel_1[5];
	float Channel_2[5];
};
struct MTI_mux_rmsw_tele{
	unsigned short dev_type;
	unsigned short dev_id;
	unsigned short time_from_last_mesg;
	unsigned short switch_status;
	unsigned short temp;
	unsigned short input_voltage;
	unsigned short meas_data[4];
};

//--- From MODBus Holding Registers(R/W) 40001-40008 ---//
struct MTI_RX_config_struct{
	unsigned short RF_channel;
	unsigned short Data_rate;
	unsigned short Tele_dev_type;
	unsigned short Specific_reg[22];
};

//--- From MODBus Holding Registers(R/W) 41051-41071 ---//
struct MTI_PWM_config_struct{
	unsigned int PWM_out_freq;
	int PLL_Control_high;
	int PLL_Control_low;
	struct PWM_Channels_control_struct{
		unsigned int cnt_max;
		unsigned int cnt_min;
		unsigned int middle_val;
		unsigned int cnt_mode;
	}CHs[2];
};
