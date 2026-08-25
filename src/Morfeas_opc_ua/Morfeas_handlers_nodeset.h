/*
File: Morfeas_handlers_nodeset.h, Declaration of OPC-UA server's Nodeset
construction/deconstruction functions for all the Morfeas handlers.

Copyright (C) 12019-12021  Sam harry Tzavaras

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
#include <stdio.h>
#include <sys/stat.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "../IPC/Morfeas_IPC.h"//<-#include "Morfeas_Types.h"

extern pthread_mutex_t OPC_UA_NODESET_access;

struct Nodeset_file_signature
{
	dev_t device;
	ino_t inode;
	struct timespec modified;
	int valid;
};

int Morfeas_Nodeset_file_signature_matches(const struct Nodeset_file_signature *signature, const struct stat *file_stat);
void Morfeas_Nodeset_file_signature_record(struct Nodeset_file_signature *signature, const struct stat *file_stat);
void Morfeas_Nodeset_file_signature_record_result(
	struct Nodeset_file_signature *attempted_signature,
	struct Nodeset_file_signature *applied_signature,
	const struct stat *file_stat,
	int parsed,
	int applied);

//Assistance function manipulate the Morfeas OPC_UA configuration
UA_StatusCode Morfeas_OPC_UA_config(UA_ServerConfig *config, const char *app_name, const char *version);

//Nodeset object, variables, and methods add and update
void Morfeas_opc_ua_add_object_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name);
void Morfeas_opc_ua_add_variable_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type);
void Morfeas_opc_ua_add_variable_node_with_callback_onRead(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type, void *call_func);
void Morfeas_opc_ua_add_and_update_variable_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, const void *value, int _UA_Type);
void Update_NodeValue_by_nodeID(UA_Server *server, UA_NodeId Node_to_update, const void * value, int _UA_Type);

//The DBus method caller function. Return 0 if not internal error.
int Morfeas_DBus_method_call(const char *handler_type, const char *dev_name, const char *method, const char *contents, UA_String *reply);
//Function that get the string sec_num section from NodeId. Return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
int get_NodeId_sec(const UA_NodeId *NodeId, unsigned char sec_num, char *sec_str, size_t sec_str_size);

	//----Morfeas Handlers----//
//SDAQ's Handler related
void SDAQ_handler_reg(UA_Server *server, char *connected_to_BUS);
void SDAQ2OPC_UA_register_update(UA_Server *server, SDAQ_reg_update_msg *ptr);
void SDAQ2OPC_UA_register_update_info(UA_Server *server, SDAQ_info_msg *ptr);
void IPC_msg_from_SDAQ_handler(UA_Server *server, unsigned char type, IPC_message *IPC_msg_dec);
// Show Unit only when current SDAQ metadata is ready; runtime nodes stay stable.
void SDAQ_refresh_unit_gate(UA_Server *server_ptr, const char *iso_channel_name, unsigned int serial, unsigned char channel);
// Refresh every ISO channel linked to one SDAQ serial.
void SDAQ_refresh_unit_gates_for_serial(UA_Server *server_ptr, unsigned int serial);
// Delete an ISO object and a possibly detached Unit child.
void Morfeas_OPC_UA_delete_ISO_Channel_node(UA_Server *server_ptr, const char *iso_channel_name);
//IOBOX's Handler related
void IOBOX_handler_reg(UA_Server *server, char *dev_name);
void IPC_msg_from_IOBOX_handler(UA_Server *server, unsigned char type,IPC_message *IPC_msg_dec);
//MDAQ's Handler related
void MDAQ_handler_reg(UA_Server *server, char *dev_name);
void IPC_msg_from_MDAQ_handler(UA_Server *server, unsigned char type,IPC_message *IPC_msg_dec);
//MTI's Handler related
void MTI_handler_reg(UA_Server *server, char *dev_name);
void IPC_msg_from_MTI_handler(UA_Server *server, unsigned char type,IPC_message *IPC_msg_dec);
//NOX's Handler related
void NOX_handler_reg(UA_Server *server, char *dev_name);
void IPC_msg_from_NOX_handler(UA_Server *server, unsigned char type,IPC_message *IPC_msg_dec);
/*
//CPAD's Handler related
void CPAD_handler_reg(UA_Server *server, char *dev_name);
void IPC_msg_from_CPAD_handler(UA_Server *server, unsigned char type,IPC_message *IPC_msg_dec);
*/
