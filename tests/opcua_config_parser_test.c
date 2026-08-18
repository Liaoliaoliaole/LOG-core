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
	test_sdaq_grammar_reject(".CH1");//no serial digits at all; pins that decode_sdaq_anchor() alone gates SDAQ now that the loose pre-switch atoi()/strstr() precheck in validate_anchor_comp() no longer also runs for handler_type==SDAQ (it remains for IOBOX/MTI/MDAQ, unchanged)

	test_mdaq_rejected();

	test_whole_document_valid_serial_anchor();
	test_whole_document_rejects_address_style_anchor();
	test_whole_document_rejects_mdaq_channel();
	test_whole_document_rejects_duplicate_sdaq_source();
	test_whole_document_allows_same_serial_different_channel();
	test_shipped_default_config_still_boots();

	printf("\n%d/%d checks passed\n", g_checks - g_failures, g_checks);
	return g_failures == 0 ? 0 : 1;
}
