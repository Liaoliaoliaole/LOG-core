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
#include <cjson/cJSON.h>
//Include Functions implementation header
#include "../Morfeas_opc_ua/Morfeas_handlers_nodeset.h"

//Local function that adding the new_MTI_config method to the Morfeas OPC_UA nodeset
void Morfeas_add_new_MTI_config(UA_Server *server_ptr, char *Parent_id, char *Node_id);

//The DBus method caller function
int Morfeas_MTI_DBus_method_call(const char *handler_type, const char *dev_name, const char *method, const char *contents, UA_String *reply);


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
	char Node_ID_str[100], Node_ID_parent_str[60];
	char *status_str = NULL, name_buff[20], anchor[50];
	unsigned char i, lim = 0, status_value;
	float meas, ref;
	int cnt;
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
						if(IPC_msg_dec->MTI_report.Tele_dev_type>=Dev_type_min && IPC_msg_dec->MTI_report.Tele_dev_type<=Dev_type_max)
						{
							meas = NAN;
							ref = NAN;
							cnt = 0;
							status_value = OFF_line;
							status_str = "OFF-line";
							//Update values from MTI's Special registers
							switch(IPC_msg_dec->MTI_report.Tele_dev_type)
							{
								case Tele_TC16: lim = 16; break;
								case Tele_TC8:  lim =  8; break;
								case Tele_TC4:  lim =  4; break;
								case Tele_quad: lim =  2; break;
							}
							sprintf(anchor, "MTI.%u.%s", IPC_msg_dec->MTI_report.MTI_IPv4, MTI_Tele_dev_type_str[IPC_msg_dec->MTI_report.Tele_dev_type]);
							for(i=1; i<=lim; i++)
							{
								//Update telemetry's Channel specific variables (Linkable)
								sprintf(Node_ID_str, "%s.CH%02u.meas", anchor, i);
								Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &meas, UA_TYPES_FLOAT);
								sprintf(Node_ID_str, "%s.CH%02u.status", anchor, i);
								Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), status_str, UA_TYPES_STRING);
								sprintf(Node_ID_str, "%s.CH%02u.status_byte", anchor, i);
								Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BYTE);
								switch(IPC_msg_dec->MTI_report.Tele_dev_type)
								{
									case Tele_TC8:
									case Tele_TC4:
										sprintf(Node_ID_str, "%s.Radio.Tele.CH%02u.ref", IPC_msg_dec->MTI_report.Dev_or_Bus_name, i);
										Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &ref, UA_TYPES_FLOAT);
										break;
									case Tele_quad:
										sprintf(Node_ID_str, "%s.Radio.Tele.CH%02u.raw", IPC_msg_dec->MTI_report.Dev_or_Bus_name, i);
										Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &cnt, UA_TYPES_INT32);
										break;
								}
							}
						}
						break;
				}
				sprintf(Node_ID_str, "%s.status_value", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_report.status), UA_TYPES_INT32);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_MTI_tree_reg:
			sprintf(Node_ID_str, "%s.IP_addr", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				//Update IPv4 variable
				inet_ntop(AF_INET, &(IPC_msg_dec->MTI_tree_reg.MTI_IPv4), name_buff, sizeof(name_buff));
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), name_buff, UA_TYPES_STRING);
				//Add Object for MTI Health data
				sprintf(Node_ID_parent_str, "%s.health", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_object_node(server, IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name, Node_ID_parent_str, "MTI Health");
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
				sprintf(Node_ID_parent_str, "%s.Radio", IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name);
				Morfeas_opc_ua_add_object_node(server, IPC_msg_dec->MTI_tree_reg.Dev_or_Bus_name, Node_ID_parent_str, "Radio");
				//Add variables to MTI Radio Config
				sprintf(Node_ID_str, "%s.RF_CH", Node_ID_parent_str);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "RF Channel", UA_TYPES_BYTE);
				sprintf(Node_ID_str, "%s.Data_rate", Node_ID_parent_str);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Data Rate", UA_TYPES_STRING);
				sprintf(Node_ID_str, "%s.Tele_dev_type", Node_ID_parent_str);
				Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Tele Dev Type", UA_TYPES_STRING);
				//Add new_MTI_config method
				sprintf(Node_ID_str, "%s.new_MTI_config()", Node_ID_parent_str);
				Morfeas_add_new_MTI_config(server, Node_ID_parent_str, Node_ID_str);
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
				sprintf(Node_ID_str, "%s.Radio.RF_CH", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_Update_Radio.RF_channel), UA_TYPES_BYTE);
				sprintf(Node_ID_str, "%s.Radio.Data_rate", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), MTI_Data_rate_str[IPC_msg_dec->MTI_Update_Radio.Data_rate], UA_TYPES_STRING);
				sprintf(Node_ID_str, "%s.Radio.Tele_dev_type", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), MTI_Tele_dev_type_str[IPC_msg_dec->MTI_Update_Radio.Tele_dev_type], UA_TYPES_STRING);
				//Check if call is with a new configuration
				if(IPC_msg_dec->MTI_Update_Radio.isNew_config)
				{
					sprintf(Node_ID_str, "%s.Radio.Tele", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
					if(!UA_Server_readNodeId(server, UA_NODEID_STRING(1, Node_ID_str), &NodeId))
					{
						UA_Server_deleteNode(server, NodeId, 1);
						UA_clear(&NodeId, &UA_TYPES[UA_TYPES_NODEID]);
					}
					if(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type>=Dev_type_min && IPC_msg_dec->MTI_Update_Radio.Tele_dev_type<=Dev_type_max)
					{
						sprintf(Node_ID_parent_str, "%s.Radio", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
						sprintf(name_buff, "Tele(%s)", MTI_Tele_dev_type_str[IPC_msg_dec->MTI_Update_Radio.Tele_dev_type]);
						Morfeas_opc_ua_add_object_node(server, Node_ID_parent_str, Node_ID_str, name_buff);
						if(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type != RMSW_MUX)
						{
							strcpy(Node_ID_parent_str, Node_ID_str);
							//Add telemetry related variables
							sprintf(Node_ID_str, "%s.index", Node_ID_parent_str);
							Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Packet Index", UA_TYPES_UINT16);
							sprintf(Node_ID_str, "%s.RX_status", Node_ID_parent_str);
							Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "RX status", UA_TYPES_BYTE);
							sprintf(Node_ID_str, "%s.success", Node_ID_parent_str);
							Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "RX Success ratio(%)", UA_TYPES_BYTE);
							sprintf(Node_ID_str, "%s.isValid", Node_ID_parent_str);
							if(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type != Tele_quad)
							{
								Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "isValid", UA_TYPES_BOOLEAN);
								sprintf(Node_ID_str, "%s.StV", Node_ID_parent_str);
								Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Samples to Validate", UA_TYPES_BYTE);
								sprintf(Node_ID_str, "%s.StF", Node_ID_parent_str);
								Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Samples to Fail", UA_TYPES_BYTE);
							}
							//Add telemetry specific variables
							sprintf(anchor, "MTI.%u.%s", IPC_msg_dec->MTI_Update_Radio.MTI_IPv4, MTI_Tele_dev_type_str[IPC_msg_dec->MTI_Update_Radio.Tele_dev_type]);
							switch(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type)
							{
								case Tele_TC16: lim = 16; break;
								case Tele_TC8:  lim =  8; break;
								case Tele_TC4:  lim =  4; break;
								case Tele_quad: lim =  2; break;
							}
							for(i=1; i<=lim; i++)
							{
								//Add telemetry's Channel node
								sprintf(Node_ID_parent_str, "%s.Radio.Tele", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
								sprintf(name_buff, "CH%02u", i);
								sprintf(Node_ID_str, "%s.%s", Node_ID_parent_str, name_buff);
								Morfeas_opc_ua_add_object_node(server, Node_ID_parent_str, Node_ID_str, name_buff);
								//Add telemetry's Channel specific variables (Linkable)
								sprintf(Node_ID_parent_str, "%s.Radio.Tele.%s", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name, name_buff);
								sprintf(Node_ID_str, "%s.%s.status", anchor, name_buff);
								Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Status", UA_TYPES_STRING);
								sprintf(Node_ID_str, "%s.%s.status_byte", anchor, name_buff);
								Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Status Value", UA_TYPES_BYTE);
								sprintf(Node_ID_str, "%s.%s.meas", anchor, name_buff);
								Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Value", UA_TYPES_FLOAT);
								//Add telemetry's Channel specific variables (Non Linkable)
								sprintf(Node_ID_parent_str, "%s.Radio.Tele.%s", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name, name_buff);
								switch(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type)
								{
									case Tele_TC8:
									case Tele_TC4:
										//Add temperature reference channel
										sprintf(Node_ID_str, "%s.ref", Node_ID_parent_str);
										Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Reference", UA_TYPES_FLOAT);
										break;
									case Tele_quad:
										//Add Raw counter channel
										sprintf(Node_ID_str, "%s.raw", Node_ID_parent_str);
										Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Raw Counter", UA_TYPES_INT32);
										break;
								}
							}
						}
						else
						{	//Add RMSW/MUX related Global switches nodes
							sprintf(Node_ID_parent_str, "%s.Radio.Tele", IPC_msg_dec->MTI_report.Dev_or_Bus_name);
							sprintf(Node_ID_str, "%s.G_SW", Node_ID_parent_str);
							Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Global ON/OFF Control", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.G_SL", Node_ID_parent_str);
							Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Global Sleep Control", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.G_P_state", Node_ID_parent_str);
							Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Global ON/OFF state", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.G_S_state", Node_ID_parent_str);
							Morfeas_opc_ua_add_variable_node(server, Node_ID_parent_str, Node_ID_str, "Global Sleep state", UA_TYPES_BOOLEAN);
						}
					}
				}
				//Update values from MTI's Special registers
				switch(IPC_msg_dec->MTI_Update_Radio.Tele_dev_type)
				{
					case Tele_TC4:
					case Tele_TC8:
					case Tele_TC16:
					case Tele_quad:
						sprintf(Node_ID_str, "%s.Radio.Tele.StV", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_Update_Radio.sRegs.for_temp_tele.StV), UA_TYPES_BYTE);
						sprintf(Node_ID_str, "%s.Radio.Tele.StF", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_Update_Radio.sRegs.for_temp_tele.StF), UA_TYPES_BYTE);
						break;
					case RMSW_MUX:
						sprintf(Node_ID_str, "%s.Radio.Tele.G_SW", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
						status_value = IPC_msg_dec->MTI_Update_Radio.sRegs.for_rmsw_dev.G_SW;
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BOOLEAN);
						sprintf(Node_ID_str, "%s.Radio.Tele.G_SL", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
						status_value = IPC_msg_dec->MTI_Update_Radio.sRegs.for_rmsw_dev.G_SL;
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BOOLEAN);
						sprintf(Node_ID_str, "%s.Radio.Tele.G_P_state", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
						status_value = IPC_msg_dec->MTI_Update_Radio.sRegs.for_rmsw_dev.G_P_state;
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BOOLEAN);
						sprintf(Node_ID_str, "%s.Radio.Tele.G_S_state", IPC_msg_dec->MTI_Update_Radio.Dev_or_Bus_name);
						status_value = IPC_msg_dec->MTI_Update_Radio.sRegs.for_rmsw_dev.G_S_state;
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BOOLEAN);
						break;
				}
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_MTI_Tele_data:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				if(IPC_msg_dec->MTI_tele_data.Tele_dev_type>=Dev_type_min&&
				   IPC_msg_dec->MTI_tele_data.Tele_dev_type<=Dev_type_max&&
				   IPC_msg_dec->MTI_tele_data.Tele_dev_type!= RMSW_MUX)
				{
					sprintf(Node_ID_parent_str, "%s.Radio", IPC_msg_dec->MTI_tele_data.Dev_or_Bus_name);
					//Update telemetry related variables. (All memory position at same order, TC4 used as reference
					sprintf(Node_ID_str, "%s.Tele.index", Node_ID_parent_str);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_tele_data.data.as_TC4.packet_index), UA_TYPES_UINT16);
					sprintf(Node_ID_str, "%s.Tele.RX_status", Node_ID_parent_str);
					status_value=IPC_msg_dec->MTI_tele_data.data.as_TC4.RX_status;
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BYTE);
					sprintf(Node_ID_str, "%s.Tele.success", Node_ID_parent_str);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->MTI_tele_data.data.as_TC4.RX_Success_ratio), UA_TYPES_BYTE);
					sprintf(Node_ID_str, "%s.Tele.isValid", Node_ID_parent_str);
					status_value=IPC_msg_dec->MTI_tele_data.data.as_TC4.Data_isValid;
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BOOLEAN);
					switch(IPC_msg_dec->MTI_tele_data.Tele_dev_type)
					{
						case Tele_TC16: lim = 16; break;
						case Tele_TC8:  lim =  8; break;
						case Tele_TC4:  lim =  4; break;
						case Tele_quad: lim =  2; break;
					}
					sprintf(anchor, "MTI.%u.%s", IPC_msg_dec->MTI_tele_data.MTI_IPv4, MTI_Tele_dev_type_str[IPC_msg_dec->MTI_tele_data.Tele_dev_type]);
					for(i=1; i<=lim; i++)
					{	//Get and decode Telemetry's data;
						if(IPC_msg_dec->MTI_tele_data.data.as_TC4.Data_isValid)
						{
							switch(IPC_msg_dec->MTI_tele_data.Tele_dev_type)
							{
								case Tele_TC16:
									meas = IPC_msg_dec->MTI_tele_data.data.as_TC16.CHs[i-1];
									break;
								case Tele_TC8:
									meas = IPC_msg_dec->MTI_tele_data.data.as_TC8.CHs[i-1];
									ref = IPC_msg_dec->MTI_tele_data.data.as_TC8.Refs[i-1];
									break;
								case Tele_TC4:
									meas = IPC_msg_dec->MTI_tele_data.data.as_TC4.CHs[i-1];
									ref = (i>=1&&i<=2)?IPC_msg_dec->MTI_tele_data.data.as_TC4.Refs[0]:
													   IPC_msg_dec->MTI_tele_data.data.as_TC4.Refs[1];
									break;
								case Tele_quad:
									meas = IPC_msg_dec->MTI_tele_data.data.as_QUAD.CHs[i-1];
									cnt = IPC_msg_dec->MTI_tele_data.data.as_QUAD.CNTs[i-1];
									break;
							}
							status_value = Okay;
							status_str = "Okay";
							if(IPC_msg_dec->MTI_tele_data.Tele_dev_type != Tele_quad)
							{
								if(meas >= NO_SENSOR_VALUE)//Check for No sensor value
								{
									status_str = "No Sensor";
									status_value = Tele_channel_noSensor;
									meas = NAN;
									ref = NAN;
								}
								else if(meas != meas)//Check for Telemetry Error (meas == NAN)
								{
									status_str = "Error";
									status_value = Tele_channel_Error;
								}
							}
						}
						else
						{
							status_value = Disconnected;
							status_str = "Disconnected";
						}
						//Update telemetry's Channel specific variables (Linkable)
						sprintf(Node_ID_str, "%s.CH%02u.meas", anchor, i);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &meas, UA_TYPES_FLOAT);
						sprintf(Node_ID_str, "%s.CH%02u.status", anchor, i);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), status_str, UA_TYPES_STRING);
						sprintf(Node_ID_str, "%s.CH%02u.status_byte", anchor, i);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &status_value, UA_TYPES_BYTE);
						//Update telemetry's Channel specific variables (Non Linkable)
						switch(IPC_msg_dec->MTI_tele_data.Tele_dev_type)
						{
							case Tele_TC8:
							case Tele_TC4:
								sprintf(Node_ID_str, "%s.Radio.Tele.CH%02u.ref", IPC_msg_dec->MTI_tele_data.Dev_or_Bus_name, i);
								Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &ref, UA_TYPES_FLOAT);
								break;
							case Tele_quad:
								sprintf(Node_ID_str, "%s.Radio.Tele.CH%02u.raw", IPC_msg_dec->MTI_tele_data.Dev_or_Bus_name, i);
								Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &cnt, UA_TYPES_INT32);
								break;
						}
					}
				}
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_MTI_RMSW_MUX_data:
			pthread_mutex_lock(&OPC_UA_NODESET_access);

			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
	}
}

UA_StatusCode Morfeas_new_MTI_config_method_callback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output)
{
	char contents[200], dev_name[Dev_or_Bus_name_str_size];
    int ret = UA_STATUSCODE_GOOD, i=0;
	cJSON *root = NULL;
	UA_String reply;

	//Get Dev_name
	while(methodId->identifier.string.data[i]!='.')
	{
		dev_name[i]=methodId->identifier.string.data[i];
		i++;
	}
	dev_name[i]='\0';
	//Construct JSON object (used as contents)
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "new_RF_CH", 16);
	cJSON_AddItemToObject(root, "new_mode", cJSON_CreateString("TC8"));
	cJSON_AddNumberToObject(root, "StV", 50);
	cJSON_AddNumberToObject(root, "StF", 30);
	cJSON_AddItemToObject(root, "G_SW", cJSON_CreateBool(0));
	cJSON_AddItemToObject(root, "G_SL", cJSON_CreateBool(0));
	//Stringify the root JSON object
	cJSON_PrintPreallocated(root, contents, sizeof(contents), 0);
	cJSON_Delete(root);

	if(Morfeas_MTI_DBus_method_call("MTI", dev_name, "new_MTI_config", contents, &reply))//Check for failure
	{
		reply = UA_STRING_ALLOC("Error!!!");
		ret = UA_STATUSCODE_BADSYNTAXERROR;
	}
	UA_Variant_setScalarCopy(output, &reply, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&reply);
    return ret;
}

void Morfeas_add_new_MTI_config(UA_Server *server_ptr, char *Parent_id, char *Node_id)
{
	const char *inp_descriptions[] = {"Range:(0..126)",
									  "Accepted values:{TC16,TC8,TC4,2CH_QUAD,RMSW/MUX}",
									  "Successful receptions to set valid flag(Range:1..254, 0=Unchanged, 255=Disable)",
									  "Failed receptions to reset valid flag(Range:1..254, 0=Unchanged, 255=Disable)",
									  "Global ON/OFF mode(Used with mode RMSW/MUX)",
									  "Global Sleep mode(Used with mode RMSW/MUX)"};
	const char *inp_names[] = {"new_RF_CH","new_Radio_mode","StV","StF","G_SW","G_SL",NULL};
	const unsigned int inp_value_type[] = {UA_TYPES_BYTE, UA_TYPES_STRING, UA_TYPES_BYTE, UA_TYPES_BYTE, UA_TYPES_BOOLEAN, UA_TYPES_BOOLEAN};

	//Configure inputArguments
    UA_Argument inputArguments[6];
	for(int i=0; i<sizeof(inputArguments)/sizeof(*inputArguments); i++)
	{
		UA_Argument_init(&inputArguments[i]);
		inputArguments[i].description = UA_LOCALIZEDTEXT("en-US", (char *)inp_descriptions[i]);
		inputArguments[i].name = UA_STRING((char *)inp_names[i]);
		inputArguments[i].dataType = UA_TYPES[inp_value_type[i]].typeId;
		inputArguments[i].valueRank = UA_VALUERANK_SCALAR_OR_ONE_DIMENSION;
	}
	//Configure outputArgument
	UA_Argument outputArgument;
	UA_Argument_init(&outputArgument);
	outputArgument.description = UA_LOCALIZEDTEXT("en-US", "Return of the method call");
	outputArgument.name = UA_STRING("Return");
	outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	outputArgument.valueRank = UA_VALUERANK_SCALAR;

	UA_MethodAttributes Method_Attr = UA_MethodAttributes_default;
	Method_Attr.description = UA_LOCALIZEDTEXT("en-US","Method new_MTI_config");
	Method_Attr.displayName = UA_LOCALIZEDTEXT("en-US","new_MTI_config()");
	Method_Attr.executable = true;
	Method_Attr.userExecutable = true;
	UA_Server_addMethodNode(server_ptr,
							UA_NODEID_STRING(1, Node_id),
							UA_NODEID_STRING(1, Parent_id),
							UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
							UA_QUALIFIEDNAME(1, Node_id),
							Method_Attr, &Morfeas_new_MTI_config_method_callback,
							sizeof(inputArguments)/sizeof(*inputArguments), inputArguments,
							1, &outputArgument,
							NULL, NULL);
}
