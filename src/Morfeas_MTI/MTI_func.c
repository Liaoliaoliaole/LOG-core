/*
File: MTI_func.c, Implementation of functions for MTI (MODBus), Part of Morfeas_project.
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

/*MTI's ModBus regions Offsets*/
#define MTI_STATUS_OFFSET 2000
#define MTI_CONFIG_OFFSET 0
#define MTI_TELE_DATA_OFFSET 2050

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>

#include <modbus.h>

#include "../Morfeas_Types.h" 

char *MTI_charger_state_str[]={"Discharging", "No Battery", "Full", "Charging"};
char *MTI_Data_rate_str[]={"250kbps", "1Mbps", "2Mbps"};
char *MTI_Tele_dev_type_str[]={"DISABLED", "", "TC16", "TC8", "RMSW/MUX", "2CH_QUAD", "TC4_W20"};

int get_MTI_status(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats)
{
	struct MTI_dev_status cur_status;
	
	if(modbus_read_input_registers(ctx, MTI_STATUS_OFFSET, sizeof(cur_status)/sizeof(short), (unsigned short*)&cur_status)<=0)
		return EXIT_FAILURE;
	//Convert and load MTI_status to stats struct
	stats->MTI_status.MTI_batt_volt = cur_status.batt_volt;
	stats->MTI_status.MTI_batt_capacity = cur_status.batt_cap;
	stats->MTI_status.MTI_charge_status = (unsigned char)cur_status.batt_state; 
	stats->MTI_status.MTI_CPU_temp = cur_status.CPU_temp;
	stats->MTI_status.buttons_state.pb1 = cur_status.Button_state==4.0;
	stats->MTI_status.buttons_state.pb2 = cur_status.Button_state==2.0;
	stats->MTI_status.buttons_state.pb3 = cur_status.Button_state==1.0;
	stats->MTI_status.PWM_gen_out_freq = cur_status.PWM_freq;
	for(unsigned char i=0; i<4;i++)
		stats->MTI_status.PWM_outDuty_CHs[i] = cur_status.PWM_Channels[i];
	return EXIT_SUCCESS;
}

int get_MTI_RX_config(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats)
{
	struct MTI_RX_config_struct cur_RX_config;
	if(modbus_read_registers(ctx, MTI_CONFIG_OFFSET, sizeof(cur_RX_config)/sizeof(short), (unsigned short*)&cur_RX_config)<=0)
		return EXIT_FAILURE;
	cur_RX_config.RX_channel = cur_RX_config.RX_channel>127?0:cur_RX_config.RX_channel;//Sanitize Radio channel frequency.
	cur_RX_config.Tele_dev_type = cur_RX_config.Tele_dev_type>Tele_TC4||cur_RX_config.Tele_dev_type<Tele_TC16?0:cur_RX_config.Tele_dev_type;//Sanitize Telemetry device type
	memcpy(&(stats->MTI_RX_config), &cur_RX_config, sizeof(cur_RX_config));
	return cur_RX_config.Tele_dev_type;
}

int get_MTI_Tele_data(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats)
{
	union MTI_Tele_data_union{
		struct MTI_16_temp_tele as_TC16;
		struct MTI_4_temp_tele as_TC4;
		struct MTI_quad_tele as_QUAD;
		struct MTI_mux_rmsw_tele as_MUXs_RMSWs[32];
	}cur_MTI_Tele_data;
	switch(stats->MTI_RX_config.Tele_dev_type)
	{
		case Tele_TC8:
		case Tele_TC16:
			if(modbus_read_registers(ctx, MTI_TELE_DATA_OFFSET, sizeof(cur_MTI_Tele_data.as_TC16)/sizeof(short), (unsigned short*)&cur_MTI_Tele_data)<=0)
				return EXIT_FAILURE;
			break;
		case Tele_TC4:
			if(modbus_read_registers(ctx, MTI_TELE_DATA_OFFSET, sizeof(cur_MTI_Tele_data.as_TC4)/sizeof(short), (unsigned short*)&cur_MTI_Tele_data)<=0)
				return EXIT_FAILURE;
			break;
		case Tele_quad:
			if(modbus_read_registers(ctx, MTI_TELE_DATA_OFFSET, sizeof(cur_MTI_Tele_data.as_QUAD)/sizeof(short), (unsigned short*)&cur_MTI_Tele_data)<=0)
				return EXIT_FAILURE;
			break;
		case RM_SW_MUX:
			break;
		default: EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}