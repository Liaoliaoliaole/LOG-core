/*
File: Morfeas_IOBOX_nodeset.c, implementation of OPC-UA server's
construction/deconstruction functions for Morfeas IOBOX_handler.

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

#include <modbus.h>
//Include Functions implementation header
#include "../Morfeas_opc_ua/Morfeas_handlers_nodeset.h"

void IOBOX_handler_reg(UA_Server *server_ptr, char *Dev_or_Bus_name)
{
	int negative_one = -1;
	char tmp_buff[30], tmp_buff_1[50], tmp_buff_2[80];
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		sprintf(tmp_buff, "%s-if (%s)", Morfeas_IPC_handler_type_name[IOBOX], Dev_or_Bus_name);
		Morfeas_opc_ua_add_object_node(server_ptr, "IOBOX-ifs", Dev_or_Bus_name, tmp_buff);
		sprintf(tmp_buff, "%s.IP_addr", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "IPv4 Address", UA_TYPES_STRING);
		//Add IOBOX Dev name variable
		sprintf(tmp_buff, "%s.dev_name", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "IOBOX's Name", UA_TYPES_STRING);
		//Set Dev name		
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_buff), Dev_or_Bus_name, UA_TYPES_STRING);
		//Add status variable and set it to "Initializing"
		sprintf(tmp_buff, "%s.status", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "IOBOX's Status", UA_TYPES_STRING);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_buff), "Initializing", UA_TYPES_STRING);
		sprintf(tmp_buff, "%s.status_value", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "IOBOX's Status Value", UA_TYPES_INT32);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_buff), &negative_one, UA_TYPES_INT32);
		//Object with electric status of a IOBOX Induction link power supplies
		sprintf(tmp_buff, "%s.Ind_link", Dev_or_Bus_name);
		Morfeas_opc_ua_add_object_node(server_ptr, Dev_or_Bus_name, tmp_buff, "I-Link Power Supply");
		sprintf(tmp_buff_1, "%s.Vin", tmp_buff);
		Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff, tmp_buff_1, "Power Supply Vin(V)", UA_TYPES_FLOAT);
		for(unsigned char i=1; i<=4; i++)
		{
			sprintf(tmp_buff_2, "CH%1hhu", i);
			sprintf(tmp_buff_1, "%s.CH%1hhu", tmp_buff, i);
			Morfeas_opc_ua_add_object_node(server_ptr, tmp_buff, tmp_buff_1, tmp_buff_2);
			sprintf(tmp_buff_2, "%s.Vout", tmp_buff_1);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff_1, tmp_buff_2, "Vout(V)", UA_TYPES_FLOAT);
			sprintf(tmp_buff_2, "%s.Iout", tmp_buff_1);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_buff_1, tmp_buff_2, "Iout(A)", UA_TYPES_FLOAT);
		}
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void IPC_msg_from_IOBOX_handler(UA_Server *server, unsigned char type, IPC_message *IPC_msg_dec)
{
	UA_NodeId NodeId;
	char Node_ID_str[60];	
	//Msg type from IOBOX_handler
	switch(type)
	{
		case IPC_IOBOX_report:
			sprintf(Node_ID_str, "%s.status", IPC_msg_dec->IOBOX_report.Dev_or_Bus_name);
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				switch(IPC_msg_dec->IOBOX_report.status)
				{
					case 0:
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), "Okay", UA_TYPES_STRING);
						break;
					default:
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), modbus_strerror(IPC_msg_dec->IOBOX_report.status), UA_TYPES_STRING);
						break;
				}
				sprintf(Node_ID_str, "%s.status_value", IPC_msg_dec->IOBOX_report.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->IOBOX_report.status), UA_TYPES_INT32);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_IOBOX_data:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				//Load power supply measurements to OPC-UA variables
				sprintf(Node_ID_str, "%s.Ind_link.Vin", IPC_msg_dec->IOBOX_data.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->IOBOX_data.Supply_Vin), UA_TYPES_FLOAT);
				for(unsigned char i=0; i<4; i++)
				{
					sprintf(Node_ID_str, "%s.Ind_link.CH%1hhu.Vout", IPC_msg_dec->IOBOX_data.Dev_or_Bus_name, i+1);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->IOBOX_data.Supply_meas[i].Vout), UA_TYPES_FLOAT);
					sprintf(Node_ID_str, "%s.Ind_link.CH%1hhu.Iout", IPC_msg_dec->IOBOX_data.Dev_or_Bus_name, i+1);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->IOBOX_data.Supply_meas[i].Iout), UA_TYPES_FLOAT);
				}
				/*
				//Object with Receivers
				sprintf(tmp_buff, "%s.RXs", Dev_or_Bus_name);
				Morfeas_opc_ua_add_object_node(server, Dev_or_Bus_name, tmp_buff, "Receivers");
				for(unsigned char i=0; i<4; i++)
				{
					sprintf(tmp_buff_2, "CH%1hhu", i+1);
					sprintf(tmp_buff_1, "%s.CH%1hhu", tmp_buff, i+1);
					Morfeas_opc_ua_add_object_node(server, tmp_buff, tmp_buff_1, tmp_buff_2);
					//Variables of Channels measurements
					for(unsigned char j=0; j<16; j++)
					{
						sprintf(tmp_buff, "CH%hh01u", j+1);
						sprintf(tmp_buff_2, "IOBOX.%u.CH%1hhu", , j+1);
						Morfeas_opc_ua_add_variable_node(server, tmp_buff_1, tmp_buff_2, tmp_buff, UA_TYPES_FLOAT);
					}
					
				}
				*/
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
	}
}