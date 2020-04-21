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


//--- Enumerators for MTI ---//
enum MTI_Dev_type_enum{
	Tele_16TC = 2,
	Tele_8TC = 3,
	RM_SW_MUX = 4,
	Tele_quad = 5,
	Tele_4TC = 6
};

//--- Extracted from MODBus Input Registers(Read only) 32001... ---//
struct MTI_dev_status{
	unsigned short batt_volt;
	unsigned short batt_cap;
	unsigned short batt_state;
	unsigned short CPU_temp;
	unsigned short Button_state;
	unsigned short PWM_clock;
	unsigned short PWM_freq;
	unsigned short PWM_Channels[4];
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