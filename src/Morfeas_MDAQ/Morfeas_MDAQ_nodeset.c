/*
File: Morfeas_MDAQ_nodeset.c, implementation of OPC-UA server's
construction/deconstruction functions for Morfeas MDAQ_handler.

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
#include <arpa/inet.h>

#include <modbus.h>
//Include Functions implementation header
#include "../Morfeas_opc_ua/Morfeas_handlers_nodeset.h"

void MDAQ_handler_reg(UA_Server *server_ptr, char *Dev_or_Bus_name)
{
	int negative_one = -1;
	char tmp_buff[30];
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		sprintf(tmp_buff, "%s-if (%s)", Morfeas_IPC_handler_type_name[MDAQ], Dev_or_Bus_name);
		Morfeas_opc_ua_add_object_node(server_ptr, "MDAQ-ifs", Dev_or_Bus_name, tmp_buff);
		sprintf(tmp_buff, "%s.IP_addr", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "IPv4 Address", UA_TYPES_STRING);
		//Add MDAQ Device name variable
		sprintf(tmp_buff, "%s.dev_name", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "Device Name", UA_TYPES_STRING);
		//Set Device name		
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_buff), Dev_or_Bus_name, UA_TYPES_STRING);
		//Add status variable and set it to "Initializing"
		sprintf(tmp_buff, "%s.status", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "MDAQ Status", UA_TYPES_STRING);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_buff), "Initializing", UA_TYPES_STRING);
		sprintf(tmp_buff, "%s.status_value", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "MDAQ Status Value", UA_TYPES_INT32);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_buff), &negative_one, UA_TYPES_INT32);
		//Object with MDAQ Board status data 
		sprintf(tmp_buff, "%s.index", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "Index", UA_TYPES_FLOAT);
		sprintf(tmp_buff, "%s.board_temp", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, tmp_buff, "Board Temperature(°C)", UA_TYPES_FLOAT);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void IPC_msg_from_MDAQ_handler(UA_Server *server, unsigned char type, IPC_message *IPC_msg_dec)
{
	UA_NodeId NodeId;
	char Node_ID_str[60], MDAQ_IPv4_addr_str[20];	
	char tmp_buff[80], tmp_buff_1[50], tmp_buff_2[80];
	float nan = NAN;
	//Msg type from MDAQ_handler
	switch(type)
	{
		case IPC_MDAQ_report:
			sprintf(Node_ID_str, "%s.status", IPC_msg_dec->MDAQ_report.Dev_or_Bus_name);
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				switch(IPC_msg_dec->MDAQ_report.status)
				{
					case 0:
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), "Okay", UA_TYPES_STRING);
						break;
					default:
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), modbus_strerror(IPC_msg_dec->MDAQ_report.status), UA_TYPES_STRING);
						break;
				}
				sprintf(Node_ID_str, "%s.status_value", IPC_msg_dec->MDAQ_report.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MDAQ_report.status), UA_TYPES_INT32);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		/*
		case IPC_MDAQ_data:
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
				//Check if node for object Receivers exist 
				sprintf(Node_ID_str, "%s.RXs", IPC_msg_dec->IOBOX_data.Dev_or_Bus_name);
				if(UA_Server_readNodeId(server, UA_NODEID_STRING(1, Node_ID_str), &NodeId))
				{
					//Update IPv4 variable
					inet_ntop(AF_INET, &(IPC_msg_dec->IOBOX_data.IOBOX_IPv4), IOBOX_IPv4_addr_str, sizeof(IOBOX_IPv4_addr_str));
					sprintf(Node_ID_str, "%s.IP_addr", IPC_msg_dec->IOBOX_data.Dev_or_Bus_name);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), IOBOX_IPv4_addr_str, UA_TYPES_STRING);
					//Add Object for Receivers
					sprintf(Node_ID_str, "%s.RXs", IPC_msg_dec->IOBOX_data.Dev_or_Bus_name);
					Morfeas_opc_ua_add_object_node(server, IPC_msg_dec->IOBOX_data.Dev_or_Bus_name, Node_ID_str, "Receivers");
					for(unsigned char i=1; i<=4; i++)
					{
						sprintf(tmp_buff, "%s.RX%hhu", Node_ID_str, i);
						sprintf(tmp_buff_1, "RX%1hhu", i);
						Morfeas_opc_ua_add_object_node(server, Node_ID_str, tmp_buff, tmp_buff_1);
						//Variables of Channels measurements
						for(unsigned char j=1; j<=16; j++)
						{
							sprintf(tmp_buff_1, "IOBOX.%u.RX%hhu.CH%hhu", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i, j);
							sprintf(tmp_buff_2, "CH%02hhu", j);
							Morfeas_opc_ua_add_object_node(server, tmp_buff, tmp_buff_1, tmp_buff_2);
							sprintf(tmp_buff_2, "%s.meas", tmp_buff_1);
							Morfeas_opc_ua_add_variable_node(server, tmp_buff_1, tmp_buff_2, "Value", UA_TYPES_FLOAT);
							sprintf(tmp_buff_2, "%s.status", tmp_buff_1);
							Morfeas_opc_ua_add_variable_node(server, tmp_buff_1, tmp_buff_2, "Status", UA_TYPES_STRING);
							sprintf(tmp_buff_2, "%s.status_byte", tmp_buff_1);
							Morfeas_opc_ua_add_variable_node(server, tmp_buff_1, tmp_buff_2, "Status_value", UA_TYPES_BYTE);
						}
						sprintf(tmp_buff_2, "IOBOX.%u.RX%hhu.index", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i);
						Morfeas_opc_ua_add_variable_node(server, tmp_buff, tmp_buff_2, "Index", UA_TYPES_UINT16);
						sprintf(tmp_buff_2, "IOBOX.%u.RX%hhu.status", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i);
						Morfeas_opc_ua_add_variable_node(server, tmp_buff, tmp_buff_2, "RX_Status", UA_TYPES_BYTE);
						sprintf(tmp_buff_2, "IOBOX.%u.RX%hhu.success", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i);
						Morfeas_opc_ua_add_variable_node(server, tmp_buff, tmp_buff_2, "RX_success", UA_TYPES_BYTE);
					}
				}
				else
					UA_clear(&NodeId, &UA_TYPES[UA_TYPES_NODEID]);
				//Load values to variables 
				for(unsigned char i=0; i<4; i++)
				{
					sprintf(tmp_buff, "%s.RX%hhu", Node_ID_str, i);
					//Variables of Channels measurements
					for(unsigned char j=0; j<16; j++)
					{
						sprintf(tmp_buff_1, "IOBOX.%u.RX%hhu.CH%hhu", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i+1, j+1);
						sprintf(tmp_buff_2, "%s.meas", tmp_buff_1);
						if(IPC_msg_dec->IOBOX_data.RX[i].CH_value[j] < 2000.0)
							Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,tmp_buff_2), 
															   &(IPC_msg_dec->IOBOX_data.RX[i].CH_value[j]), UA_TYPES_FLOAT);
						else
							Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,tmp_buff_2), 
															   &nan, UA_TYPES_FLOAT);
					}
					//Variables for receiver 
					sprintf(tmp_buff_2, "IOBOX.%u.RX%hhu.index", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i+1);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,tmp_buff_2), &(IPC_msg_dec->IOBOX_data.RX[i].index), UA_TYPES_UINT16);
					sprintf(tmp_buff_2, "IOBOX.%u.RX%hhu.status", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i+1);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,tmp_buff_2), &(IPC_msg_dec->IOBOX_data.RX[i].status), UA_TYPES_BYTE);
					sprintf(tmp_buff_2, "IOBOX.%u.RX%hhu.success", IPC_msg_dec->IOBOX_data.IOBOX_IPv4, i+1);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,tmp_buff_2), &(IPC_msg_dec->IOBOX_data.RX[i].success), UA_TYPES_BYTE);
				}
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		*/
	}
}