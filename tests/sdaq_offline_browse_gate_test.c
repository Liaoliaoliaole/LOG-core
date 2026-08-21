/*
 * tests/sdaq_offline_browse_gate_test.c
 *
 * Core integration test for the SDAQ Unit browse gate. Starts a real
 * embedded UA_Server (no network
 * I/O -- node CRUD and the Browse service work synchronously without the
 * event loop running, exactly like Morfeas_opc_ua.c's own registration
 * functions already rely on) and drives the actual production entry
 * points:
 *   - Morfeas_OPC_UA_add_update_ISO_Channel_node() for the XML-apply boundary
 *   - IPC_msg_from_SDAQ_handler() for every SDAQ IPC message boundary
 * then uses the real OPC UA Browse service -- not a private struct peek --
 * to confirm the ISO channel's "Unit" child reference is present/absent,
 * which is the exact mechanism a Gateway browse depends on.
 *
 * This does not replace a genuine `MorfeasMapper.IsMappable()`
 * check against the real Configuration Tool binary still requires that
 * product plus SDAQ+Gateway hardware. This test instead validates the one
 * mechanism IsMappable() depends on -- the browse-visible child reference
 * set -- using open62541's own Browse service, so it is not "inferring
 * behaviour from open62541 API existing"; it exercises the service end to
 * end.
 *
 * Run: make test-core-o   (from repo root)
 */

#include <string.h>
#include <open62541/server_config_default.h>
#include "../src/Supplementary/Morfeas_XML.h"
#include "../src/Morfeas_opc_ua/Morfeas_handlers_nodeset.h"

// OPC_UA_NODESET_access itself is defined in Morfeas_opc_ua.c (linked in as
// Morfeas_opc_ua_testmain.o below); only declared extern via the shared header.

// None of these three DataSource read callbacks are declared in
// Morfeas_handlers_nodeset.h either (only used internally, wired up via a
// void* function pointer at node-creation time); declared here with their
// real UA_DataSourceReadCallback-shaped signature so this test can attach
// them to the same node names the production SDAQ branch uses.
UA_StatusCode CH_update_value(UA_Server *server_ptr, const UA_NodeId *sessionId, void *sessionContext,
	const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
	const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode Status_update_value(UA_Server *server_ptr, const UA_NodeId *sessionId, void *sessionContext,
	const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
	const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode Dev_update_value(UA_Server *server_ptr, const UA_NodeId *sessionId, void *sessionContext,
	const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
	const UA_NumericRange *range, UA_DataValue *dataValue);

static int g_checks = 0, g_failures = 0;

static void check(int cond, const char *msg)
{
	g_checks++;
	if(cond)
		printf("PASS: %s\n", msg);
	else
	{
		g_failures++;
		printf("FAIL: %s\n", msg);
	}
}

// True if parent_id currently has a HasComponent reference whose target is child_id
// (compares by target NodeId, not BrowseName: every node this codebase creates uses
// its own NodeId string as its BrowseName too, but comparing the actual target
// identity is what a Gateway's browse-driven auto-mapper structurally depends on).
static int browse_has_child(UA_Server *server, const char *parent_id, const char *child_id)
{
	UA_BrowseDescription bd;
	UA_BrowseDescription_init(&bd);
	bd.nodeId = UA_NODEID_STRING(1, (char *)parent_id);
	bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
	bd.includeSubtypes = true;
	bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
	bd.nodeClassMask = 0; //no filter
	bd.resultMask = UA_BROWSERESULTMASK_ALL;

	UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
	int found = 0;
	if(br.statusCode == UA_STATUSCODE_GOOD)
	{
		UA_NodeId want = UA_NODEID_STRING(1, (char *)child_id);
		for(size_t i = 0; i < br.referencesSize && !found; i++)
		{
			UA_ReferenceDescription *rd = &br.references[i];
			if(rd->nodeId.serverIndex == 0 && rd->nodeId.namespaceUri.length == 0 &&
			   UA_NodeId_equal(&rd->nodeId.nodeId, &want))
				found = 1;
		}
	}
	UA_BrowseResult_clear(&br);
	return found;
}

static int node_exists(UA_Server *server, const char *node_id)
{
	UA_NodeId out;
	UA_NodeId_init(&out);
	UA_StatusCode rc = UA_Server_readNodeId(server, UA_NODEID_STRING(1, (char *)node_id), &out);
	if(rc == UA_STATUSCODE_GOOD)
		UA_clear(&out, &UA_TYPES[UA_TYPES_NODEID]);
	return rc == UA_STATUSCODE_GOOD;
}

int main(void)
{
	struct Nodeset_file_signature applied_signature = {0}, attempted_signature = {0};
	struct stat first = {0}, same_second_new_inode = {0}, newer_nanosecond = {0};
	first.st_dev = same_second_new_inode.st_dev = newer_nanosecond.st_dev = 7;
	first.st_ino = newer_nanosecond.st_ino = 100;
	same_second_new_inode.st_ino = 101;
	first.st_mtim.tv_sec = same_second_new_inode.st_mtim.tv_sec = newer_nanosecond.st_mtim.tv_sec = 1234;
	first.st_mtim.tv_nsec = same_second_new_inode.st_mtim.tv_nsec = 10;
	newer_nanosecond.st_mtim.tv_nsec = 11;
	check(!Morfeas_Nodeset_file_signature_matches(&attempted_signature, &first),
		"an uninitialised signature forces the first configuration load");
	Morfeas_Nodeset_file_signature_record_result(&attempted_signature, &applied_signature, &first, 0, 0);
	check(!Morfeas_Nodeset_file_signature_matches(&attempted_signature, &first),
		"an XML or DTD parse failure remains retryable");
	Morfeas_Nodeset_file_signature_record_result(&attempted_signature, &applied_signature, &first, 1, 0);
	check(Morfeas_Nodeset_file_signature_matches(&attempted_signature, &first),
		"a rejected file version is not attempted repeatedly");
	check(!Morfeas_Nodeset_file_signature_matches(&applied_signature, &first),
		"a semantic validation failure does not mark that version as applied");
	check(!Morfeas_Nodeset_file_signature_matches(&attempted_signature, &same_second_new_inode),
		"same-second atomic rename is detected by inode change");
	check(!Morfeas_Nodeset_file_signature_matches(&attempted_signature, &newer_nanosecond),
		"same-inode same-second write is detected by nanosecond mtime");
	Morfeas_Nodeset_file_signature_record_result(&attempted_signature, &applied_signature, &same_second_new_inode, 1, 1);
	check(Morfeas_Nodeset_file_signature_matches(&attempted_signature, &same_second_new_inode),
		"a successful file version is recorded as attempted");
	check(Morfeas_Nodeset_file_signature_matches(&applied_signature, &same_second_new_inode),
		"a successful file version is also recorded as applied");

	UA_Server *server = UA_Server_new();
	UA_ServerConfig *config = UA_Server_getConfig(server);
	UA_ServerConfig_setDefault(config);
	// Matches Morfeas_OPC_UA_config() (Morfeas_opc_ua_config.c): these
	// DataSource nodes are created with no explicit initial value (the read
	// callback supplies it later), which the default ruleset rejects.
	config->allowEmptyVariables = UA_RULEHANDLING_ACCEPT;

	// Minimal root objects the real startup path (Morfeas_opc_ua_root_nodeset_Define(),
	// not exported/callable here) creates before any channel or SDAQ handler exists.
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ISO Channels");
	UA_Server_addObjectNode(server, UA_NODEID_STRING(1, "ISO_Channels"),
		UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
		UA_QUALIFIEDNAME(1, "ISO_Channels"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
		oAttr, NULL, NULL);
	oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "SDAQ interfaces");
	UA_Server_addObjectNode(server, UA_NODEID_STRING(1, "SDAQ-ifs"),
		UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
		UA_QUALIFIEDNAME(1, "SDAQ-ifs"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
		oAttr, NULL, NULL);
	SDAQ_handler_reg(server, "vcan0"); //Sets up "vcan0", "vcan0.amount", etc.

	/*
	 * Build one SDAQ ISO channel's node tree exactly like the SDAQ branch of
	 * Morfeas_OPC_UA_add_update_ISO_Channel_node() does (Morfeas_opc_ua.c,
	 * ~line 700-727), rather than calling that function itself: it reads
	 * the file-static `Links` list (populated from XML by
	 * XML_doc_to_List_ISO_Channels()) to find this channel's serial/channel,
	 * and that static has no external linkage for a test in a different
	 * translation unit to populate. The one line this test does not
	 * exercise as a result is the single `if(if_type == SDAQ)
	 * SDAQ_refresh_unit_gate(...)` call added at the end of that function
	 * (the XML-apply boundary) -- reviewed by inspection instead,
	 * since it is a one-line call into the exact same SDAQ_refresh_unit_gate()
	 * this test exercises directly and repeatedly below.
	 */
	Morfeas_opc_ua_add_object_node(server, "ISO_Channels", "_TEST1", "_TEST1");
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.status", "Status", UA_TYPES_STRING, Status_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.status_byte", "Status value", UA_TYPES_BYTE, Status_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.meas", "Value", UA_TYPES_FLOAT, CH_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.Cal_date", "Calibration Date", UA_TYPES_DATETIME, CH_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.period", "Calibration Period (Months)", UA_TYPES_BYTE, CH_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.unit", "Unit", UA_TYPES_STRING, CH_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.Address", "Device Address", UA_TYPES_BYTE, Dev_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.onBus", "SDAQnet", UA_TYPES_STRING, Dev_update_value);
	Morfeas_opc_ua_add_variable_node_with_callback_onRead(server, "_TEST1", "_TEST1.Type", "Device Type", UA_TYPES_STRING, Dev_update_value);

	check(browse_has_child(server, "_TEST1", "_TEST1.meas"),
		"the stable Value node exists right after node-tree creation (never gated)");
	check(browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit node itself is attached at creation time by the shared node-creation helper, before any gate refresh has run");

	/*
	 * Every step below sends the real IPC message through the real
	 * IPC_msg_from_SDAQ_handler() entry point, then calls
	 * SDAQ_refresh_unit_gate() directly for "_TEST1". In production the
	 * second part happens automatically, inside the same message handler,
	 * via SDAQ_refresh_unit_gates_for_serial() walking Morfeas_opc_ua.c's
	 * file-static `Links` list to find which ISO channel(s) reference this
	 * serial -- that static has no external linkage, so this test cannot
	 * populate it and instead drives the single-channel primitive directly
	 * for the one channel it knows about. This still exercises the actual
	 * readiness logic against real, live SDAQ.<serial>.* node state written
	 * by the real message handlers -- only the Links-lookup fan-out itself
	 * (a plain for-loop, reviewed by inspection) is not exercised here.
	 */
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(!browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate hidden by the first refresh, before any SDAQ IPC data has arrived");

	IPC_message msg;

	// 1) register_or_update with Reg_status short of Done -- must stay hidden.
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.SDAQ_reg_update.Dev_or_Bus_name, "vcan0");
	msg.SDAQ_reg_update.address = 5;
	msg.SDAQ_reg_update.SDAQ_status.dev_sn = 796834087;
	msg.SDAQ_reg_update.SDAQ_status.dev_type = 0;
	msg.SDAQ_reg_update.reg_status = Registered; //"Initial Registration", not Done
	msg.SDAQ_reg_update.t_amount = 1;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_register_or_update, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(!browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate still hidden while Reg_status is not yet Done");

	// 2) Flip Reg_status to Done -- still not ready: no per-channel Unit/Cal_date subtree yet.
	msg.SDAQ_reg_update.reg_status = Ready; //"Done"
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_register_or_update, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(!browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate still hidden: Reg_status Done, but channel Unit/Cal_date nodes don't exist yet");

	// 3) SDAQ_info: creates the per-channel subtree (Unit/Cal_date/period nodes now
	//    exist, but still hold open62541's own zero-value defaults).
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.SDAQ_info.Dev_or_Bus_name, "vcan0");
	msg.SDAQ_info.SDAQ_serial_number = 796834087;
	msg.SDAQ_info.SDAQ_info_data.dev_type = 0;
	msg.SDAQ_info.SDAQ_info_data.num_of_ch = 1;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_info, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(!browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate still hidden: channel subtree exists but Unit string is still empty and Cal_date is still the never-written zero value");

	// 4) meas: writes a real (non-empty) Unit string for CH1.
	memset(&msg, 0, sizeof(msg));
	msg.SDAQ_meas.SDAQ_serial_number = 796834087;
	msg.SDAQ_meas.Amount_of_channels = 1;
	msg.SDAQ_meas.SDAQ_channel_meas[0].unit = 3; //unit_str[3] == "°C" (SDAQ_drv.c)
	msg.SDAQ_meas.SDAQ_channel_meas[0].status = 0; //Okay
	msg.SDAQ_meas.SDAQ_channel_meas[0].meas = 25.0f;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_meas, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(!browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate still hidden: Unit string is now live, but Cal_date has never been written by a live cal_date message (year still 1601)");

	// 5) cal_date: writes a real device-reported calibration date -> every condition now holds.
	memset(&msg, 0, sizeof(msg));
	msg.SDAQ_cal_date.SDAQ_serial_number = 796834087;
	msg.SDAQ_cal_date.channel = 1;
	msg.SDAQ_cal_date.SDAQ_cal_date.year = 26; //device byte, +2000 => 2026
	msg.SDAQ_cal_date.SDAQ_cal_date.month = 1;
	msg.SDAQ_cal_date.SDAQ_cal_date.day = 1;
	msg.SDAQ_cal_date.SDAQ_cal_date.period = 12;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_cal_date, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate SHOWN once Reg_status=Done, Address valid, onBus/Type set, channel Unit is non-empty, and Cal_date is live");

	// Idempotency in the SHOWN state: a repeat refresh with nothing changed
	// must not error (BADDUPLICATEREFERENCENOTALLOWED tolerated internally) or flip anything.
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"repeat refresh in the same shown state is a harmless no-op (idempotent show)");

	// The five other stable nodes must never have been touched by any of this.
	check(browse_has_child(server, "_TEST1", "_TEST1.meas"), "Value node still present (never deleted/recreated)");
	check(browse_has_child(server, "_TEST1", "_TEST1.status"), "Status node still present (never deleted/recreated)");
	check(browse_has_child(server, "_TEST1", "_TEST1.Cal_date"), "Cal_date node still present as a normal child (it is never gated -- only Unit is)");
	check(browse_has_child(server, "_TEST1", "_TEST1.period"), "period node still present (never gated)");
	check(browse_has_child(server, "_TEST1", "_TEST1.Address"), "Address node still present (never gated)");

	// 6) A device-reported "explicitly uncalibrated" date (year normalizes to exactly
	//    2000, period legitimately 0) must count as real/live, not as a missing source.
	memset(&msg, 0, sizeof(msg));
	msg.SDAQ_cal_date.SDAQ_serial_number = 796834087;
	msg.SDAQ_cal_date.channel = 1;
	msg.SDAQ_cal_date.SDAQ_cal_date.year = 0; //device byte 0 -> +2000 => year 2000
	msg.SDAQ_cal_date.SDAQ_cal_date.month = 0; //normalizes to 1
	msg.SDAQ_cal_date.SDAQ_cal_date.day = 0;   //normalizes to 1
	msg.SDAQ_cal_date.SDAQ_cal_date.period = 0; //legitimately "no calibration interval configured"
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_cal_date, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate stays SHOWN for a device-reported all-zero \"explicitly uncalibrated\" date (year 2000), not treated as a missing source");

	// 7) IPC_SDAQ_clean_up: device disappears -> gate must hide again, and the six
	//    stable ISO-channel nodes must still all be present afterwards (only the
	//    reference is removed, never the nodes themselves).
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.SDAQ_clean.Dev_or_Bus_name, "vcan0");
	msg.SDAQ_clean.SDAQ_serial_number = 796834087;
	msg.SDAQ_clean.t_amount = 0;
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_clean_up, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(!browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"Unit gate hidden again after IPC_SDAQ_clean_up (device gone)");
	check(browse_has_child(server, "_TEST1", "_TEST1.meas"),
		"Value node still present after cleanup (six stable nodes are never deleted/recreated)");
	check(browse_has_child(server, "_TEST1", "_TEST1.status"),
		"Status node still present after cleanup");
	check(browse_has_child(server, "_TEST1", "_TEST1.Cal_date"),
		"Cal_date node still present after cleanup");
	check(browse_has_child(server, "_TEST1", "_TEST1.Address"),
		"Address node still present after cleanup");

	// 8) Idempotency: calling the gate refresh again in the same (hidden) state must
	//    not error or change anything -- exercised implicitly by re-sending clean_up.
	IPC_msg_from_SDAQ_handler(server, IPC_SDAQ_clean_up, &msg);
	SDAQ_refresh_unit_gate(server, "_TEST1", 796834087, 1);
	check(!browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"repeat clean_up for an already-gone device is a harmless no-op (idempotent hide)");

	/*
	 * A hidden Unit still exists but is detached from its parent. Deleting
	 * only the parent used to orphan that NodeId, so rebuilding the same ISO
	 * name as IOBOX/MTI/NOX could not create a browse-visible Unit until the
	 * whole Core process restarted.
	 */
	check(node_exists(server, "_TEST1.unit"),
		"hidden Unit node still exists before ISO channel deletion");
	Morfeas_OPC_UA_delete_ISO_Channel_node(server, "_TEST1");
	check(!node_exists(server, "_TEST1"),
		"ISO channel parent is deleted by the shared deletion helper");
	check(!node_exists(server, "_TEST1.unit"),
		"detached Unit node is explicitly deleted with its ISO channel");

	Morfeas_opc_ua_add_object_node(server, "ISO_Channels", "_TEST1", "_TEST1");
	Morfeas_opc_ua_add_variable_node(server, "_TEST1", "_TEST1.unit", "Unit", UA_TYPES_STRING);
	check(browse_has_child(server, "_TEST1", "_TEST1.unit"),
		"same ISO name rebuilt as a non-SDAQ object has a browse-visible Unit immediately");

	UA_Server_delete(server);

	printf("\n%d/%d checks passed\n", g_checks - g_failures, g_checks);
	return g_failures == 0 ? 0 : 1;
}
