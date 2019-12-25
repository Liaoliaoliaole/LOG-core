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

#include <math.h>

//Include Functions implementation header
#include "Morfeas_handlers_nodeset.h"

pthread_mutex_t OPC_UA_NODESET_access = PTHREAD_MUTEX_INITIALIZER;

void IPC_msg_from_SDAQ_handler(UA_Server *server, unsigned char type,IPC_message *IPC_msg_dec)
{
	UA_NodeId NodeId;
	UA_DateTime cal_time;
	UA_DateTimeStruct calibration_date = {0};
	char Node_ID_str[60], meas_status_str[60];
	//Msg type from SDAQ_handler
	switch(type)
	{
		case IPC_SDAQ_meas:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.unit", IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
													   IPC_msg_dec->SDAQ_meas.channel);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   unit_str[IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.unit],
												   UA_TYPES_STRING);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.timestamp", IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
														 	IPC_msg_dec->SDAQ_meas.channel);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   &(IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.timestamp),
												   UA_TYPES_UINT16);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.status", IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
														 IPC_msg_dec->SDAQ_meas.channel);
				sprintf(meas_status_str, "%s%s", !IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.status?"Okay":
												 Channel_status_byte_dec(IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.status),
												 IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.unit<Unit_code_base_region_size ? ", Un-calibrated":"");
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   meas_status_str,
												   UA_TYPES_STRING);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.status_byte", IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
														 IPC_msg_dec->SDAQ_meas.channel);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   &(IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.status),
												   UA_TYPES_BYTE);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.meas",IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
													  IPC_msg_dec->SDAQ_meas.channel);
				if(IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.status&(1<<No_sensor))
					IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.meas = NAN;
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   &(IPC_msg_dec->SDAQ_meas.SDAQ_channel_meas.meas),
												   UA_TYPES_FLOAT);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_CAN_BUS_info:
			sprintf(Node_ID_str, "%s.BUS_util", IPC_msg_dec->BUS_info.connected_to_BUS);
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->BUS_info.BUS_utilization), UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.volts", IPC_msg_dec->BUS_info.connected_to_BUS);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->BUS_info.voltage), UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.amps", IPC_msg_dec->BUS_info.connected_to_BUS);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->BUS_info.amperage), UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.shunt", IPC_msg_dec->BUS_info.connected_to_BUS);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->BUS_info.shunt_temp), UA_TYPES_FLOAT);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_SDAQ_register_or_update:
			SDAQ2OPC_UA_register_update(server, (SDAQ_reg_update_msg*)IPC_msg_dec);//mutex inside
			break;
		case IPC_SDAQ_clean_up:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				sprintf(Node_ID_str,"%s.amount",IPC_msg_dec->SDAQ_clean.connected_to_BUS);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->SDAQ_clean.t_amount), UA_TYPES_BYTE);
				UA_NodeId_init(&NodeId);
				sprintf(Node_ID_str, "%s.%d", IPC_msg_dec->SDAQ_clean.connected_to_BUS, IPC_msg_dec->SDAQ_clean.SDAQ_serial_number);
				//check if the node is already removed
				if(!UA_Server_readNodeId(server, UA_NODEID_STRING(1, Node_ID_str), &NodeId))
					UA_Server_deleteNode(server, NodeId, 1);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_SDAQ_info:
			SDAQ2OPC_UA_register_update_info(server, (SDAQ_info_msg*)IPC_msg_dec);//mutex inside
			break;
		case IPC_SDAQ_cal_date:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.Cal_date", IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
														   IPC_msg_dec->SDAQ_meas.channel);
				calibration_date.day = !IPC_msg_dec->SDAQ_cal_date.SDAQ_cal_date.day?1:IPC_msg_dec->SDAQ_cal_date.SDAQ_cal_date.day;
				calibration_date.month = !IPC_msg_dec->SDAQ_cal_date.SDAQ_cal_date.month?1:IPC_msg_dec->SDAQ_cal_date.SDAQ_cal_date.month;
				calibration_date.year = IPC_msg_dec->SDAQ_cal_date.SDAQ_cal_date.year + 2000;
				cal_time = UA_DateTime_fromStruct(calibration_date);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   &cal_time,
												   UA_TYPES_DATETIME);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.period", IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
														 IPC_msg_dec->SDAQ_meas.channel);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   &(IPC_msg_dec->SDAQ_cal_date.SDAQ_cal_date.period),
												   UA_TYPES_BYTE);
				sprintf(Node_ID_str, "SDAQ.%d.CH%hhu.points", IPC_msg_dec->SDAQ_meas.SDAQ_serial_number,
														 IPC_msg_dec->SDAQ_meas.channel);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
												   &(IPC_msg_dec->SDAQ_cal_date.SDAQ_cal_date.amount_of_points),
												   UA_TYPES_BYTE);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_SDAQ_timediff:
			sprintf(Node_ID_str, "SDAQ.%d.TimeDiff", IPC_msg_dec->SDAQ_timediff.SDAQ_serial_number);
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->SDAQ_timediff.Timediff), UA_TYPES_UINT16);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
	}
}

void SDAQ_handler_reg(UA_Server *server_ptr, char *connected_to_BUS)
{
	char tmp_buff[30], tmp_buff_1[30], zero=0;
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		sprintf(tmp_buff, "%s-if (%s)", Morfeas_IPC_handler_type_name[SDAQ], connected_to_BUS);
		Morfeas_opc_ua_add_object_node(server_ptr, "SDAQ-ifs", connected_to_BUS, tmp_buff);
		sprintf(tmp_buff, "%s.SDAQnet", connected_to_BUS);
		Morfeas_opc_ua_add_object_node(server_ptr, connected_to_BUS, tmp_buff, "SDAQnet");
		sprintf(tmp_buff, "%s.BUS_util", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, connected_to_BUS, tmp_buff, "BUS_Util (%)", UA_TYPES_FLOAT);
		sprintf(tmp_buff, "%s.amount", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, connected_to_BUS, tmp_buff, "Dev_on_BUS", UA_TYPES_BYTE);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_buff), &zero, UA_TYPES_BYTE);
		//Object with electric status of a SDAQnet port
		sprintf(tmp_buff, "%s.Electrics", connected_to_BUS);
		Morfeas_opc_ua_add_object_node(server_ptr, connected_to_BUS, tmp_buff, "Electric");
		sprintf(tmp_buff_1, "%s.volts", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff, tmp_buff_1, "Voltage (V)", UA_TYPES_FLOAT);
		sprintf(tmp_buff_1, "%s.amps", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff, tmp_buff_1, "Amperage (A)", UA_TYPES_FLOAT);
		sprintf(tmp_buff_1, "%s.shunt", connected_to_BUS);
		Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff, tmp_buff_1, "Shunt Temp (°C)", UA_TYPES_FLOAT);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void SDAQ2OPC_UA_register_update_info(UA_Server *server_ptr, SDAQ_info_msg *ptr)
{
	const char init_val = -1;
	char SDAQ_anchor_str[15], tmp_str[50], tmp_str2[50], tmp_str3[70];
	UA_NodeId out;
	//UA_NodeId_init(&out);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		sprintf(SDAQ_anchor_str,"%s.%d", ptr->connected_to_BUS, ptr->SDAQ_serial_number);
		if(!UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, SDAQ_anchor_str), &out))
		{
			//UA_NodeId_init(&out);
			sprintf(tmp_str,"SDAQ.%d.Info",ptr->SDAQ_serial_number);
			if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, tmp_str), &out))
			{
				Morfeas_opc_ua_add_object_node(server_ptr, SDAQ_anchor_str, tmp_str, "Info");
				sprintf(tmp_str2,"SDAQ.%d.Type",ptr->SDAQ_serial_number);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Type", UA_TYPES_STRING);
				sprintf(tmp_str2,"SDAQ.%d.Firm_Rev",ptr->SDAQ_serial_number);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Firm_Rev", UA_TYPES_BYTE);
				sprintf(tmp_str2,"SDAQ.%d.Hw_Rev",ptr->SDAQ_serial_number);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Hw_Rev", UA_TYPES_BYTE);
				sprintf(tmp_str2,"SDAQ.%d.Amount_of_channels",ptr->SDAQ_serial_number);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Channels_on_SDAQ", UA_TYPES_BYTE);
				sprintf(tmp_str2,"SDAQ.%d.Samplerate",ptr->SDAQ_serial_number);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Samplerate", UA_TYPES_BYTE);
				sprintf(tmp_str2,"SDAQ.%d.Max_cal_points",ptr->SDAQ_serial_number);
				Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str, tmp_str2, "Max_cal_points", UA_TYPES_BYTE);
				sprintf(tmp_str,"SDAQ.%d.Channels",ptr->SDAQ_serial_number);
				Morfeas_opc_ua_add_object_node(server_ptr, SDAQ_anchor_str, tmp_str, "Channels");
			}
			sprintf(tmp_str,"SDAQ.%d.Channels",ptr->SDAQ_serial_number);
			for(unsigned char i=1; i <= ptr->SDAQ_info_data.num_of_ch; i++)
			{
				//UA_NodeId_init(&out);
				sprintf(tmp_str2,"SDAQ.%d.CH%hhu", ptr->SDAQ_serial_number, i);
				if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, tmp_str2), &out))
				{
					sprintf(tmp_str3,"CH%02hhu", i);
					Morfeas_opc_ua_add_object_node(server_ptr, tmp_str, tmp_str2, tmp_str3);
					sprintf(tmp_str3,"%s.meas", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Value", UA_TYPES_FLOAT);
					sprintf(tmp_str3,"%s.timestamp", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Timestamp", UA_TYPES_UINT16);
					sprintf(tmp_str3,"%s.status_byte", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Status Value", UA_TYPES_BYTE);
					Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str3), &init_val, UA_TYPES_BYTE);
					sprintf(tmp_str3,"%s.status", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Status", UA_TYPES_STRING);
					Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str3), "Initializing", UA_TYPES_STRING);
					sprintf(tmp_str3,"%s.unit", tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Unit", UA_TYPES_STRING);
					sprintf(tmp_str3,"%s.Cal_date",tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Calibration Date", UA_TYPES_DATETIME);
					sprintf(tmp_str3,"%s.period",tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Period (Months)", UA_TYPES_BYTE);
					sprintf(tmp_str3,"%s.points",tmp_str2);
					Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str3, "Calibration Points", UA_TYPES_BYTE);
				}
			}
			sprintf(tmp_str,"SDAQ.%d.Type",ptr->SDAQ_serial_number);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), (char *)dev_type_str[ptr->SDAQ_info_data.dev_type], UA_TYPES_STRING);
			sprintf(tmp_str,"SDAQ.%d.Firm_Rev",ptr->SDAQ_serial_number);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.firm_rev), UA_TYPES_BYTE);
			sprintf(tmp_str,"SDAQ.%d.Hw_Rev",ptr->SDAQ_serial_number);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.hw_rev), UA_TYPES_BYTE);
			sprintf(tmp_str,"SDAQ.%d.Amount_of_channels",ptr->SDAQ_serial_number);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.num_of_ch), UA_TYPES_BYTE);
			sprintf(tmp_str,"SDAQ.%d.Samplerate",ptr->SDAQ_serial_number);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.sample_rate), UA_TYPES_BYTE);
			sprintf(tmp_str,"SDAQ.%d.Max_cal_points",ptr->SDAQ_serial_number);
			Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_info_data.max_cal_point), UA_TYPES_BYTE);
		}
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

// Function that find if an SDAQ is registered on a SDAQnet handler other than the on_Bus. Returns: the name string of the Bus or NULL if it's not find or if SDAQ registered on on_Bus
char * find_if_SDAQ_is_registered(UA_Server *server_ptr, const unsigned int serial_number, const char * on_Bus)
{
	char Node_id_str[50], *retval;
	UA_Variant res_Value;
	UA_String *Value, UA_str_on_bus = UA_String_fromChars(on_Bus);
	sprintf(Node_id_str, "SDAQ.%d.onBus", serial_number);
	//Return NULL, because the SDAQ is not registered.
	if(UA_Server_readValue(server_ptr,  UA_NODEID_STRING(1, Node_id_str), &res_Value))
		return NULL;
	if(res_Value.type->typeIndex == UA_DATATYPEKIND_STRING)
	{
		Value = res_Value.data;
		if(!UA_String_equal(Value, &UA_str_on_bus))
		{
			retval = calloc(Value->length+1, sizeof(char));
			memcpy(retval, Value->data, Value->length);
			retval[Value->length] = '\0';
			return retval;
		}
	}
	return NULL;
}

void SDAQ2OPC_UA_register_update(UA_Server *server_ptr, SDAQ_reg_update_msg *ptr)
{
	char SDAQ_anchor_str[15], tmp_str[50], tmp_str2[50], *pre_reg_on_bus;
	UA_NodeId Node_Id;
	//UA_NodeId_init(&Node_Id);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		//Build SDAQ_anchor
		sprintf(SDAQ_anchor_str,"%s.%d",ptr->connected_to_BUS,ptr->SDAQ_status.dev_sn);
		//Check if SDAQ is pre-register on other bus
		if((pre_reg_on_bus = find_if_SDAQ_is_registered(server_ptr, ptr->SDAQ_status.dev_sn, ptr->connected_to_BUS)))
		{
			sprintf(tmp_str,"%s.%d", pre_reg_on_bus, ptr->SDAQ_status.dev_sn);
			UA_Server_deleteNode(server_ptr, UA_NODEID_STRING(1, tmp_str), 1);
			free(pre_reg_on_bus);
		}
		//Check if the Node with the SDAQ's data is already exist, if no add it, elsewhere update data only
		if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, SDAQ_anchor_str), &Node_Id))
		{
			//SDAQ's object
			sprintf(tmp_str,"%s.SDAQnet",ptr->connected_to_BUS);
			sprintf(tmp_str2,"%s (%02hhu)", (char *)dev_type_str[ptr->SDAQ_status.dev_type], ptr->address);
			Morfeas_opc_ua_add_object_node(server_ptr, tmp_str, SDAQ_anchor_str, tmp_str2);
			//SDAQ's SDAQnet info and Timediff
			sprintf(tmp_str,"SDAQ.%d.onBus",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "Connected on Bus", UA_TYPES_STRING);
			sprintf(tmp_str,"SDAQ.%d.S/N",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "S/N", UA_TYPES_UINT32);
			sprintf(tmp_str,"SDAQ.%d.Address",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "Address", UA_TYPES_BYTE);
			sprintf(tmp_str,"SDAQ.%d.TimeDiff",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "TimeDiff", UA_TYPES_UINT16);
			//SDAQ's Info object and Info Variables
			sprintf(tmp_str2,"SDAQ.%d.Status",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_object_node(server_ptr, SDAQ_anchor_str, tmp_str2, "Status");
			sprintf(tmp_str,"SDAQ.%d.State",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "State", UA_TYPES_STRING);
			sprintf(tmp_str,"SDAQ.%d.inSync",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "inSync", UA_TYPES_STRING);
			sprintf(tmp_str,"SDAQ.%d.Error",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "Error", UA_TYPES_STRING);
			sprintf(tmp_str,"SDAQ.%d.Mode",ptr->SDAQ_status.dev_sn);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "Mode", UA_TYPES_STRING);
		}
		sprintf(tmp_str,"%s.amount",ptr->connected_to_BUS);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->t_amount), UA_TYPES_BYTE);
		sprintf(tmp_str,"SDAQ.%d.onBus",ptr->SDAQ_status.dev_sn);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), ptr->connected_to_BUS, UA_TYPES_STRING);
		sprintf(tmp_str,"SDAQ.%d.S/N",ptr->SDAQ_status.dev_sn);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_status.dev_sn), UA_TYPES_UINT32);
		sprintf(tmp_str,"SDAQ.%d.Address",ptr->SDAQ_status.dev_sn);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->address), UA_TYPES_BYTE);
		sprintf(tmp_str,"SDAQ.%d.State",ptr->SDAQ_status.dev_sn);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, State), UA_TYPES_STRING);
		sprintf(tmp_str,"SDAQ.%d.inSync",ptr->SDAQ_status.dev_sn);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, In_sync), UA_TYPES_STRING);
		sprintf(tmp_str,"SDAQ.%d.Error",ptr->SDAQ_status.dev_sn);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Error), UA_TYPES_STRING);
		sprintf(tmp_str,"SDAQ.%d.Mode",ptr->SDAQ_status.dev_sn);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Mode), UA_TYPES_STRING);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}
