/*
File: Morfeas_handlers_nodeset.h, Declaration of OPC-UA server's Nodeset
construction/deconstruction functions for all the Morfeas handlers.

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

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "Morfeas_IPC.h"//<-#include "Types.h"

extern pthread_mutex_t OPC_UA_NODESET_access;

//Assistance function manipulate the Morfeas OPC_UA configuration
UA_StatusCode Morfeas_OPC_UA_config(UA_ServerConfig *config, const char *app_name, const char *version);

//Nodeset object, variables, and methods add and update
void Morfeas_opc_ua_add_abject_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name);
void Morfeas_opc_ua_add_variable_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type);
void Morfeas_opc_ua_add_variable_node_with_callback_onRead(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type, void *call_func);
void Update_NodeValue_by_nodeID(UA_Server *server, UA_NodeId Node_to_update, const void * value, int _UA_Type);

	//----Morfeas BUSs Handlers----//
//SDAQ's Handler related
void SDAQ_handler_reg(UA_Server *server, char *connected_to_BUS);
void SDAQ2OPC_UA_register_update(UA_Server *server, SDAQ_reg_update_msg *ptr);
void SDAQ2OPC_UA_register_update_info(UA_Server *server, SDAQ_info_msg *ptr);


