/*
File: Morfeas_MTI_nodeset.c, implementation of OPC-UA server's
construction/deconstruction functions for Morfeas MTI_handler.

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

void MTI_handler_reg(UA_Server *server_ptr, char *Dev_or_Bus_name)
{
	int negative_one = -1;
	char Node_ID_str[30];
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		//Add MTI handler root node
		sprintf(Node_ID_str, "%s-if (%s)", Morfeas_IPC_handler_type_name[MTI], Dev_or_Bus_name);
		Morfeas_opc_ua_add_object_node(server_ptr, "MTI-ifs", Dev_or_Bus_name, Node_ID_str);
		sprintf(Node_ID_str, "%s.IP_addr", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "IPv4 Address", UA_TYPES_STRING);
		//Add MTI Device name variable
		sprintf(Node_ID_str, "%s.dev_name", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "Device Name", UA_TYPES_STRING);
		//Set Device name
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,Node_ID_str), Dev_or_Bus_name, UA_TYPES_STRING);
		//Add status variable and set it to "Initializing"
		sprintf(Node_ID_str, "%s.status", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "MTI Status", UA_TYPES_STRING);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,Node_ID_str), "Initializing", UA_TYPES_STRING);
		sprintf(Node_ID_str, "%s.status_value", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "MTI Status Value", UA_TYPES_INT32);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,Node_ID_str), &negative_one, UA_TYPES_INT32);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void IPC_msg_from_MTI_handler(UA_Server *server, unsigned char type, IPC_message *IPC_msg_dec)
{
	UA_NodeId NodeId;
	char Node_ID_str[50], Node_ID_parent_str[80]; 
	char MTI_IPv4_addr_str[20];
	//Msg type from MTI_handler
	switch(type)
	{
		case IPC_MTI_report:
			sprintf(Node_ID_str, "%s.status", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				switch(IPC_msg_dec->MTI_report.status)
				{
					case OK_status:
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), "Okay", UA_TYPES_STRING);
						break;
					default:
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), modbus_strerror(IPC_msg_dec->MTI_report.status), UA_TYPES_STRING);
						break;
				}
				sprintf(Node_ID_str, "%s.status_value", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_report.status), UA_TYPES_INT32);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_MTI_tree_reg:
			sprintf(Node_ID_str, "%s.IP_addr", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				//Update IPv4 variable
				inet_ntop(AF_INET, &(IPC_msg_dec->MTI_tree_reg.MTI_IPv4), MTI_IPv4_addr_str, sizeof(MTI_IPv4_addr_str));
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), MTI_IPv4_addr_str, UA_TYPES_STRING);
				//Add Object for MTI Health data
				sprintf(Node_ID_parent_str, "%s.health", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
				Morfeas_opc_ua_add_object_node(server, IPC_msg_dec->MTI_report.Dev_or_Bus_name, Node_ID_parent_str, "MTI Health");
				//Add variables to MTI Health node
				sprintf(Node_ID_str, "%s.CPU_temp", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "CPU Temperature(°C)", UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.batt_capacity", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Battery Capacity(%)", UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.batt_voltage", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Battery Voltage(V)", UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.batt_state", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Battery state", UA_TYPES_STRING);
				//Add Object for MTI Radio Config
				sprintf(Node_ID_parent_str, "%s.Radio", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
				Morfeas_opc_ua_add_object_node(server, IPC_msg_dec->MTI_report.Dev_or_Bus_name, Node_ID_parent_str, "Radio");
				//Add variables to MTI Radio Config
				sprintf(Node_ID_str, "%s.RF_CH", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "RF Channel", UA_TYPES_BYTE);
				sprintf(Node_ID_str, "%s.data_rate", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Modem Data Rate", UA_TYPES_STRING);
				sprintf(Node_ID_str, "%s.tele_dev_type", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Tele Dev Type", UA_TYPES_STRING);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_MTI_Update_Health:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				//Update MTI Health node variables
				sprintf(Node_ID_str, "%s.CPU_temp", IPC_msg_dec->MTI_Update_Health.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_Update_Health.cpu_temp), UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.batt_capacity", IPC_msg_dec->MTI_Update_Health.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_Update_Health.batt_capacity), UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.batt_voltage", IPC_msg_dec->MTI_Update_Health.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_Update_Health.batt_voltage), UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.batt_state", IPC_msg_dec->MTI_Update_Health.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), MTI_charger_state_str[IPC_msg_dec->MTI_Update_Health.batt_state], UA_TYPES_STRING);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_MTI_Update_Radio:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				//Update MTI Radio node variables
				sprintf(Node_ID_str, "%s.RF_CH", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_Update_Radio.RF_channel), UA_TYPES_BYTE);
				sprintf(Node_ID_str, "%s.data_rate", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), MTI_Data_rate_str[IPC_msg_dec->MTI_Update_Radio.Data_rate], UA_TYPES_STRING);
				sprintf(Node_ID_str, "%s.tele_dev_type", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), MTI_Tele_dev_type_str[IPC_msg_dec->MTI_Update_Radio.Tele_dev_type], UA_TYPES_STRING);
				//Check call with new configuration
				if(IPC_msg_dec->MTI_Update_Radio.new_config)
				{
					sprintf(Node_ID_str, "%s.Tele", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
					if(!UA_Server_readNodeId(server, UA_NODEID_STRING(1, Node_ID_str), &NodeId))
					{
						UA_Server_deleteNode(server, NodeId, 1);
						UA_clear(&NodeId, &UA_TYPES[UA_TYPES_NODEID]);
					}
					if(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type>=Dev_type_min && IPC_msg_dec->MTI_Update_Radio.Tele_dev_type<=Dev_type_max)
					{
						sprintf(Node_ID_parent_str, "%s.Radio", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
						Morfeas_opc_ua_add_object_node(server, Node_ID_parent_str, Node_ID_str, "Telemetries");
						switch(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type)
						{
							case Tele_TC16:
								break;
							case Tele_TC8:
								break;
							case Tele_TC4:
								break;
							case Tele_quad:
								break;
							case RM_SW_MUX:
								break;
						}
					}
				}
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_MTI_data:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
	}
}