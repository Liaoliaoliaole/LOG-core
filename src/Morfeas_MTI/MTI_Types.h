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

extern char *MTI_charger_state_str[];
//--- Enumerators for MTI ---//
enum MTI_Dev_type_enum{
	Tele_TC16 = 2,
	Tele_TC8,
	RM_SW_MUX,
	Tele_quad,
	Tele_TC4
};

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

//--- From MODBus Holding Registers(R/W) 40001-40008 ---//
struct MTI_RX_config_struct{
	unsigned short RX_channel;
	unsigned short Data_rate;
	unsigned short Tele_dev_type;
	unsigned short specific_reg[5];
};