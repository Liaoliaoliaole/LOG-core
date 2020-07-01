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
#define MTI_MODBUS_MAX_READ_REGISTERS (MODBUS_MAX_READ_REGISTERS-1) //Correction for the wrong MODBus implementation of icraft

/*MTI's ModBus regions Offsets*/
//In holding registers region
#define MTI_CONFIG_OFFSET 0
//In Read registers region
#define MTI_RMSWs_DATA_OFFSET 25 //short registers
#define MTI_STATUS_OFFSET 2000 //float registers
#define MTI_TELE_DATA_OFFSET 2050 //float registers

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

char *MTI_charger_state_str[]={"Discharging", "Full", "No Battery", "Charging"};
char *MTI_Data_rate_str[]={"250kbps", "1Mbps", "2Mbps"};
char *MTI_Tele_dev_type_str[]={"DISABLED", "", "TC16", "TC8", "RMSW/MUX", "2CH_QUAD", "TC4_W20"};
char *MTI_RM_dev_type_str[]={"", "RMSW", "MUX", "Mini_RMSW"};

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

int get_MTI_Radio_config(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats)
{
	struct MTI_RX_config_struct cur_RX_config;

	if(modbus_read_registers(ctx, MTI_CONFIG_OFFSET, sizeof(cur_RX_config)/sizeof(short), (unsigned short*)&cur_RX_config)<=0)
		return EXIT_FAILURE;
	//Sanitization of status values
	cur_RX_config.RF_channel = cur_RX_config.RF_channel>127?0:cur_RX_config.RF_channel;//Sanitize Radio channel frequency.
	cur_RX_config.Tele_dev_type = cur_RX_config.Tele_dev_type>Tele_TC4||cur_RX_config.Tele_dev_type<Tele_TC16?0:cur_RX_config.Tele_dev_type;//Sanitize Telemetry device type
	//Convert and load values to stats struct
	stats->MTI_Radio_config.RF_channel = cur_RX_config.RF_channel;
	stats->MTI_Radio_config.Data_rate = cur_RX_config.Data_rate;
	stats->MTI_Radio_config.Tele_dev_type = cur_RX_config.Tele_dev_type;
	for(int i=0;i<sizeof(stats->MTI_Radio_config.Specific_reg)/sizeof(stats->MTI_Radio_config.Specific_reg[0]); i++)
		stats->MTI_Radio_config.Specific_reg[i] = cur_RX_config.Specific_reg[i];
	//memcpy(&(stats->MTI_Radio_config.Specific_reg), &(cur_RX_config.Specific_reg), sizeof(stats->MTI_Radio_config.Specific_reg));
	return cur_RX_config.Tele_dev_type;
}

int get_MTI_Tele_data(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats)
{
	int remain_words, i, pos;
	union MTI_Tele_data_union{
		struct MTI_16_temp_tele as_TC16;
		struct MTI_4_temp_tele as_TC4;
		struct MTI_quad_tele as_QUAD;
		struct MTI_mux_rmsw_tele as_MUXs_RMSWs[32];
	}cur_MTI_Tele_data;
	
	switch(stats->MTI_Radio_config.Tele_dev_type)
	{
		case Tele_TC8:
		case Tele_TC16:
			if(modbus_read_input_registers(ctx, MTI_TELE_DATA_OFFSET, sizeof(cur_MTI_Tele_data.as_TC16)/sizeof(short), (unsigned short*)&cur_MTI_Tele_data)<=0)
				return EXIT_FAILURE;
			stats->Tele_data.as_TC16.packet_index = (int)cur_MTI_Tele_data.as_TC16.index;
			stats->Tele_data.as_TC16.RX_status = cur_MTI_Tele_data.as_TC16.rx_status;
			stats->Tele_data.as_TC16.RX_Success_ratio = cur_MTI_Tele_data.as_TC16.success;
			stats->Tele_data.as_TC16.Data_isValid = cur_MTI_Tele_data.as_TC16.valid_data;
			memcpy(&(stats->Tele_data.as_TC16.CHs), &(cur_MTI_Tele_data.as_TC16.channels), sizeof(stats->Tele_data.as_TC16.CHs));
			break;
		case Tele_TC4:
			if(modbus_read_input_registers(ctx, MTI_TELE_DATA_OFFSET, sizeof(cur_MTI_Tele_data.as_TC4)/sizeof(short), (unsigned short*)&cur_MTI_Tele_data)<=0)
				return EXIT_FAILURE;
			stats->Tele_data.as_TC4.packet_index = cur_MTI_Tele_data.as_TC4.index;
			stats->Tele_data.as_TC4.RX_status = cur_MTI_Tele_data.as_TC4.rx_status;
			stats->Tele_data.as_TC4.RX_Success_ratio = cur_MTI_Tele_data.as_TC4.success;
			stats->Tele_data.as_TC4.Data_isValid = cur_MTI_Tele_data.as_TC4.valid_data;
			memcpy(&(stats->Tele_data.as_TC4.CHs), &(cur_MTI_Tele_data.as_TC4.channels), sizeof(cur_MTI_Tele_data.as_TC4.channels));
			break;
		case Tele_quad:
			if(modbus_read_input_registers(ctx, MTI_TELE_DATA_OFFSET, sizeof(cur_MTI_Tele_data.as_QUAD)/sizeof(short), (unsigned short*)&cur_MTI_Tele_data)<=0)
				return EXIT_FAILURE;
			if((int)cur_MTI_Tele_data.as_QUAD.index ^ stats->Tele_data.as_QUAD.packet_index)
			{
				stats->Tele_data.as_QUAD.Data_isValid = 1;
				stats->Tele_data.as_QUAD.packet_index = cur_MTI_Tele_data.as_QUAD.index;
				stats->Tele_data.as_QUAD.RX_status = cur_MTI_Tele_data.as_QUAD.rx_status;
				stats->Tele_data.as_QUAD.RX_Success_ratio = cur_MTI_Tele_data.as_QUAD.success;
				stats->Tele_data.as_QUAD.CHs[0] = *(cur_MTI_Tele_data.as_QUAD.Channel_1);
				stats->Tele_data.as_QUAD.CHs[1] = *(cur_MTI_Tele_data.as_QUAD.Channel_2);
			}
			else
				stats->Tele_data.as_QUAD.Data_isValid = 0;
			break;
		case RM_SW_MUX:
			//Zero the amount of detected devices 
			stats->Tele_data.as_RMSWs.amount_of_devices = 0;
			//Loop that Getting The Remote controlling devices data, and store them to the cur_MTI_Tele_data struct.
			for(i=0, remain_words = sizeof(cur_MTI_Tele_data.as_MUXs_RMSWs)/sizeof(short); remain_words>0; remain_words -= MTI_MODBUS_MAX_READ_REGISTERS, i++)
			{
				if(modbus_read_input_registers(ctx, i*MTI_MODBUS_MAX_READ_REGISTERS + MTI_RMSWs_DATA_OFFSET, 
											  (remain_words>MTI_MODBUS_MAX_READ_REGISTERS?MTI_MODBUS_MAX_READ_REGISTERS:remain_words),
											  ((unsigned short*)&cur_MTI_Tele_data)+i*MTI_MODBUS_MAX_READ_REGISTERS)<=0)
					return EXIT_FAILURE;
			}
			//Loop that find the detected devices and load them to the stats.
			for(i=0, pos=0; i<MAX_RMSW_DEVs; i++)
			{
				if(cur_MTI_Tele_data.as_MUXs_RMSWs[i].dev_type)
				{
					stats->Tele_data.as_RMSWs.amount_of_devices++;
					//Convert and load data of the Detected controlling telemetry device
					stats->Tele_data.as_RMSWs.det_devs_data[pos].pos_offset = i;
					stats->Tele_data.as_RMSWs.det_devs_data[pos].dev_type = cur_MTI_Tele_data.as_MUXs_RMSWs[i].dev_type;
					stats->Tele_data.as_RMSWs.det_devs_data[pos].dev_id = cur_MTI_Tele_data.as_MUXs_RMSWs[i].dev_id;
					stats->Tele_data.as_RMSWs.det_devs_data[pos].time_from_last_mesg = cur_MTI_Tele_data.as_MUXs_RMSWs[i].time_from_last_mesg;
					stats->Tele_data.as_RMSWs.det_devs_data[pos].dev_temp = ((short)cur_MTI_Tele_data.as_MUXs_RMSWs[i].temp)/128.0;
					stats->Tele_data.as_RMSWs.det_devs_data[pos].input_voltage = cur_MTI_Tele_data.as_MUXs_RMSWs[i].input_voltage/1000.0;
					stats->Tele_data.as_RMSWs.det_devs_data[pos].switch_status.as_byte = cur_MTI_Tele_data.as_MUXs_RMSWs[i].switch_status;
					switch(stats->Tele_data.as_RMSWs.det_devs_data[pos].dev_type)
					{
						case RMSW_2CH:
							for(int j=0;j<4; j+=2)
							{
								stats->Tele_data.as_RMSWs.det_devs_data[pos].meas_data[j] = cur_MTI_Tele_data.as_MUXs_RMSWs[i].meas_data[j]/1000.0;
								stats->Tele_data.as_RMSWs.det_devs_data[pos].meas_data[j+1] = cur_MTI_Tele_data.as_MUXs_RMSWs[i].meas_data[j+1]/1000.0;
							}
							break;
						case Mini_RMSW:
							for(int j=0;j<4; j++)
								stats->Tele_data.as_RMSWs.det_devs_data[pos].meas_data[j] = ((short)cur_MTI_Tele_data.as_MUXs_RMSWs[i].meas_data[j])/16.0;
							break;
						default:
							for(int j=0; j<4;j++)
								stats->Tele_data.as_RMSWs.det_devs_data[pos].meas_data[j] = NAN;
					}
					pos++;
				}
			}
			break;
		default: 
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}