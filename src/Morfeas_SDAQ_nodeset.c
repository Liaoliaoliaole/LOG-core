/*
File: Morfeas_SDAQ_nodeset.c, implementation of OPC-UA server's Nodeset
construction/deconstruction functions for Morfeas SDAQ_handler.

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

//Include Functions implementation header
#include "Morfeas_handlers_nodeset.h"

pthread_mutex_t OPC_UA_NODESET_access = PTHREAD_MUTEX_INITIALIZER;

void SDAQ_handler_reg(UA_Server *server_ptr, char *connected_to_BUS)
{
	char tmp_buff[30], tmp_buff_1[30];
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		sprintf(tmp_buff, "%s-if (%s)", Morfeas_IPC_handler_type_name[SDAQ], connected_to_BUS);
		Morfeas_opc_ua_add_abject_node(server_ptr,"Morfeas_Handlers", connected_to_BUS, tmp_buff);
		sprintf(tmp_buff, "%s.SDAQnet", connected_to_BUS);
		Morfeas_opc_ua_add_abject_node(server_ptr, connected_to_BUS, tmp_buff, "SDAQnet");
		sprintf(tmp_buff, "%s.BUS_util", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, connected_to_BUS, tmp_buff, "BUS_Util (%)", UA_TYPES_FLOAT);
		sprintf(tmp_buff, "%s.amount", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, connected_to_BUS, tmp_buff, "Dev_on_BUS", UA_TYPES_BYTE);
		//Object with electric status of a SDAQnet port
		sprintf(tmp_buff, "%s.Electrics", connected_to_BUS);
		Morfeas_opc_ua_add_abject_node(server_ptr, connected_to_BUS, tmp_buff, "Electric");
		sprintf(tmp_buff_1, "%s.volts", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff, tmp_buff_1, "Voltage", UA_TYPES_FLOAT);
		sprintf(tmp_buff_1, "%s.amps", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff, tmp_buff_1, "Amperage", UA_TYPES_FLOAT);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void SDAQ2OPC_UA_register_update(UA_Server *server_ptr, SDAQ_reg_update_msg *ptr)
{
	char SDAQ_anchor_str[15], tmp_str[50], tmp_str2[50];
	UA_NodeId out;
	UA_NodeId_init(&out);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		sprintf(SDAQ_anchor_str,"%s.%d",ptr->connected_to_BUS,ptr->SDAQ_status.dev_sn);
		if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, SDAQ_anchor_str), &out))
		{
			sprintf(tmp_str,"%s.SDAQnet",ptr->connected_to_BUS);
			sprintf(tmp_str2,"%s (%02hhu)", (char *)dev_type_str[ptr->SDAQ_status.dev_type], ptr->address);
			Morfeas_opc_ua_add_abject_node(server_ptr, tmp_str, SDAQ_anchor_str, tmp_str2);
			sprintf(tmp_str,"%s.S/N",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "S/N", UA_TYPES_UINT32);
			sprintf(tmp_str,"%s.Address",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "Address", UA_TYPES_BYTE);
			sprintf(tmp_str,"%s.TimeDiff",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "TimeDiff", UA_TYPES_UINT16);
			sprintf(tmp_str2,"%s.Status",SDAQ_anchor_str);
			Morfeas_opc_ua_add_abject_node(server_ptr, SDAQ_anchor_str, tmp_str2, "Status");
			sprintf(tmp_str,"%s.State",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "State", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.inSync",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "inSync", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.Error",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "Error", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.Mode",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "Mode", UA_TYPES_STRING);
		}
		sprintf(tmp_str,"%s.amount",ptr->connected_to_BUS);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->t_amount), UA_TYPES_BYTE);
		sprintf(tmp_str,"%s.S/N",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_status.dev_sn), UA_TYPES_UINT32);
		sprintf(tmp_str,"%s.Address",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->address), UA_TYPES_BYTE);
		sprintf(tmp_str,"%s.State",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, State), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.inSync",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, In_sync), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.Error",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Error), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.Mode",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Mode), UA_TYPES_STRING);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void SDAQ2OPC_UA_register_update_info(UA_Server *server_ptr, SDAQ_info_msg *ptr)
{
	char SDAQ_anchor_str[15], tmp_str[50], tmp_str2[50], tmp_str3[60];
	UA_NodeId out;
	UA_NodeId_init(&out);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		sprintf(SDAQ_anchor_str,"%s.%d", ptr->connected_to_BUS, ptr->SDAQ_serial_number);
		if(!UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, SDAQ_anchor_str), &out))
		{
			UA_NodeId_init(&out);
			sprintf(tmp_str,"%s.Info",SDAQ_anchor_str);
			if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, tmp_str), &out))
			{
				Morfeas_opc_ua_add_abject_node(server_ptr, SDAQ_anchor_str, tmp_str, "Info");
				sprintf(tmp_str2,"%s.Firm_Rev",SDAQ_anchor_str);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Firm_Rev", UA_TYPES_BYTE);
				sprintf(tmp_str2,"%s.Hw_Rev",SDAQ_anchor_str);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Hw_Rev", UA_TYPES_BYTE);
				sprintf(tmp_str2,"%s.Amount_of_channels",SDAQ_anchor_str);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Channels_on_SDAQ", UA_TYPES_BYTE);
				sprintf(tmp_str2,"%s.Samplerate",SDAQ_anchor_str);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Samplerate", UA_TYPES_BYTE);
				sprintf(tmp_str2,"%s.Max_cal_points",SDAQ_anchor_str);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Max_cal_points", UA_TYPES_BYTE);
				sprintf(tmp_str,"%s.Channels",SDAQ_anchor_str);
				Morfeas_opc_ua_add_abject_node(server_ptr, SDAQ_anchor_str, tmp_str, "Channels");
			}
			sprintf(tmp_str,"%s.Channels",SDAQ_anchor_str);
			for(unsigned char i=1; i <= ptr->SDAQ_info_data.num_of_ch; i++)
			{
				UA_NodeId_init(&out);
				sprintf(tmp_str2,"%s.CH%hhu", SDAQ_anchor_str, i);
				if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, tmp_str2), &out))
				{
					sprintf(tmp_str3,"CH%02hhu", i);
					Morfeas_opc_ua_add_abject_node(server_ptr, tmp_str, tmp_str2, tmp_str3);
					sprintf(tmp_str3,"%s.meas", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Value", UA_TYPES_FLOAT);
					sprintf(tmp_str3,"%s.timestamp", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Timestamp", UA_TYPES_UINT16);
					sprintf(tmp_str3,"%s.status", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Status", UA_TYPES_STRING);
					sprintf(tmp_str3,"%s.unit", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Unit", UA_TYPES_STRING);
					sprintf(tmp_str3,"%s.dates",tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Calibration Date", UA_TYPES_DATETIME);
					sprintf(tmp_str3,"%s.period",tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Period (Months)", UA_TYPES_BYTE);
					sprintf(tmp_str3,"%s.points",tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Calibration Points", UA_TYPES_BYTE);
				}
			}
			sprintf(tmp_str,"%s.Firm_Rev",SDAQ_anchor_str);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.firm_rev), UA_TYPES_BYTE);
			sprintf(tmp_str,"%s.Hw_Rev",SDAQ_anchor_str);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.hw_rev), UA_TYPES_BYTE);
			sprintf(tmp_str,"%s.Amount_of_channels",SDAQ_anchor_str);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.num_of_ch), UA_TYPES_BYTE);
			sprintf(tmp_str,"%s.Samplerate",SDAQ_anchor_str);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.sample_rate), UA_TYPES_BYTE);
			sprintf(tmp_str,"%s.Max_cal_points",SDAQ_anchor_str);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.max_cal_point), UA_TYPES_BYTE);
		}
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}
