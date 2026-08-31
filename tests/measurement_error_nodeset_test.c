/* Embedded OPC-UA regression tests for production measurement IPC handlers. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <open62541/server_config_default.h>

#include "../src/Morfeas_opc_ua/Morfeas_handlers_nodeset.h"

static int checks, failures;

#define CHECK(condition, message) \
	do { checks++; if(condition) printf("PASS: %s\n", message); \
	else { failures++; fprintf(stderr, "FAIL: %s\n", message); } } while(0)

static void add_root(UA_Server *server, const char *id)
{
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)id);
	UA_Server_addObjectNode(server, UA_NODEID_STRING(1, (char *)id),
		UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
		UA_QUALIFIEDNAME(1, (char *)id), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), attr, NULL, NULL);
}

static int read_float(UA_Server *server, const char *node_id, float *out)
{
	UA_Variant value;
	UA_Variant_init(&value);
	if(UA_Server_readValue(server, UA_NODEID_STRING(1, (char *)node_id), &value) != UA_STATUSCODE_GOOD ||
		!UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT]))
	{
		UA_Variant_clear(&value);
		return 0;
	}
	*out = *(float *)value.data;
	UA_Variant_clear(&value);
	return 1;
}

static void test_iobox(UA_Server *server)
{
	IPC_message msg = {0};
	float value = NAN;
	const unsigned int ip = 16909060;
	IOBOX_handler_reg(server, "ibox");
	strcpy(msg.IOBOX_channels_reg.Dev_or_Bus_name, "ibox");
	msg.IOBOX_channels_reg.IOBOX_IPv4 = ip;
	IPC_msg_from_IOBOX_handler(server, IPC_IOBOX_channels_reg, &msg);

	memset(&msg, 0, sizeof(msg));
	strcpy(msg.IOBOX_data.Dev_or_Bus_name, "ibox");
	msg.IOBOX_data.IOBOX_IPv4 = ip;
	msg.IOBOX_data.RX[0].status = 1;
	msg.IOBOX_data.RX[0].success = 88;
	msg.IOBOX_data.RX[0].CH_value[0] = 12.5f;
	IPC_msg_from_IOBOX_handler(server, IPC_IOBOX_data, &msg);
	CHECK(read_float(server, "IOBOX.16909060.RX1.CH1.meas", &value) && value == 12.5f,
		"IOBOX healthy channel preserves its measurement");
	CHECK(read_float(server, "IOBOX.16909060.RX1.Status.meas", &value) && value == 1.0f,
		"IOBOX status metric reports connected receiver");
	CHECK(read_float(server, "IOBOX.16909060.RX1.Success.meas", &value) && value == 88.0f,
		"IOBOX success metric preserves the receiver quality value");

	msg.IOBOX_data.RX[0].CH_value[0] = NO_SENSOR_VALUE;
	IPC_msg_from_IOBOX_handler(server, IPC_IOBOX_data, &msg);
	CHECK(read_float(server, "IOBOX.16909060.RX1.CH1.meas", &value) && value == MORFEAS_MEAS_ERROR_NO_SENSOR,
		"IOBOX no-sensor sentinel maps to -902");

	msg.IOBOX_data.RX[0].status = 0;
	msg.IOBOX_data.RX[0].success = 0;
	IPC_msg_from_IOBOX_handler(server, IPC_IOBOX_data, &msg);
	CHECK(read_float(server, "IOBOX.16909060.RX1.CH1.meas", &value) && value == MORFEAS_MEAS_ERROR_OFFLINE,
		"IOBOX disconnected receiver maps to -901");

	memset(&msg, 0, sizeof(msg));
	strcpy(msg.IOBOX_report.Dev_or_Bus_name, "ibox");
	msg.IOBOX_report.IOBOX_IPv4 = ip;
	msg.IOBOX_report.status = 1;
	IPC_msg_from_IOBOX_handler(server, IPC_IOBOX_report, &msg);
	CHECK(read_float(server, "IOBOX.16909060.RX1.CH1.meas", &value) && value == MORFEAS_MEAS_ERROR_UNREGISTERED,
		"IOBOX device transport error maps registered channels to -905");
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.IOBOX_data.Dev_or_Bus_name, "ibox");
	msg.IOBOX_data.IOBOX_IPv4 = ip;
	msg.IOBOX_data.RX[0].status = 1;
	msg.IOBOX_data.RX[0].success = 100;
	msg.IOBOX_data.RX[0].CH_value[0] = 19.0f;
	IPC_msg_from_IOBOX_handler(server, IPC_IOBOX_data, &msg);
	CHECK(read_float(server, "IOBOX.16909060.RX1.CH1.meas", &value) && value == 19.0f,
		"IOBOX valid frame recovers the measurement after transport failure");
}

static void test_sdaq(UA_Server *server)
{
	IPC_message msg = {0};
	float value = NAN;
	const unsigned int serial = 99112233U;
	SDAQ_handler_reg(server, "can2");
	strcpy(msg.SDAQ_reg_update.Dev_or_Bus_name, "can2");
	msg.SDAQ_reg_update.address = 4;
	msg.SDAQ_reg_update.SDAQ_status.dev_sn = serial;
	msg.SDAQ_reg_update.reg_status = Ready;
	msg.SDAQ_reg_update.t_amount = 1;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_register_or_update, &msg);
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.SDAQ_info.Dev_or_Bus_name, "can2");
	msg.SDAQ_info.SDAQ_serial_number = serial;
	msg.SDAQ_info.SDAQ_info_data.num_of_ch = 1;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_info, &msg);

	memset(&msg, 0, sizeof(msg));
	msg.SDAQ_meas.SDAQ_serial_number = serial;
	msg.SDAQ_meas.Amount_of_channels = 1;
	msg.SDAQ_meas.SDAQ_channel_meas[0].status = 1 << No_sensor;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_meas, &msg);
	CHECK(read_float(server, "SDAQ.99112233.CH1.meas", &value) && value == MORFEAS_MEAS_ERROR_NO_SENSOR,
		"SDAQ no-sensor status maps to -902");
	msg.SDAQ_meas.SDAQ_channel_meas[0].status = 1 << 7;
	msg.SDAQ_meas.SDAQ_channel_meas[0].meas = 12.0f;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_meas, &msg);
	CHECK(read_float(server, "SDAQ.99112233.CH1.meas", &value) && value == MORFEAS_MEAS_ERROR_UNCLASSIFIED,
		"SDAQ unclassified status maps to -904");
}

static void test_mti(UA_Server *server)
{
	IPC_message msg = {0};
	float value = NAN;
	const unsigned int ip = 3232235786U;
	char node[96];
	MTI_handler_reg(server, "mti");
	strcpy(msg.MTI_tree_reg.Dev_or_Bus_name, "mti");
	msg.MTI_tree_reg.MTI_IPv4 = ip;
	IPC_msg_from_MTI_handler(server, IPC_MTI_tree_reg, &msg);

	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_Update_Radio.Dev_or_Bus_name, "mti");
	msg.MTI_Update_Radio.MTI_IPv4 = ip;
	msg.MTI_Update_Radio.Tele_dev_type = Tele_TC4;
	msg.MTI_Update_Radio.isNew_config = 1;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Update_Radio, &msg);
	snprintf(node, sizeof(node), "MTI.%u.%s.CH1.meas", ip, MTI_Tele_dev_type_str[Tele_TC4]);

	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_tele_data.Dev_or_Bus_name, "mti");
	msg.MTI_tele_data.MTI_IPv4 = ip;
	msg.MTI_tele_data.Tele_dev_type = Tele_TC4;
	msg.MTI_tele_data.data.as_TC4.Data_isValid = 1;
	msg.MTI_tele_data.data.as_TC4.RX_Success_ratio = 100;
	msg.MTI_tele_data.data.as_TC4.CHs[0] = 3.25f;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Tele_data, &msg);
	CHECK(read_float(server, node, &value) && value == 3.25f, "MTI valid telemetry preserves measurement");

	msg.MTI_tele_data.data.as_TC4.Data_isValid = 0;
	msg.MTI_tele_data.data.as_TC4.RX_Success_ratio = 1;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Tele_data, &msg);
	CHECK(read_float(server, node, &value) && value == MORFEAS_MEAS_ERROR_DATA_INVALID,
		"MTI invalid data with packets arriving maps to -907");
	msg.MTI_tele_data.data.as_TC4.RX_Success_ratio = 0;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Tele_data, &msg);
	CHECK(read_float(server, node, &value) && value == MORFEAS_MEAS_ERROR_OFFLINE,
		"MTI invalid data with no packets maps to -901");
	msg.MTI_tele_data.data.as_TC4.Data_isValid = 1;
	msg.MTI_tele_data.data.as_TC4.CHs[0] = NO_SENSOR_VALUE;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Tele_data, &msg);
	CHECK(read_float(server, node, &value) && value == MORFEAS_MEAS_ERROR_NO_SENSOR,
		"MTI no-sensor sentinel maps to -902");
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_report.Dev_or_Bus_name, "mti");
	msg.MTI_report.MTI_IPv4 = ip;
	msg.MTI_report.Tele_dev_type = Tele_TC4;
	msg.MTI_report.amount_of_Linkable_tele = 1;
	msg.MTI_report.status = 1;
	IPC_msg_from_MTI_handler(server, IPC_MTI_report, &msg);
	CHECK(read_float(server, node, &value) && value == MORFEAS_MEAS_ERROR_UNREGISTERED,
		"MTI device transport error maps telemetry to -905");
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_Update_Radio.Dev_or_Bus_name, "mti");
	msg.MTI_Update_Radio.MTI_IPv4 = ip;
	msg.MTI_Update_Radio.Tele_dev_type = Disabled;
	msg.MTI_Update_Radio.isNew_config = 1;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Update_Radio, &msg);
	CHECK(read_float(server, node, &value) && value == MORFEAS_MEAS_ERROR_OFFLINE,
		"MTI radio disabled keeps telemetry source and maps it to -901");
	msg.MTI_Update_Radio.Tele_dev_type = Tele_TC4;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Update_Radio, &msg);
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_tele_data.Dev_or_Bus_name, "mti");
	msg.MTI_tele_data.MTI_IPv4 = ip;
	msg.MTI_tele_data.Tele_dev_type = Tele_TC4;
	msg.MTI_tele_data.data.as_TC4.Data_isValid = 1;
	msg.MTI_tele_data.data.as_TC4.RX_Success_ratio = 100;
	msg.MTI_tele_data.data.as_TC4.CHs[0] = 3.25f;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Tele_data, &msg);
	CHECK(read_float(server, node, &value) && value == 3.25f,
		"MTI valid telemetry recovers after a disabled radio");
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_Update_Radio.Dev_or_Bus_name, "mti");
	msg.MTI_Update_Radio.MTI_IPv4 = ip;
	msg.MTI_Update_Radio.Tele_dev_type = RMSW_MUX;
	msg.MTI_Update_Radio.isNew_config = 1;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Update_Radio, &msg);
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_RMSW_MUX_data.Dev_or_Bus_name, "mti");
	msg.MTI_RMSW_MUX_data.MTI_IPv4 = ip;
	msg.MTI_RMSW_MUX_data.Devs_data.amount_of_devices = 1;
	msg.MTI_RMSW_MUX_data.Devs_data.det_devs_data[0].dev_type = Mini_RMSW;
	msg.MTI_RMSW_MUX_data.Devs_data.det_devs_data[0].dev_id = 7;
	msg.MTI_RMSW_MUX_data.Devs_data.det_devs_data[0].meas_data[0] = 6.0f;
	IPC_msg_from_MTI_handler(server, IPC_MTI_RMSW_MUX_data, &msg);
	CHECK(read_float(server, "MTI.3232235786.ID:7.CH1.meas", &value) && value == 6.0f,
		"MTI RMSW source is registered with its measured value");
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_Update_Radio.Dev_or_Bus_name, "mti");
	msg.MTI_Update_Radio.MTI_IPv4 = ip;
	msg.MTI_Update_Radio.Tele_dev_type = Disabled;
	msg.MTI_Update_Radio.isNew_config = 1;
	IPC_msg_from_MTI_handler(server, IPC_MTI_Update_Radio, &msg);
	CHECK(read_float(server, "MTI.3232235786.ID:7.CH1.meas", &value) && value == MORFEAS_MEAS_ERROR_OFFLINE,
		"MTI radio disabled maps existing RMSW source to -901");
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.MTI_RMSW_MUX_data.Dev_or_Bus_name, "mti");
	msg.MTI_RMSW_MUX_data.MTI_IPv4 = ip;
	msg.MTI_RMSW_MUX_data.Devs_data.amount_to_be_remove = 1;
	msg.MTI_RMSW_MUX_data.Devs_data.IDs_to_be_removed[0] = 7;
	IPC_msg_from_MTI_handler(server, IPC_MTI_RMSW_MUX_data, &msg);
	CHECK(read_float(server, "MTI.3232235786.ID:7.CH1.meas", &value) && value == MORFEAS_MEAS_ERROR_OFFLINE,
		"MTI removed RMSW source remains present and maps to -901");
}

static void test_nox(UA_Server *server)
{
	IPC_message msg = {0};
	float value = NAN;
	NOX_handler_reg(server, "can1");
	strcpy(msg.NOX_BUS_info.Dev_or_Bus_name, "can1");
	msg.NOX_BUS_info.active_devs[0] = 1;
	IPC_msg_from_NOX_handler(server, IPC_NOX_CAN_BUS_info, &msg);

	memset(&msg, 0, sizeof(msg));
	strcpy(msg.NOX_data.Dev_or_Bus_name, "can1");
	msg.NOX_data.sensor_addr = 0;
	msg.NOX_data.NOXs_data.meas_state = 1;
	msg.NOX_data.NOXs_data.status.in_temperature = 0;
	msg.NOX_data.NOXs_data.status.heater_mode_state = 3;
	IPC_msg_from_NOX_handler(server, IPC_NOX_data, &msg);
	CHECK(read_float(server, "can1.sensors.addr_0.NOx_value", &value) && value == MORFEAS_MEAS_ERROR_STANDBY,
		"NOX heater-off state maps to -906");
	msg.NOX_data.NOXs_data.status.heater_mode_state = 1;
	IPC_msg_from_NOX_handler(server, IPC_NOX_data, &msg);
	CHECK(read_float(server, "can1.sensors.addr_0.NOx_value", &value) && value == MORFEAS_MEAS_ERROR_STALL,
		"NOX warm-up state maps to -903");
	msg.NOX_data.NOXs_data.status.heater_mode_state = 0;
	IPC_msg_from_NOX_handler(server, IPC_NOX_data, &msg);
	CHECK(read_float(server, "can1.sensors.addr_0.NOx_value", &value) && value == MORFEAS_MEAS_ERROR_UNCLASSIFIED,
		"NOX invalid unclassified state maps to -904");
	msg.NOX_data.NOXs_data.status.in_temperature = 1;
	msg.NOX_data.NOXs_data.status.is_NOx_value_valid = 1;
	msg.NOX_data.NOXs_data.status.is_O2_value_valid = 1;
	msg.NOX_data.NOXs_data.NOx_value = 44.0f;
	IPC_msg_from_NOX_handler(server, IPC_NOX_data, &msg);
	CHECK(read_float(server, "can1.sensors.addr_0.NOx_value", &value) && value == 44.0f,
		"NOX valid measurement recovers from error state");
}

static void test_missing_source_fallback(UA_Server *server)
{
	struct Link_entry links[4] = {0};
	const unsigned char types[] = {SDAQ, IOBOX, MTI, NOX};
	const char *names[] = {"SDAQ", "IOBOX", "MTI", "NOX"};
	for(size_t i = 0; i < 4; i++)
	{
		UA_DataValue value;
		UA_DataValue_init(&value);
		links[i].interface_type_num = types[i];
		links[i].identifier = 99;
		links[i].channel = 1;
		links[i].rxNum_teleType_or_value = types[i] == MTI ? Tele_TC4 : (types[i] == NOX ? NOx_val : 1);
		links[i].CAN_IF_name = "missingcan";
		CHECK(Morfeas_read_linked_value(server, &links[i], "meas", &value) == UA_STATUSCODE_GOOD && value.hasValue &&
			UA_Variant_hasScalarType(&value.value, &UA_TYPES[UA_TYPES_FLOAT]) &&
			*(float *)value.value.data == MORFEAS_MEAS_ERROR_UNREGISTERED, names[i]);
		UA_DataValue_clear(&value);
	}
	UA_DataValue value;
	UA_DataValue_init(&value);
	CHECK(Morfeas_read_linked_value(server, &links[0], "unit", &value) == UA_STATUSCODE_GOOD && !value.hasValue,
		"missing non-measurement source does not receive a numeric fallback");
	UA_DataValue_clear(&value);
	float source_value = 71.0f;
	Morfeas_opc_ua_add_variable_node(server, "SDAQ-ifs", "SDAQ.99.CH1.meas", "Value", UA_TYPES_FLOAT);
	Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1, "SDAQ.99.CH1.meas"), &source_value, UA_TYPES_FLOAT);
	UA_DataValue_init(&value);
	CHECK(Morfeas_read_linked_value(server, &links[0], "meas", &value) == UA_STATUSCODE_GOOD && value.hasValue &&
		UA_Variant_hasScalarType(&value.value, &UA_TYPES[UA_TYPES_FLOAT]) && *(float *)value.value.data == source_value,
		"recreated SDAQ source replaces the generic fallback with its live value");
	UA_DataValue_clear(&value);
}

int main(void)
{
	UA_Server *server = UA_Server_new();
	UA_ServerConfig_setDefault(UA_Server_getConfig(server));
	UA_Server_getConfig(server)->allowEmptyVariables = UA_RULEHANDLING_ACCEPT;
	add_root(server, "IOBOX-ifs");
	add_root(server, "MTI-ifs");
	add_root(server, "NOX-ifs");
	add_root(server, "SDAQ-ifs");
	add_root(server, "ISO_Channels");
	test_sdaq(server);
	test_iobox(server);
	test_mti(server);
	test_nox(server);
	test_missing_source_fallback(server);
	UA_Server_delete(server);
	printf("\n%d checks, %d passed, %d failed\n", checks, checks - failures, failures);
	return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
