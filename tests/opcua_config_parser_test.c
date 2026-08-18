/*
 * tests/opcua_config_parser_test.c
 *
 * Standalone regression test for Core-A1 (strict SDAQ anchor decoder,
 * validator/list-builder parity, duplicate-SDAQ-source detection and
 * retired-MDAQ rejection) in src/Supplementary/Morfeas_XML.c.
 *
 * This test does not use the project's DTD-validating file loader
 * (Morfeas_XML_parsing) because it needs to exercise both valid and
 * intentionally invalid documents purely in memory. It calls
 * Morfeas_opc_ua_config_valid() and XML_doc_to_List_ISO_Channels() exactly as
 * Morfeas_opc_ua.c does, and validate_anchor_comp() directly for the grammar
 * acceptance table, via an extern prototype (it is not part of the public
 * Morfeas_XML.h API, but it is not "static" either).
 *
 * Build/run: see tests/README.md in this directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gmodule.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "../src/IPC/Morfeas_IPC.h"
#include "../src/Supplementary/Morfeas_XML.h"

extern int validate_anchor_comp(char *anchor_str, char handler_type);

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond, fmt, ...) \
	do { \
		g_checks++; \
		if (!(cond)) { \
			g_failures++; \
			fprintf(stderr, "FAIL: " fmt "\n", ##__VA_ARGS__); \
		} else { \
			printf("PASS: " fmt "\n", ##__VA_ARGS__); \
		} \
	} while (0)

static void test_sdaq_grammar_accept(const char *anchor)
{
	int rc = validate_anchor_comp((char *)anchor, SDAQ);
	CHECK(rc == EXIT_SUCCESS, "accept \"%s\" (rc=%d)", anchor, rc);
}

static void test_sdaq_grammar_reject(const char *anchor)
{
	int rc = validate_anchor_comp((char *)anchor, SDAQ);
	CHECK(rc == EXIT_FAILURE, "reject \"%s\" (rc=%d)", anchor, rc);
}

static void test_mdaq_rejected(void)
{
	int rc = validate_anchor_comp("167772170.CH1.Val1", MDAQ);
	CHECK(rc == EXIT_FAILURE, "INTERFACE_TYPE=MDAQ is unconditionally rejected (rc=%d)", rc);
}

static void test_iobox_grammar_accept(const char *anchor)
{
	int rc = validate_anchor_comp((char *)anchor, IOBOX);
	CHECK(rc == EXIT_SUCCESS, "IOBOX accept \"%s\" (rc=%d)", anchor, rc);
}

static void test_iobox_grammar_reject(const char *anchor)
{
	int rc = validate_anchor_comp((char *)anchor, IOBOX);
	CHECK(rc == EXIT_FAILURE, "IOBOX reject \"%s\" (rc=%d)", anchor, rc);
}

static void test_mti_grammar_accept(const char *anchor)
{
	int rc = validate_anchor_comp((char *)anchor, MTI);
	CHECK(rc == EXIT_SUCCESS, "MTI accept \"%s\" (rc=%d)", anchor, rc);
}

static void test_mti_grammar_reject(const char *anchor)
{
	int rc = validate_anchor_comp((char *)anchor, MTI);
	CHECK(rc == EXIT_FAILURE, "MTI reject \"%s\" (rc=%d)", anchor, rc);
}

//Build an in-memory NODESet document (no DTD needed for Morfeas_opc_ua_config_valid()).
static xmlDocPtr build_doc(const char *channels_xml)
{
	char buf[4096];
	snprintf(buf, sizeof(buf), "<NODESet>%s</NODESet>", channels_xml);
	return xmlReadMemory(buf, (int)strlen(buf), "test.xml", NULL, XML_PARSE_NOBLANKS);
}

static const char *CHANNEL_TEMPLATE =
	"<CHANNEL>"
	"<ISO_CHANNEL>%s</ISO_CHANNEL>"
	"<INTERFACE_TYPE>%s</INTERFACE_TYPE>"
	"<ANCHOR>%s</ANCHOR>"
	"<DESCRIPTION>d</DESCRIPTION>"
	"<MIN>0</MIN>"
	"<MAX>1</MAX>"
	"</CHANNEL>";

//IOBOX/MTI/NOX own their UNIT statically from the XML; SDAQ does not (it is
//XML-optional/not read from the XML). Fixtures for those three interfaces
//use this template when they need a non-empty UNIT to pass validation.
static const char *CHANNEL_TEMPLATE_WITH_UNIT =
	"<CHANNEL>"
	"<ISO_CHANNEL>%s</ISO_CHANNEL>"
	"<INTERFACE_TYPE>%s</INTERFACE_TYPE>"
	"<ANCHOR>%s</ANCHOR>"
	"<DESCRIPTION>d</DESCRIPTION>"
	"<MIN>0</MIN>"
	"<MAX>1</MAX>"
	"<UNIT>%s</UNIT>"
	"</CHANNEL>";

static void test_whole_document_valid_serial_anchor(void)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE, "TE1", "SDAQ", "796834087.CH1");
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_SUCCESS, "single valid serial-form SDAQ channel is accepted (rc=%d)", rc);

	GSList *links = NULL;
	XML_doc_to_List_ISO_Channels(xmlDocGetRootElement(doc), &links);
	CHECK(g_slist_length(links) == 1, "list-builder produced exactly one Link_entry");
	if (links) {
		struct Link_entry *e = links->data;
		CHECK(e->identifier == 796834087u, "list-builder decoded serial=796834087 (got %u)", e->identifier);
		CHECK(e->channel == 1, "list-builder decoded channel=1 (got %u)", e->channel);
	}
	g_slist_free_full(links, free_Link_entry);
	xmlFreeDoc(doc);
}

//Reproduces the field incident: an address-style SDAQ anchor must reject the whole document.
static void test_whole_document_rejects_address_style_anchor(void)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE, "_Protea_NH3", "SDAQ", "CAN1.ADDR:05.CH:01");
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (incident fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "field-incident address-style anchor \"CAN1.ADDR:05.CH:01\" rejects whole document (rc=%d)", rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_rejects_mdaq_channel(void)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE, "TE200", "MDAQ", "167772170.CH1.Val1");
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (MDAQ fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "retired INTERFACE_TYPE=MDAQ rejects whole document (rc=%d)", rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_rejects_duplicate_sdaq_source(void)
{
	char c1[512], c2[512], both[1024];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE, "TE_A", "SDAQ", "796834087.CH1");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE, "TE_B", "SDAQ", "796834087.CH1");
	snprintf(both, sizeof(both), "%s%s", c1, c2);
	xmlDocPtr doc = build_doc(both);
	CHECK(doc != NULL, "in-memory doc parses (duplicate-source fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "two ISO_CHANNELs sharing the same (SDAQ, serial, channel) source are rejected (rc=%d)", rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_allows_same_serial_different_channel(void)
{
	char c1[512], c2[512], both[1024];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE, "TE_A", "SDAQ", "796834087.CH1");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE, "TE_B", "SDAQ", "796834087.CH2");
	snprintf(both, sizeof(both), "%s%s", c1, c2);
	xmlDocPtr doc = build_doc(both);
	CHECK(doc != NULL, "in-memory doc parses (same-serial different-channel fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_SUCCESS, "same serial, different channel is a legal pair of SDAQ channels (rc=%d)", rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_rejects_duplicate_iobox_source(void)
{
	char c1[512], c2[512], both[1024];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE_WITH_UNIT, "TE_A", "IOBOX", "117440522.RX1.CH1", "C");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE_WITH_UNIT, "TE_B", "IOBOX", "117440522.RX1.CH1", "C");
	snprintf(both, sizeof(both), "%s%s", c1, c2);
	xmlDocPtr doc = build_doc(both);
	CHECK(doc != NULL, "in-memory doc parses (IOBOX duplicate-source fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "two ISO_CHANNELs sharing the same (IOBOX, identifier, receiver, channel) source are rejected (rc=%d)", rc);
	xmlFreeDoc(doc);
}

//Same identifier/receiver, different channel/kind: all legal, distinct sources.
static void test_whole_document_allows_distinct_iobox_sources(void)
{
	char c1[512], c2[512], c3[512], all[2048];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE_WITH_UNIT, "TE_CH1", "IOBOX", "117440522.RX1.CH1", "C");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE_WITH_UNIT, "TE_CH2", "IOBOX", "117440522.RX1.CH2", "C");
	snprintf(c3, sizeof(c3), CHANNEL_TEMPLATE_WITH_UNIT, "TE_Status", "IOBOX", "117440522.RX1.Status", "bool");//UNIT is required even for the discrete Status anchor -- no per-anchor-kind exemption
	snprintf(all, sizeof(all), "%s%s%s", c1, c2, c3);
	xmlDocPtr doc = build_doc(all);
	CHECK(doc != NULL, "in-memory doc parses (IOBOX distinct-source fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_SUCCESS, "same IOBOX identifier/receiver with different channel/kind are legal distinct sources (rc=%d)", rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_rejects_duplicate_mti_source(void)
{
	char c1[512], c2[512], both[1024];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE_WITH_UNIT, "TE_A", "MTI", "222222.TC16.CH1", "C");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE_WITH_UNIT, "TE_B", "MTI", "222222.TC16.CH1", "C");
	snprintf(both, sizeof(both), "%s%s", c1, c2);
	xmlDocPtr doc = build_doc(both);
	CHECK(doc != NULL, "in-memory doc parses (MTI duplicate-source fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "two ISO_CHANNELs sharing the same (MTI, identifier, type, channel) source are rejected (rc=%d)", rc);
	xmlFreeDoc(doc);
}

//Same identifier/channel number, different telemetry type: TC16.CH1 and TC8.CH1
//are not a semantic duplicate (plan section 6: "TC16.CH1 与 TC8.CH1 不是 semantic duplicate").
static void test_whole_document_allows_distinct_mti_telemetry_type(void)
{
	char c1[512], c2[512], both[1024];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE_WITH_UNIT, "TE_TC16", "MTI", "222222.TC16.CH1", "C");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE_WITH_UNIT, "TE_TC8", "MTI", "222222.TC8.CH1", "C");
	snprintf(both, sizeof(both), "%s%s", c1, c2);
	xmlDocPtr doc = build_doc(both);
	CHECK(doc != NULL, "in-memory doc parses (MTI distinct-telemetry-type fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_SUCCESS, "same MTI identifier/channel with different telemetry type are legal distinct sources (rc=%d)", rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_rejects_duplicate_nox_source(void)
{
	char c1[512], c2[512], both[1024];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE_WITH_UNIT, "TE_A", "NOX", "can0.addr_0.NOx", "ppm");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE_WITH_UNIT, "TE_B", "NOX", "can0.addr_0.NOx", "ppm");
	snprintf(both, sizeof(both), "%s%s", c1, c2);
	xmlDocPtr doc = build_doc(both);
	CHECK(doc != NULL, "in-memory doc parses (NOX duplicate-source fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "two ISO_CHANNELs sharing the same (NOX, can_if, address, measurement) source are rejected (rc=%d)", rc);
	xmlFreeDoc(doc);
}

//Same physical NOX sensor, NOx and O2 measurements: legal, independently linkable pair.
static void test_whole_document_allows_nox_and_o2_pair(void)
{
	char c1[512], c2[512], both[1024];
	snprintf(c1, sizeof(c1), CHANNEL_TEMPLATE_WITH_UNIT, "TE_NOx", "NOX", "can0.addr_0.NOx", "ppm");
	snprintf(c2, sizeof(c2), CHANNEL_TEMPLATE_WITH_UNIT, "TE_O2", "NOX", "can0.addr_0.O2", "%");
	snprintf(both, sizeof(both), "%s%s", c1, c2);
	xmlDocPtr doc = build_doc(both);
	CHECK(doc != NULL, "in-memory doc parses (NOX NOx+O2 pair fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_SUCCESS, "NOx and O2 on the same NOX sensor are a legal independent pair (rc=%d)", rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_rejects_missing_unit(const char *interface_type, const char *anchor)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE, "TE_NoUnit", interface_type, anchor);//no <UNIT> node at all
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (%s missing-UNIT fixture)", interface_type);
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "%s channel with no UNIT node is rejected (rc=%d)", interface_type, rc);
	xmlFreeDoc(doc);
}

static void test_whole_document_rejects_blank_unit(const char *interface_type, const char *anchor)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE_WITH_UNIT, "TE_BlankUnit", interface_type, anchor, "   ");//whitespace-only UNIT
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (%s blank-UNIT fixture)", interface_type);
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "%s channel with a whitespace-only UNIT is rejected (rc=%d)", interface_type, rc);
	xmlFreeDoc(doc);
}

static void test_list_builder_iobox_fields(void)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE_WITH_UNIT, "TE_IOBOX", "IOBOX", "117440522.RX3.CH5", "C");
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (IOBOX list-builder fixture)");
	if (!doc) return;

	GSList *links = NULL;
	XML_doc_to_List_ISO_Channels(xmlDocGetRootElement(doc), &links);
	CHECK(g_slist_length(links) == 1, "IOBOX list-builder produced exactly one Link_entry");
	if (links) {
		struct Link_entry *e = links->data;
		CHECK(e->identifier == 117440522u, "IOBOX list-builder decoded identifier=117440522 (got %u)", e->identifier);
		CHECK(e->rxNum_teleType_or_value == 3, "IOBOX list-builder decoded receiver=3 (got %u)", e->rxNum_teleType_or_value);
		CHECK(e->channel == 5, "IOBOX list-builder decoded channel=5 (got %u)", e->channel);
	}
	g_slist_free_full(links, free_Link_entry);
	xmlFreeDoc(doc);
}

static void test_list_builder_mti_rmsw_fields(void)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE_WITH_UNIT, "TE_MiniRMSW", "MTI", "222222.ID:7.CH2", "C");
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (MTI Mini-RMSW list-builder fixture)");
	if (!doc) return;

	GSList *links = NULL;
	XML_doc_to_List_ISO_Channels(xmlDocGetRootElement(doc), &links);
	CHECK(g_slist_length(links) == 1, "MTI list-builder produced exactly one Link_entry");
	if (links) {
		struct Link_entry *e = links->data;
		CHECK(e->identifier == 222222u, "MTI list-builder decoded identifier=222222 (got %u)", e->identifier);
		CHECK(e->rxNum_teleType_or_value == RMSW_MUX, "MTI list-builder decoded rxNum_teleType_or_value=RMSW_MUX (got %u)", e->rxNum_teleType_or_value);
		CHECK(e->tele_ID == 7, "MTI list-builder decoded tele_ID=7 (got %u)", e->tele_ID);
		CHECK(e->channel == 2, "MTI list-builder decoded channel=2 (got %u)", e->channel);
	}
	g_slist_free_full(links, free_Link_entry);
	xmlFreeDoc(doc);
}

//C-1: no CHANNEL child element's content may be empty (2026-08-19 code
//review, F-1 -- this rule pre-dates Core-A2 but had no test coverage on
//either side of the Web/Core boundary, which is how the Web equivalent
//went unimplemented long enough to reach a live LOGDemo32 reproduction).
static void test_whole_document_rejects_empty_description(void)
{
	char channel[512] =
		"<CHANNEL>"
		"<ISO_CHANNEL>TE_EmptyDesc</ISO_CHANNEL>"
		"<INTERFACE_TYPE>SDAQ</INTERFACE_TYPE>"
		"<ANCHOR>796834087.CH1</ANCHOR>"
		"<DESCRIPTION></DESCRIPTION>"
		"<MIN>0</MIN>"
		"<MAX>1</MAX>"
		"</CHANNEL>";
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (empty-DESCRIPTION fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "channel with an empty <DESCRIPTION/> is rejected (rc=%d)", rc);
	xmlFreeDoc(doc);
}

//C-4: ISO_CHANNEL length must be < ISO_channel_name_size (20).
static void test_whole_document_rejects_iso_channel_too_long(void)
{
	static const char *too_long = "TE_Twenty_Chars_Long";//20 chars, == ISO_channel_name_size -> reject
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE, too_long, "SDAQ", "796834087.CH1");
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (too-long ISO_CHANNEL fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "ISO_CHANNEL of length %zu (>= ISO_channel_name_size) is rejected (rc=%d)", strlen(too_long), rc);
	xmlFreeDoc(doc);
}

//C-5: ISO_CHANNEL must not contain '.'.
static void test_whole_document_rejects_iso_channel_with_dot(void)
{
	char channel[512];
	snprintf(channel, sizeof(channel), CHANNEL_TEMPLATE, "TE_Bad.Name", "SDAQ", "796834087.CH1");
	xmlDocPtr doc = build_doc(channel);
	CHECK(doc != NULL, "in-memory doc parses (dotted ISO_CHANNEL fixture)");
	if (!doc) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_FAILURE, "ISO_CHANNEL containing '.' is rejected (rc=%d)", rc);
	xmlFreeDoc(doc);
}

//Run through the real DTD + strict validator pipeline, exactly like Morfeas_opc_ua.c does at startup.
static void test_shipped_default_config_still_boots(void)
{
	xmlDocPtr doc = NULL;
	int parse_rc = Morfeas_XML_parsing("configuration/OPC_UA_Config.xml", &doc);
	CHECK(parse_rc == EXIT_SUCCESS, "shipped default configuration/OPC_UA_Config.xml passes DTD validation (run from repo root)");
	if (parse_rc != EXIT_SUCCESS) return;

	int rc = Morfeas_opc_ua_config_valid(xmlDocGetRootElement(doc));
	CHECK(rc == EXIT_SUCCESS, "shipped default configuration/OPC_UA_Config.xml passes the strict semantic validator (rc=%d)", rc);
	xmlFreeDoc(doc);
}

int main(void)
{
	//11.1 grammar acceptance table
	test_sdaq_grammar_accept("1.CH1");
	test_sdaq_grammar_accept("796834087.CH1");
	test_sdaq_grammar_accept("4294967295.CH16");//uint32 max serial, SDAQ_MAX_AMOUNT_OF_CHANNELS max channel

	test_sdaq_grammar_reject("CAN1.ADDR:05.CH:01");
	test_sdaq_grammar_reject("can0.5.CH1");
	test_sdaq_grammar_reject("0.CH1");
	test_sdaq_grammar_reject("123.CH0");
	test_sdaq_grammar_reject("123.CH17");//SDAQ_MAX_AMOUNT_OF_CHANNELS+1
	test_sdaq_grammar_reject("4294967296.CH1");//uint32 overflow by 1
	test_sdaq_grammar_reject("18446744073709551616.CH1");//overflow beyond unsigned long on 64-bit too
	test_sdaq_grammar_reject("123.CH1.foo");//trailing text
	test_sdaq_grammar_reject(" 123.CH1");//leading whitespace
	test_sdaq_grammar_reject("123.CH1 ");//trailing whitespace
	test_sdaq_grammar_reject("-123.CH1");//negative
	test_sdaq_grammar_reject("000123.CH01");//leading zeros on both components
	test_sdaq_grammar_reject("123.ch1");//lower-case "ch"
	test_sdaq_grammar_reject("123.CH");//missing channel digits
	test_sdaq_grammar_reject(".CH1");//no serial digits at all; pins that decode_sdaq_anchor() alone gates SDAQ

	test_mdaq_rejected();

	//Core-A2: IOBOX grammar acceptance table
	test_iobox_grammar_accept("1.RX1.CH1");
	test_iobox_grammar_accept("117440522.RX1.CH1");
	test_iobox_grammar_accept("117440522.RX6.CH16");//IOBOX_Amount_of_All_RXs max receiver, IOBOX_Amount_of_channels max channel
	test_iobox_grammar_accept("117440522.RX1.Status");
	test_iobox_grammar_accept("117440522.RX1.Success");

	test_iobox_grammar_reject("0.RX1.CH1");//identifier zero
	test_iobox_grammar_reject("117440522.RX0.CH1");//receiver zero
	test_iobox_grammar_reject("117440522.RX7.CH1");//receiver > IOBOX_Amount_of_All_RXs
	test_iobox_grammar_reject("117440522.RX1.CH0");//channel zero
	test_iobox_grammar_reject("117440522.RX1.CH17");//channel > IOBOX_Amount_of_channels
	test_iobox_grammar_reject("117440522.RX01.CH1");//leading zero on receiver
	test_iobox_grammar_reject("117440522.RX1.CH01");//leading zero on channel
	test_iobox_grammar_reject("0117440522.RX1.CH1");//leading zero on identifier
	test_iobox_grammar_reject("117440522.rx1.CH1");//lower-case "rx"
	test_iobox_grammar_reject("117440522.RX1.ch1");//lower-case "ch"
	test_iobox_grammar_reject("117440522.RX1.Statusx");//suffix on Status
	test_iobox_grammar_reject("117440522.RX1.CH1extra");//trailing text
	test_iobox_grammar_reject("117440522.RX1");//missing CH/Status/Success
	test_iobox_grammar_reject(" 117440522.RX1.CH1");//leading whitespace
	test_iobox_grammar_reject("117440522.RX1.CH1 ");//trailing whitespace
	test_iobox_grammar_reject("4294967296.RX1.CH1");//uint32 overflow by 1
	test_iobox_grammar_reject("-117440522.RX1.CH1");//negative

	//Core-A2: MTI grammar acceptance table
	test_mti_grammar_accept("222222.TC16.CH1");
	test_mti_grammar_accept("222222.TC16.CH16");
	test_mti_grammar_accept("222222.TC8.CH8");
	test_mti_grammar_accept("222222.TC4.CH4");
	test_mti_grammar_accept("222222.QUAD.CH2");
	test_mti_grammar_accept("222222.ID:1.CH1");
	test_mti_grammar_accept("222222.ID:255.CH4");//tele_ID upper bound (unsigned char)

	test_mti_grammar_reject("222222.TC16.CH0");//channel zero
	test_mti_grammar_reject("222222.TC16.CH17");//channel > 16 for TC16
	test_mti_grammar_reject("222222.TC8.CH9");//channel > 8 for TC8
	test_mti_grammar_reject("222222.TC4.CH5");//channel > 4 for TC4
	test_mti_grammar_reject("222222.QUAD.CH3");//channel > 2 for QUAD
	test_mti_grammar_reject("222222.ID:4.CH5");//channel > 4 for Mini-RMSW
	test_mti_grammar_reject("222222.ID:0.CH1");//tele_ID zero
	test_mti_grammar_reject("222222.ID:256.CH1");//tele_ID > 255: must reject, not silently truncate into unsigned char
	test_mti_grammar_reject("222222.ID:01.CH1");//leading zero on tele_ID
	test_mti_grammar_reject("222222.RMSW/MUX.CH1");//the runtime radio-mode string is never itself a valid anchor token
	test_mti_grammar_reject("222222.tc16.CH1");//lower-case type literal
	test_mti_grammar_reject("222222.TC16.ch1");//lower-case "ch"
	test_mti_grammar_reject("222222.TC160.CH1");//literal must be followed immediately by '.', not more digits
	test_mti_grammar_reject("222222.TC16.CH1extra");//trailing text
	test_mti_grammar_reject("222222.TC16");//missing channel
	test_mti_grammar_reject("0222222.TC16.CH1");//leading zero on identifier
	test_mti_grammar_reject(" 222222.TC16.CH1");//leading whitespace
	test_mti_grammar_reject("222222.TC16.CH1 ");//trailing whitespace

	test_whole_document_valid_serial_anchor();
	test_whole_document_rejects_address_style_anchor();
	test_whole_document_rejects_mdaq_channel();
	test_whole_document_rejects_duplicate_sdaq_source();
	test_whole_document_allows_same_serial_different_channel();
	test_whole_document_rejects_duplicate_iobox_source();
	test_whole_document_allows_distinct_iobox_sources();
	test_whole_document_rejects_duplicate_mti_source();
	test_whole_document_allows_distinct_mti_telemetry_type();
	test_whole_document_rejects_duplicate_nox_source();
	test_whole_document_allows_nox_and_o2_pair();
	test_whole_document_rejects_missing_unit("IOBOX", "117440522.RX1.CH1");
	test_whole_document_rejects_missing_unit("MTI", "222222.TC16.CH1");
	test_whole_document_rejects_missing_unit("NOX", "can0.addr_0.NOx");
	test_whole_document_rejects_blank_unit("IOBOX", "117440522.RX1.CH1");
	test_whole_document_rejects_blank_unit("MTI", "222222.TC16.CH1");
	test_whole_document_rejects_blank_unit("NOX", "can0.addr_0.NOx");
	test_whole_document_rejects_missing_unit("IOBOX", "117440522.RX1.Status");//no per-anchor-kind exemption: Status is not "naturally unitless" as far as this rule is concerned
	test_list_builder_iobox_fields();
	test_list_builder_mti_rmsw_fields();
	test_whole_document_rejects_empty_description();
	test_whole_document_rejects_iso_channel_too_long();
	test_whole_document_rejects_iso_channel_with_dot();
	test_shipped_default_config_still_boots();

	printf("\n%d/%d checks passed\n", g_checks - g_failures, g_checks);
	return g_failures == 0 ? 0 : 1;
}
