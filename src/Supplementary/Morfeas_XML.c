/*
File: Morfeas_XML.c, Implementation of functions for read XML files
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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>

#include <glib.h>
#include <gmodule.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "../IPC/Morfeas_IPC.h"// -> #include "Morfeas_Types.h"
#include "Morfeas_run_check.h"
#include "../sdaq-worker/src/SDAQ_drv.h"//Only for the SDAQ_MAX_AMOUNT_OF_CHANNELS constant

/*
void print_XML_node(xmlNode * node)
{
    xmlNode *cur_node;
	if(node->type == XML_ELEMENT_NODE)
	{
		printf("Node name: %s\n", node->name);
		for (cur_node = node->children; cur_node; cur_node = cur_node->next)
		{
			if(cur_node->type == XML_ELEMENT_NODE)
			{
				if(cur_node->children)
				{
					if(cur_node->children->content)
					{
						printf("\tChild Node name: %s\n", cur_node->name);
						printf("\t\tHave contents: %s\n", cur_node->children->content);
					}
					else if(cur_node->children)
					{
						print_XML_node(cur_node);
					}
				}
				else
					printf("\t\tEmpty\n");
			}
			else
				printf("Is Not 'XML_ELEMENT_NODE'\n");
		}
	}
}
*/
xmlNode* scaning_XML_nodes_for_empty(xmlNode * node)
{
    xmlNode *cur_node, *ret = NULL;
	if(node->type == XML_ELEMENT_NODE)
	{
		for (cur_node = node->children; cur_node; cur_node = cur_node->next)
		{
			if(cur_node->type == XML_ELEMENT_NODE)
			{
				if(cur_node->children)
				{
					ret = scaning_XML_nodes_for_empty(cur_node);
					if(ret)
						return ret;
				}
				else
					return cur_node;
			}
		}
	}
	return NULL;
}

xmlNode * get_XML_node(xmlNode *root_node, const char *Node_name)
{
	xmlNode *cur_node, *ret = NULL;
	if(root_node->type == XML_ELEMENT_NODE)
	{
		for (cur_node = root_node->children; cur_node; cur_node = cur_node->next)
		{
			if(cur_node->type == XML_ELEMENT_NODE)
			{
				if(!strcmp((char *)(cur_node->name), Node_name))
					return cur_node;
				if(cur_node->children)
					if((ret = get_XML_node(cur_node, Node_name)))
						return ret;
			}
		}
	}
	return NULL;
}

char * XML_node_get_content(xmlNode *node, const char *node_name)
{
    xmlNode *cur_node;
	if(node)
	{
		for (cur_node = node->children; cur_node; cur_node = cur_node->next)
		{
			if(cur_node->type == XML_ELEMENT_NODE)
			{
				if(!strcmp((char *)(cur_node->name), node_name))
				{
					if(cur_node->children)
						return (char *)(cur_node->children->content);
					else
						return NULL;
				}
			}
		}
	}
	return NULL;
}
/*
 * Function where check node for attribute "Disable".
 * Return: 0 if Disable="false",
		   1 if Disable="true",
		   -1 if attribute is not found.
 */
int getprop_disable(xmlNode *node)
{
	int retval;
	xmlChar *content;
	while(node)
	{
		if(node->type == XML_ELEMENT_NODE)
		{
			if((content = xmlGetProp(node, BAD_CAST"Disable")))
			{
				if(!strcmp((char *)content, "true"))
					retval = 1;
				else if(!strcmp((char *)content, "false"))
					retval = 0;
				else
					retval = -1;
				xmlFree(content);
				return retval;
			}
		}
		node = node->next;
	}
	return -1;
}

int Morfeas_XML_parsing(const char *filename, xmlDocPtr *doc)
{
    xmlParserCtxtPtr ctxt;
    //--- create a parser context ---//
    if(!(ctxt = xmlNewParserCtxt()))
    {
        fprintf(stderr, "Failed to allocate parser context\n");
		return EXIT_FAILURE;
    }
    //--- parse the file, activating the DTD validation option ---//
    if(!(*doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID | XML_PARSE_NOBLANKS)))
    {
        fprintf(stderr, "Failed to parse %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return EXIT_FAILURE;
    }
    else
    {	//check if validation succeeded
		if(!(ctxt->valid))
		{
        	fprintf(stderr, "Failed to validate %s\n", filename);
        	xmlFreeParserCtxt(ctxt);
        	xmlFreeDoc(*doc);
        	return EXIT_FAILURE;
        }
    }
    xmlFreeParserCtxt(ctxt);
    return EXIT_SUCCESS;
}

#define anchor_check_buff_size 100

/*
 * Decode both the core's canonical NOX anchor format
 *     can0.addr_0.NOx
 * and the operator-facing format historically written by the web UI
 *     CAN0.ADDR:0.NOx
 *
 * The CAN interface is normalised to lower case because OPC-UA source node
 * identifiers are case-sensitive and are registered as e.g. "can0.sensors".
 */
static int decode_nox_anchor(const char *anchor_str, char *can_if_name, size_t can_if_name_size,
                             unsigned int *sensor_address, unsigned char *measurement)
{
	const char *first_dot, *second_dot, *address_begin, *address_end;
	const char *address_value;
	char *parse_end;
	unsigned long parsed_address;
	size_t can_if_length;

	if(!anchor_str || !can_if_name || can_if_name_size == 0 || !sensor_address || !measurement)
		return EXIT_FAILURE;

	first_dot = strchr(anchor_str, '.');
	second_dot = first_dot ? strchr(first_dot + 1, '.') : NULL;
	if(!first_dot || !second_dot || strchr(second_dot + 1, '.'))
		return EXIT_FAILURE;

	can_if_length = (size_t)(first_dot - anchor_str);
	if(can_if_length == 0 || can_if_length >= can_if_name_size || can_if_length >= Dev_or_Bus_name_str_size)
		return EXIT_FAILURE;

	for(size_t i = 0; i < can_if_length; i++)
		can_if_name[i] = (char)g_ascii_tolower(anchor_str[i]);
	can_if_name[can_if_length] = '\0';

	address_begin = first_dot + 1;
	address_end = second_dot;
	if((size_t)(address_end - address_begin) <= strlen("addr_"))
		return EXIT_FAILURE;
	if(strncasecmp(address_begin, "addr_", strlen("addr_")) &&
	   strncasecmp(address_begin, "addr:", strlen("addr:")))
		return EXIT_FAILURE;

	address_value = address_begin + strlen("addr_");
	for(const char *digit = address_value; digit < address_end; digit++)
	{
		if(!g_ascii_isdigit(*digit))
			return EXIT_FAILURE;
	}

	parsed_address = strtoul(address_value, &parse_end, 10);
	if(parse_end != address_end || parsed_address > 1)
		return EXIT_FAILURE;

	if(!strcasecmp(second_dot + 1, "NOx"))
		*measurement = NOx_val;
	else if(!strcasecmp(second_dot + 1, "O2"))
		*measurement = O2_val;
	else
		return EXIT_FAILURE;

	*sensor_address = (unsigned int)parsed_address;
	return EXIT_SUCCESS;
}

/*
 * Strict, full-string decoder for the canonical SDAQ anchor grammar:
 *     <serial>.CH<channel>
 * where <serial> is a non-zero uint32 with no leading zero, and <channel> is
 * a non-zero decimal with no leading zero, not exceeding SDAQ_MAX_AMOUNT_OF_CHANNELS.
 * This is the single decoder shared by validate_anchor_comp() and
 * XML_doc_to_List_ISO_Channels(); it must never be re-implemented a second time.
 * Return 0 on success, or -1 on failure.
 */
static int decode_sdaq_anchor(const char *anchor_str, unsigned int *serial, unsigned char *channel)
{
	const char *serial_begin, *serial_end, *channel_begin;
	char *parse_end;
	unsigned long parsed_serial, parsed_channel;

	if(!anchor_str || !serial || !channel)
		return EXIT_FAILURE;

	serial_begin = anchor_str;
	if(!g_ascii_isdigit(*serial_begin) || *serial_begin == '0')//No leading zero, digits only
		return EXIT_FAILURE;
	for(serial_end = serial_begin; g_ascii_isdigit(*serial_end); serial_end++);
	if(*serial_end != '.')
		return EXIT_FAILURE;

	if(strncmp(serial_end + 1, "CH", strlen("CH")))//Only the upper-case literal ".CH" is accepted
		return EXIT_FAILURE;
	channel_begin = serial_end + 1 + strlen("CH");
	if(!g_ascii_isdigit(*channel_begin) || *channel_begin == '0')//No leading zero, digits only
		return EXIT_FAILURE;

	errno = 0;
	parsed_serial = strtoul(serial_begin, &parse_end, 10);
	if(parse_end != serial_end || errno == ERANGE || parsed_serial == 0 || parsed_serial > UINT32_MAX)
		return EXIT_FAILURE;

	errno = 0;
	parsed_channel = strtoul(channel_begin, &parse_end, 10);
	if(*parse_end != '\0' || errno == ERANGE || parsed_channel == 0 || parsed_channel > SDAQ_MAX_AMOUNT_OF_CHANNELS)
		return EXIT_FAILURE;//parse_end must reach the end of the string: no trailing text allowed

	*serial = (unsigned int)parsed_serial;
	*channel = (unsigned char)parsed_channel;
	return EXIT_SUCCESS;
}

/*
 * Strict, full-string decoder for the canonical IOBOX anchor grammar:
 *     <identifier>.RX<receiver>.CH<channel>
 *     <identifier>.RX<receiver>.Status
 *     <identifier>.RX<receiver>.Success
 * identifier: uint32, no leading zero. receiver: 1..IOBOX_Amount_of_All_RXs.
 * channel (CH form only): 1..IOBOX_Amount_of_channels. ".RX", ".CH",
 * ".Status" and ".Success" are matched case-sensitively; no suffix, extra
 * segment or trailing text is accepted. *channel is written using the same
 * encoding as struct Link_entry's channel field: 1..IOBOX_Amount_of_channels
 * for a CH anchor, or the IOBOX_RX_Status_link_channel/IOBOX_RX_Success_link_channel
 * sentinels for the two aggregate anchors, so callers can assign it directly.
 * This is the single decoder shared by validate_anchor_comp() and
 * XML_doc_to_List_ISO_Channels(); it must never be re-implemented a second time.
 * Return 0 on success, or -1 on failure.
 */
static int decode_iobox_anchor(const char *anchor_str, unsigned int *identifier, unsigned char *receiver, unsigned char *channel)
{
	const char *id_begin, *id_end, *rx_begin, *rx_end, *tail, *ch_begin;
	char *parse_end;
	unsigned long parsed_id, parsed_rx, parsed_ch;

	if(!anchor_str || !identifier || !receiver || !channel)
		return EXIT_FAILURE;

	id_begin = anchor_str;
	if(!g_ascii_isdigit(*id_begin) || *id_begin == '0')//No leading zero, digits only
		return EXIT_FAILURE;
	for(id_end = id_begin; g_ascii_isdigit(*id_end); id_end++);
	if(*id_end != '.')
		return EXIT_FAILURE;

	if(strncmp(id_end + 1, "RX", strlen("RX")))//Only the upper-case literal ".RX" is accepted
		return EXIT_FAILURE;
	rx_begin = id_end + 1 + strlen("RX");
	if(!g_ascii_isdigit(*rx_begin) || *rx_begin == '0')//No leading zero, digits only
		return EXIT_FAILURE;
	for(rx_end = rx_begin; g_ascii_isdigit(*rx_end); rx_end++);
	if(*rx_end != '.')
		return EXIT_FAILURE;

	errno = 0;
	parsed_id = strtoul(id_begin, &parse_end, 10);
	if(parse_end != id_end || errno == ERANGE || parsed_id == 0 || parsed_id > UINT32_MAX)
		return EXIT_FAILURE;

	errno = 0;
	parsed_rx = strtoul(rx_begin, &parse_end, 10);
	if(parse_end != rx_end || errno == ERANGE || parsed_rx == 0 || parsed_rx > IOBOX_Amount_of_All_RXs)
		return EXIT_FAILURE;

	tail = rx_end + 1;
	if(!strcmp(tail, "Status"))
	{
		parsed_ch = IOBOX_RX_Status_link_channel;
	}
	else if(!strcmp(tail, "Success"))
	{
		parsed_ch = IOBOX_RX_Success_link_channel;
	}
	else
	{
		if(strncmp(tail, "CH", strlen("CH")))//Only ".CH", "Status" or "Success" are accepted after the receiver
			return EXIT_FAILURE;
		ch_begin = tail + strlen("CH");
		if(!g_ascii_isdigit(*ch_begin) || *ch_begin == '0')//No leading zero, digits only
			return EXIT_FAILURE;
		errno = 0;
		parsed_ch = strtoul(ch_begin, &parse_end, 10);
		if(*parse_end != '\0' || errno == ERANGE || parsed_ch == 0 || parsed_ch > IOBOX_Amount_of_channels)
			return EXIT_FAILURE;//parse_end must reach the end of the string: no trailing text allowed
	}

	*identifier = (unsigned int)parsed_id;
	*receiver = (unsigned char)parsed_rx;
	*channel = (unsigned char)parsed_ch;
	return EXIT_SUCCESS;
}

/*
 * Strict, full-string decoder for the canonical MTI anchor grammar:
 *     <identifier>.TC16.CH<1..16>
 *     <identifier>.TC8.CH<1..8>
 *     <identifier>.TC4.CH<1..4>
 *     <identifier>.QUAD.CH<1..2>
 *     <identifier>.ID:<1..255>.CH<1..4>
 * identifier: uint32, no leading zero, full string consumed. The literal
 * "RMSW/MUX" (a runtime radio mode string) is never itself a valid anchor
 * token; Mini-RMSW devices are only reachable through the "ID:<id>" form.
 * tele_ID is unsigned char in struct Link_entry, so an ID>255 is rejected
 * outright rather than silently truncated by an unchecked atoi()+assignment,
 * as the previous sscanf-based parser did.
 * *tele_type_or_id receives RMSW_MUX or the matching
 * enum MTI_Telemetry_Dev_type_enum value -- exactly the value struct
 * Link_entry's rxNum_teleType_or_value field already holds, so callers can
 * assign it directly; *tele_ID is only meaningful when *tele_type_or_id == RMSW_MUX.
 * This is the single decoder shared by validate_anchor_comp() and
 * XML_doc_to_List_ISO_Channels(); it must never be re-implemented a second time.
 * Return 0 on success, or -1 on failure.
 */
static int decode_mti_anchor(const char *anchor_str, unsigned int *identifier, unsigned char *tele_type_or_id,
                              unsigned char *tele_ID, unsigned char *channel)
{
	const char *id_begin, *id_end, *tail, *rmsw_id_begin, *rmsw_id_end, *ch_begin;
	char *parse_end;
	unsigned long parsed_id, parsed_tele_id, parsed_ch;
	unsigned char type_out, max_channel, tele_id_out = 0;

	if(!anchor_str || !identifier || !tele_type_or_id || !tele_ID || !channel)
		return EXIT_FAILURE;

	id_begin = anchor_str;
	if(!g_ascii_isdigit(*id_begin) || *id_begin == '0')//No leading zero, digits only
		return EXIT_FAILURE;
	for(id_end = id_begin; g_ascii_isdigit(*id_end); id_end++);
	if(*id_end != '.')
		return EXIT_FAILURE;

	errno = 0;
	parsed_id = strtoul(id_begin, &parse_end, 10);
	if(parse_end != id_end || errno == ERANGE || parsed_id == 0 || parsed_id > UINT32_MAX)
		return EXIT_FAILURE;

	tail = id_end + 1;

	if(!strncmp(tail, "ID:", strlen("ID:")))
	{
		rmsw_id_begin = tail + strlen("ID:");
		if(!g_ascii_isdigit(*rmsw_id_begin) || *rmsw_id_begin == '0')//No leading zero, digits only
			return EXIT_FAILURE;
		for(rmsw_id_end = rmsw_id_begin; g_ascii_isdigit(*rmsw_id_end); rmsw_id_end++);
		if(*rmsw_id_end != '.')
			return EXIT_FAILURE;

		errno = 0;
		parsed_tele_id = strtoul(rmsw_id_begin, &parse_end, 10);
		if(parse_end != rmsw_id_end || errno == ERANGE || parsed_tele_id == 0 || parsed_tele_id > 255)
			return EXIT_FAILURE;//tele_ID is unsigned char in struct Link_entry: never silently truncate an overflowing ID

		type_out = RMSW_MUX;
		tele_id_out = (unsigned char)parsed_tele_id;
		max_channel = 4;//Mini-RMSW: RMSW_MUX_Mini_data_struct.meas_data[4]
		ch_begin = rmsw_id_end + 1;
	}
	else if(!strncmp(tail, "TC16", strlen("TC16")) && tail[strlen("TC16")] == '.')
	{
		type_out = Tele_TC16;
		max_channel = 16;
		ch_begin = tail + strlen("TC16") + 1;
	}
	else if(!strncmp(tail, "TC8", strlen("TC8")) && tail[strlen("TC8")] == '.')
	{
		type_out = Tele_TC8;
		max_channel = 8;
		ch_begin = tail + strlen("TC8") + 1;
	}
	else if(!strncmp(tail, "TC4", strlen("TC4")) && tail[strlen("TC4")] == '.')
	{
		type_out = Tele_TC4;
		max_channel = 4;
		ch_begin = tail + strlen("TC4") + 1;
	}
	else if(!strncmp(tail, "QUAD", strlen("QUAD")) && tail[strlen("QUAD")] == '.')
	{
		type_out = Tele_quad;
		max_channel = 2;
		ch_begin = tail + strlen("QUAD") + 1;
	}
	else
		return EXIT_FAILURE;//Includes the literal "RMSW/MUX" telemetry-mode string: not a valid direct anchor token

	if(strncmp(ch_begin, "CH", strlen("CH")))
		return EXIT_FAILURE;
	ch_begin += strlen("CH");
	if(!g_ascii_isdigit(*ch_begin) || *ch_begin == '0')//No leading zero, digits only
		return EXIT_FAILURE;
	errno = 0;
	parsed_ch = strtoul(ch_begin, &parse_end, 10);
	if(*parse_end != '\0' || errno == ERANGE || parsed_ch == 0 || parsed_ch > max_channel)
		return EXIT_FAILURE;//parse_end must reach the end of the string: no trailing text allowed

	*identifier = (unsigned int)parsed_id;
	*tele_type_or_id = type_out;
	*tele_ID = tele_id_out;
	*channel = (unsigned char)parsed_ch;
	return EXIT_SUCCESS;
}

//Function that validate the anchor's components. Return 0 on success or -1 on failure.
int validate_anchor_comp(char *anchor_str, char handler_type)
{
	unsigned int nox_sensor_address;
	unsigned char nox_measurement;
	char nox_can_if[Dev_or_Bus_name_str_size];
	if(!anchor_str)
		return EXIT_FAILURE;
	//Every interface below is validated exclusively by its own decode_*_anchor()
	//function; there must never be a second, independent sscanf/atoi/strstr
	//grammar for the same interface.
	switch(handler_type)
	{
		case IOBOX:
		{
			unsigned int iobox_id;
			unsigned char iobox_rx, iobox_ch;
			if(decode_iobox_anchor(anchor_str, &iobox_id, &iobox_rx, &iobox_ch))
				return EXIT_FAILURE;
			break;
		}
		case MDAQ://MDAQ device type is retired; INTERFACE_TYPE="MDAQ" is not a supported anchor.
			return EXIT_FAILURE;
		case SDAQ:
		{
			unsigned int sdaq_serial;
			unsigned char sdaq_channel;
			if(decode_sdaq_anchor(anchor_str, &sdaq_serial, &sdaq_channel))
				return EXIT_FAILURE;
			break;
		}
		case MTI:
		{
			unsigned int mti_id;
			unsigned char mti_type, mti_tele_id, mti_ch;
			if(decode_mti_anchor(anchor_str, &mti_id, &mti_type, &mti_tele_id, &mti_ch))
				return EXIT_FAILURE;
			break;
		}
		case NOX:
			if(decode_nox_anchor(anchor_str, nox_can_if, sizeof(nox_can_if),
			                     &nox_sensor_address, &nox_measurement))
				return EXIT_FAILURE;
			break;
		default: return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int Morfeas_opc_ua_config_valid(xmlNode *root_element)
{
	union check_flags{
		struct xml_check_flags{
			unsigned interface_name : 1;
			unsigned iso_channel : 1;
			unsigned anchor : 1;
		}as_struct;
		unsigned char as_byte;
	}fl = {.as_byte = 0};
	xmlNode *check_element, *element;
	char *content, *iso_channel, *dev_type_str;
	unsigned int if_name_okay;
	if(!root_element->children)
		return EXIT_SUCCESS;
	//Check for Empty XML nodes content and for Invalid Interface_name content
	for(element = root_element->children; element; element = element->next)
	{
		if(element->type == XML_ELEMENT_NODE)
		{
			for(check_element = element->children; check_element; check_element = check_element->next)
			{
				if(check_element->type == XML_ELEMENT_NODE)
				{
					if(!check_element->children)//Empty Check
					{
						fprintf(stderr, "\nNode:%s (on line:%d) found to have zero content!!!!\n\n", check_element->name, check_element->line);
						return EXIT_FAILURE;
					}
					else if(!strcmp((char *)(check_element->name), "INTERFACE_TYPE"))//Invalid Interface name
					{
						fl.as_struct.interface_name = 1;
						if_name_okay = 0;
						for(int i=0; Morfeas_IPC_handler_type_name[i]; i++)
							if(!strcmp(Morfeas_IPC_handler_type_name[i], (char *)(check_element->children->content)))
								if_name_okay = 1;
						if(!if_name_okay)
						{
							fprintf(stderr, "\nContent:\"%s\" of Node:\"INTERFACE_TYPE\" (on line: %d) is Out of Range (",
								check_element->children->content,
							    check_element->line);
							for(int j=0; Morfeas_IPC_handler_type_name[j]; j++)
							{
								fprintf(stderr, "%s",Morfeas_IPC_handler_type_name[j]);
								if(Morfeas_IPC_handler_type_name[j+1])
									fprintf(stderr,", ");
							}
							fprintf(stderr,")!!!!\n\n");
							return EXIT_FAILURE;
						}
					}
				}
			}
		}
	}
	//Check for duplicate ISO_CHANNEL
	for(element = root_element->children; element->next; element = element->next)
	{   //print_XML_node(element);
		if(element->type == XML_ELEMENT_NODE)
		{
			if((iso_channel = XML_node_get_content(element, "ISO_CHANNEL")))
			{
				fl.as_struct.iso_channel = 1;
				for(check_element = element->next; check_element; check_element = check_element->next)
				{
					if((content = XML_node_get_content(check_element, "ISO_CHANNEL")))
					{
						if(!strcmp(content, iso_channel))
						{
							fprintf(stderr, "\nISO_CHANNEL: \"%s\" Found multiple times!!!!\n\n", iso_channel);
							return EXIT_FAILURE;
						}
					}
				}
			}
		}
	}
	//Check for invalid contents in ISO_CHANNEL and ANCHOR
	for(check_element = root_element->children; check_element; check_element = check_element->next)
	{
		if((iso_channel = XML_node_get_content(check_element, "ISO_CHANNEL")))
		{
			if(strlen(iso_channel)>=ISO_channel_name_size)
			{
				fprintf(stderr, "\nISO_CHANNEL : \"%s\" is too long (>=%u) !!!!\n\n", iso_channel, ISO_channel_name_size);
				return EXIT_FAILURE;
			}
			if(strstr(iso_channel,"."))//'.' is illegal character for the ISO_Channel
			{
				fprintf(stderr, "\nISO_CHANNEL : \"%s\" is NOT valid (contains '.') !!!!\n\n", iso_channel);
				return EXIT_FAILURE;
			}
		}
		if((content = XML_node_get_content(check_element, "ANCHOR"))
		  &&(dev_type_str = XML_node_get_content(check_element, "INTERFACE_TYPE")))
		{
			int anchor_if_type = if_type_str_2_num(dev_type_str);
			fl.as_struct.anchor = 1;
			//TODO: More checks on values
			if(validate_anchor_comp(content, anchor_if_type))
			{
				fprintf(stderr, "\nANCHOR :\"%s\" of ISO_CHANNEL :\"%s\" (Type:\"%s\") is NOT valid!!!!\n\n", content,
																												  iso_channel,
																												  dev_type_str);
				return EXIT_FAILURE;
			}
			//IOBOX/MTI/NOX own their UNIT statically from the XML (unlike SDAQ,
			//which is not XML-owned and is read from live channel runtime); the
			//DTD only makes UNIT optional, so this interface-specific
			//non-empty requirement has to be enforced here.
			if(anchor_if_type == IOBOX || anchor_if_type == MTI || anchor_if_type == NOX)
			{
				char *unit_content = XML_node_get_content(check_element, "UNIT");
				int unit_is_blank = 1;
				if(unit_content)
				{
					for(char *p = unit_content; *p; p++)
					{
						if(!g_ascii_isspace(*p))
						{
							unit_is_blank = 0;
							break;
						}
					}
				}
				if(unit_is_blank)
				{
					fprintf(stderr, "\nISO_CHANNEL :\"%s\" (Type:\"%s\") is missing a non-empty UNIT!!!!\n\n",
							iso_channel ? iso_channel : "?", dev_type_str);
					return EXIT_FAILURE;
				}
			}
			//Reject two ISO_CHANNELs that resolve to the same parsed runtime
			//source, per interface. Comparison is always on decoded fields,
			//never on raw ANCHOR text: same identifier/IP with a different
			//receiver/channel/telemetry-type/measurement is a legal pair, not
			//a duplicate.
			if(anchor_if_type == SDAQ)
			{
				unsigned int serial, other_serial;
				unsigned char channel_num, other_channel;
				char *other_content, *other_dev_type_str, *other_iso_channel;
				xmlNode *other_element;

				decode_sdaq_anchor(content, &serial, &channel_num);//Already validated above
				for(other_element = check_element->next; other_element; other_element = other_element->next)
				{
					if(!(other_content = XML_node_get_content(other_element, "ANCHOR")) ||
					   !(other_dev_type_str = XML_node_get_content(other_element, "INTERFACE_TYPE")) ||
					   if_type_str_2_num(other_dev_type_str) != SDAQ ||
					   decode_sdaq_anchor(other_content, &other_serial, &other_channel))
						continue;
					if(serial == other_serial && channel_num == other_channel)
					{
						other_iso_channel = XML_node_get_content(other_element, "ISO_CHANNEL");
						fprintf(stderr, "\nANCHOR :\"%s\" of ISO_CHANNEL :\"%s\" duplicates the SDAQ source already used by ISO_CHANNEL :\"%s\"!!!!\n\n",
								other_content, other_iso_channel ? other_iso_channel : "?", iso_channel ? iso_channel : "?");
						return EXIT_FAILURE;
					}
				}
			}
			else if(anchor_if_type == IOBOX)
			{
				unsigned int id, other_id;
				unsigned char rx, other_rx, ch, other_ch;
				char *other_content, *other_dev_type_str, *other_iso_channel;
				xmlNode *other_element;

				decode_iobox_anchor(content, &id, &rx, &ch);//Already validated above
				for(other_element = check_element->next; other_element; other_element = other_element->next)
				{
					if(!(other_content = XML_node_get_content(other_element, "ANCHOR")) ||
					   !(other_dev_type_str = XML_node_get_content(other_element, "INTERFACE_TYPE")) ||
					   if_type_str_2_num(other_dev_type_str) != IOBOX ||
					   decode_iobox_anchor(other_content, &other_id, &other_rx, &other_ch))
						continue;
					if(id == other_id && rx == other_rx && ch == other_ch)
					{
						other_iso_channel = XML_node_get_content(other_element, "ISO_CHANNEL");
						fprintf(stderr, "\nANCHOR :\"%s\" of ISO_CHANNEL :\"%s\" duplicates the IOBOX source already used by ISO_CHANNEL :\"%s\"!!!!\n\n",
								other_content, other_iso_channel ? other_iso_channel : "?", iso_channel ? iso_channel : "?");
						return EXIT_FAILURE;
					}
				}
			}
			else if(anchor_if_type == MTI)
			{
				unsigned int id, other_id;
				unsigned char type, other_type, tele_id, other_tele_id, ch, other_ch;
				char *other_content, *other_dev_type_str, *other_iso_channel;
				xmlNode *other_element;

				decode_mti_anchor(content, &id, &type, &tele_id, &ch);//Already validated above
				for(other_element = check_element->next; other_element; other_element = other_element->next)
				{
					if(!(other_content = XML_node_get_content(other_element, "ANCHOR")) ||
					   !(other_dev_type_str = XML_node_get_content(other_element, "INTERFACE_TYPE")) ||
					   if_type_str_2_num(other_dev_type_str) != MTI ||
					   decode_mti_anchor(other_content, &other_id, &other_type, &other_tele_id, &other_ch))
						continue;
					if(id == other_id && type == other_type && tele_id == other_tele_id && ch == other_ch)
					{
						other_iso_channel = XML_node_get_content(other_element, "ISO_CHANNEL");
						fprintf(stderr, "\nANCHOR :\"%s\" of ISO_CHANNEL :\"%s\" duplicates the MTI source already used by ISO_CHANNEL :\"%s\"!!!!\n\n",
								other_content, other_iso_channel ? other_iso_channel : "?", iso_channel ? iso_channel : "?");
						return EXIT_FAILURE;
					}
				}
			}
			else if(anchor_if_type == NOX)
			{
				char can_if[Dev_or_Bus_name_str_size], other_can_if[Dev_or_Bus_name_str_size];
				unsigned int addr, other_addr;
				unsigned char meas, other_meas;
				char *other_content, *other_dev_type_str, *other_iso_channel;
				xmlNode *other_element;

				decode_nox_anchor(content, can_if, sizeof(can_if), &addr, &meas);//Already validated above
				for(other_element = check_element->next; other_element; other_element = other_element->next)
				{
					if(!(other_content = XML_node_get_content(other_element, "ANCHOR")) ||
					   !(other_dev_type_str = XML_node_get_content(other_element, "INTERFACE_TYPE")) ||
					   if_type_str_2_num(other_dev_type_str) != NOX ||
					   decode_nox_anchor(other_content, other_can_if, sizeof(other_can_if), &other_addr, &other_meas))
						continue;
					if(addr == other_addr && meas == other_meas && !strcmp(can_if, other_can_if))
					{
						other_iso_channel = XML_node_get_content(other_element, "ISO_CHANNEL");
						fprintf(stderr, "\nANCHOR :\"%s\" of ISO_CHANNEL :\"%s\" duplicates the NOX source already used by ISO_CHANNEL :\"%s\"!!!!\n\n",
								other_content, other_iso_channel ? other_iso_channel : "?", iso_channel ? iso_channel : "?");
						return EXIT_FAILURE;
					}
				}
			}
		}
	}
	if(!fl.as_byte)
	{
		fprintf(stderr, "\nConfiguration XML have missing nodes !!!!\n\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

//GCompareFunc used in g_slist_find_custom
gint List_Links_cmp (gconstpointer a, gconstpointer b)
{
	const char *node_data = ((struct Link_entry*)a)->ISO_channel_name, *ISO_Channel_name = b;
	return strcmp(node_data, ISO_Channel_name);
}

//Constructor of Entry for List with data type "struct Link_entry"
struct Link_entry* new_Link_entry()
{
    struct Link_entry *new_node = g_slice_new0(struct Link_entry);
    return new_node;
}

//Deconstructor for Data of Lists with data type "struct Link_entry"
void free_Link_entry(gpointer data)
{
	struct Link_entry *node = (struct Link_entry *) data;
	if(node->CAN_IF_name)
	{
		free(node->CAN_IF_name);
		node->CAN_IF_name = NULL;
	}
	g_slice_free(struct Link_entry, data);
}
/*
void print_List (gpointer data, gpointer user_data)
{
	struct Link_entry *node_data = data;
	printf("Data of Node :\n");
	printf("\tISO_channel_name: %s\n", node_data->ISO_channel_name);
	printf("\tInterface_type: %s\n", node_data->interface_type);
	printf("\tIdentifier: %u\n", node_data->identifier);
	printf("\tChannel: %hhu\n", node_data->channel);
	if(node_data->CAN_IF_name)
		printf("\CAN_IF_name: %s\n", node_data->CAN_IF_name);
}
*/
int Morfeas_OPC_UA_calc_diff_of_ISO_Channel_node(xmlNode *root_element, GSList **cur_Links)
{
	xmlNode *check_element;
	GSList *node;
	char *iso_channel;
	if(cur_Links && root_element->children)
	{
		for(check_element = root_element->children; check_element; check_element = check_element->next)
		{
			if((iso_channel = XML_node_get_content(check_element, "ISO_CHANNEL")))
			{
				if((node = g_slist_find_custom(*cur_Links, iso_channel, List_Links_cmp)))
				{
					free_Link_entry(node->data);
					*cur_Links = g_slist_delete_link(*cur_Links, node);
				}
			}
		}
	}
	return EXIT_SUCCESS;
}

int XML_doc_to_List_ISO_Channels(xmlNode *root_element, GSList **cur_Links)
{
	unsigned int nox_sensor_address;
	unsigned char nox_measurement;
	xmlNode *check_element;
	struct Link_entry *list_cur_Links_node_data;
	char *iso_channel_str, *dev_type_str, *anchor_ptr;
	char nox_can_if[Dev_or_Bus_name_str_size];

	g_slist_free_full(*cur_Links, free_Link_entry);//Free List cur_Links
	*cur_Links = NULL;
	if(root_element->children)
	{
		for(check_element = root_element->children; check_element; check_element = check_element->next)
		{
			iso_channel_str = XML_node_get_content(check_element, "ISO_CHANNEL");
			dev_type_str = XML_node_get_content(check_element, "INTERFACE_TYPE");
			anchor_ptr = XML_node_get_content(check_element, "ANCHOR");
			if(iso_channel_str && dev_type_str && anchor_ptr)
			{
				list_cur_Links_node_data = new_Link_entry();
				if(list_cur_Links_node_data)
				{
					memccpy(&(list_cur_Links_node_data->ISO_channel_name), iso_channel_str, '\0', sizeof(list_cur_Links_node_data->ISO_channel_name));
					switch((list_cur_Links_node_data->interface_type_num = if_type_str_2_num(dev_type_str)))
					{
						case IOBOX:
						{
							unsigned int iobox_id;
							unsigned char iobox_rx, iobox_ch;
							if(decode_iobox_anchor(anchor_ptr, &iobox_id, &iobox_rx, &iobox_ch))
							{
								free_Link_entry(list_cur_Links_node_data);
								continue;
							}
							list_cur_Links_node_data->identifier = iobox_id;
							list_cur_Links_node_data->rxNum_teleType_or_value = iobox_rx;
							list_cur_Links_node_data->channel = iobox_ch;
							break;
						}
						case MDAQ:
							sscanf(anchor_ptr, "%u.CH%hhu.Val%hhu", &(list_cur_Links_node_data->identifier),
																	&(list_cur_Links_node_data->channel),
																	&(list_cur_Links_node_data->rxNum_teleType_or_value));
							break;
						case SDAQ:
						{
							unsigned int sdaq_serial;
							unsigned char sdaq_channel;
							if(decode_sdaq_anchor(anchor_ptr, &sdaq_serial, &sdaq_channel))
							{
								free_Link_entry(list_cur_Links_node_data);
								continue;
							}
							list_cur_Links_node_data->identifier = sdaq_serial;
							list_cur_Links_node_data->channel = sdaq_channel;
							break;
						}
						case MTI:
						{
							unsigned int mti_id;
							unsigned char mti_type, mti_tele_id, mti_ch;
							if(decode_mti_anchor(anchor_ptr, &mti_id, &mti_type, &mti_tele_id, &mti_ch))
							{
								free_Link_entry(list_cur_Links_node_data);
								continue;
							}
							list_cur_Links_node_data->identifier = mti_id;
							list_cur_Links_node_data->rxNum_teleType_or_value = mti_type;
							list_cur_Links_node_data->tele_ID = mti_tele_id;
							list_cur_Links_node_data->channel = mti_ch;
							break;
						}
						case NOX:
							if(decode_nox_anchor(anchor_ptr, nox_can_if, sizeof(nox_can_if),
							                     &nox_sensor_address, &nox_measurement))
							{
								free_Link_entry(list_cur_Links_node_data);
								continue;
							}
							if(!(list_cur_Links_node_data->CAN_IF_name = strdup(nox_can_if)))
							{
								fprintf(stderr,"Memory error!\n");
								exit(EXIT_FAILURE);
							}
							list_cur_Links_node_data->channel = (unsigned char)nox_sensor_address;
							list_cur_Links_node_data->rxNum_teleType_or_value = nox_measurement;
							break;
					}
					*cur_Links = g_slist_append(*cur_Links, list_cur_Links_node_data);
				}
				else
				{
					fprintf(stderr,"Memory error!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	return EXIT_SUCCESS;
}

//Return 0 if file is accessible, or 1 if configs_dir does not exist, otherwise -1
int check_file(const char *configs_dir, const char *file_name)
{
	DIR *configs_dir_ptr;
	int retval = 0;
	char *abs_file_path;
	if(!configs_dir || !file_name)
		return -1;
	if(!(configs_dir_ptr = opendir(configs_dir)))
		return 1;
	closedir(configs_dir_ptr);
	abs_file_path = calloc(strlen(configs_dir)+strlen(file_name)+5, sizeof(char));
	if(!abs_file_path)
	{
		fprintf(stderr, "Memory Error!!!\n");
		exit(EXIT_FAILURE);
	}
	strcat(abs_file_path, configs_dir);
	if(abs_file_path[strlen(abs_file_path)-1]!='/')
		abs_file_path[strlen(abs_file_path)]='/';
	strcat(abs_file_path, file_name);
	if(access((const char*)abs_file_path, R_OK))
		retval = -1;
	free(abs_file_path);
	return retval;
}

int Morfeas_daemon_config_valid(xmlNode *root_element)
{
	xmlNode *xml_node, *components_head_node, *check_node;
	xmlChar *content, *ipv4_addr, *dev_name, *config_Dir;
	//Check for nodes with Empty content
	if((xml_node = scaning_XML_nodes_for_empty(root_element)))
	{
		fprintf(stderr, "\nNode \"%s\" @Line: %d does not have content !!!!\n\n", xml_node->name, xml_node->line);
		return EXIT_FAILURE;
	}
	//Check for existence of node "CONFIGS_DIR"
	if(!(config_Dir = (xmlChar *) XML_node_get_content(root_element, "CONFIGS_DIR")))
	{
		fprintf(stderr, "\"CONFIGS_DIR\" XML node not found\n");
		return EXIT_FAILURE;
	}
	//Check for existence of node "LOGGERS_DIR"
	if(!XML_node_get_content(root_element, "LOGGERS_DIR"))
	{
		fprintf(stderr, "\"LOGGERS_DIR\" XML node not found\n");
		return EXIT_FAILURE;
	}
	//Check for existence of node "LOGSTAT_DIR"
	if(!XML_node_get_content(root_element, "LOGSTAT_DIR"))
	{
		fprintf(stderr, "\"LOGSTAT_DIR\" XML node not found\n");
		return EXIT_FAILURE;
	}
	//Check for existence of node "COMPONENTS"
	if(!(components_head_node = get_XML_node(root_element, "COMPONENTS")))
	{
		fprintf(stderr, "\"COMPONENTS\" XML node not found\n");
		return EXIT_FAILURE;
	}
	//Scan children of node "COMPONENTS" for Attribute errors
	xml_node = components_head_node->children;
	while(xml_node)
	{
		if(xml_node->type == XML_ELEMENT_NODE)
		{
			if((content = xmlGetProp(xml_node, BAD_CAST"Disable")))
			{
				if(strcmp((char *)content, "true") && strcmp((char *)content, "false"))
				{
					fprintf(stderr, "Attribute Value: \"%s\" for XML node \"COMPONENTS\"(Line:%d) is out of range (true,false)\n",
						(char*)content, xml_node->line);
					xmlFree(content);
					return EXIT_FAILURE;
				}
				xmlFree(content);
			}
			else
			{
				fprintf(stderr, "Unknown Attribute found at Line:%d\n", xml_node->line);
				return EXIT_FAILURE;
			}
		}
		xml_node = xml_node->next;
	}
	//Check for existence of node "OPC_UA_SERVER" and validate it's contents
	if((xml_node = get_XML_node(components_head_node, "OPC_UA_SERVER")))
	{
		if((content = (xmlChar *) XML_node_get_content(xml_node, "APP_NAME")))
		{
			if(strstr((char*)content, " "))
			{
				fprintf(stderr, "Content (\"%s\") of XML node \"APP_NAME\" is invalid (contain Whitespaces)!!!\n",content);
				return EXIT_FAILURE;
			}
		}
		else
		{
			fprintf(stderr, "\"APP_NAME\" XML child node of \"OPC_UA_SERVER\" not found\n");
			return EXIT_FAILURE;
		}
	}
	else
	{
		fprintf(stderr, "\"OPC_UA_SERVER\" XML node not found\n");
		return EXIT_FAILURE;
	}
	//Scan all SDAQ_HANDLER nodes for CANBUS_IF with duplicate content
	xml_node = components_head_node->children;
	while(xml_node)
	{
		if(xml_node->type == XML_ELEMENT_NODE)
		{
			if(!strcmp((char*)xml_node->name, "SDAQ_HANDLER"))
			{
				content = (xmlChar *) XML_node_get_content(xml_node, "CANBUS_IF");
				check_node = xml_node->next;
				while(check_node)
				{
					if(check_node->type == XML_ELEMENT_NODE)
					{
						if(!strcmp((char*)check_node->name, "SDAQ_HANDLER"))
						{
							if(!strcmp((char*)content, XML_node_get_content(check_node, "CANBUS_IF")))
							{
								fprintf(stderr, "XML Node with name \"CANBUS_IF\" with content \"%s\" for SDAQ_HANDLER found multiple times!!!\n", content);
								return EXIT_FAILURE;
							}
						}
					}
					check_node = check_node->next;
				}
			}
		}
		xml_node = xml_node->next;
	}
	//Scan all NOX_HANDLER nodes for CANBUS_IF with duplicate content
	xml_node = components_head_node->children;
	while(xml_node)
	{
		if(xml_node->type == XML_ELEMENT_NODE)
		{
			if(!strcmp((char*)xml_node->name, "NOX_HANDLER"))
			{
				content = (xmlChar *) XML_node_get_content(xml_node, "CANBUS_IF");
				check_node = xml_node->next;
				while(check_node)
				{
					if(check_node->type == XML_ELEMENT_NODE)
					{
						if(!strcmp((char*)check_node->name, "NOX_HANDLER"))
						{
							if(!strcmp((char*)content, XML_node_get_content(check_node, "CANBUS_IF")))
							{
								fprintf(stderr, "XML Node with name \"CANBUS_IF\" with content \"%s\" for NOX_HANDLER found multiple times!!!\n",content);
								return EXIT_FAILURE;
							}
						}
					}
					check_node = check_node->next;
				}
			}
		}
		xml_node = xml_node->next;
	}
	//Check duplicate usage for contents of CANBUS_IF
	xmlChar *check_content;
	xml_node = components_head_node->children;
	while(xml_node)
	{
		if(xml_node->type == XML_ELEMENT_NODE )
		{
			if(!getprop_disable(xml_node) && (content = (xmlChar *) XML_node_get_content(xml_node, "CANBUS_IF")))
			{
				check_node = xml_node->next;
				while(check_node)
				{
					if(check_node->type == XML_ELEMENT_NODE)
					{
						if(!getprop_disable(check_node) && (check_content = (xmlChar *) XML_node_get_content(check_node, "CANBUS_IF")))
						{
							if(!strcmp((char*)content, (char*)check_content))
							{
								fprintf(stderr, "\"CANBUS_IF\":\"%s\" found to be used in multiple Handlers!!!\n",content);
								return EXIT_FAILURE;
							}
						}
					}
					check_node = check_node->next;
				}
			}
		}
		xml_node = xml_node->next;
	}
	//Scan nodes MDAQ,IOBOX,MTI for child node with duplicate and validate content
	xml_node = components_head_node->children;
	while(xml_node)
	{
		if(xml_node->type == XML_ELEMENT_NODE)
		{
			if(!strcmp((char*)xml_node->name, "MDAQ_HANDLER") ||
			   !strcmp((char*)xml_node->name, "IOBOX_HANDLER")||
			   !strcmp((char*)xml_node->name, "MTI_HANDLER"))
			{
				ipv4_addr = (xmlChar *) XML_node_get_content(xml_node, "IPv4_ADDR");
				//Check "IPv4_ADDR" content if is a valid IPv4 address
				if(!is_valid_IPv4((char *)ipv4_addr))
				{
					fprintf(stderr, "The Internet protocol version 4 address (%s) on line %d is not valid !!!\n",
									 ipv4_addr,
									 get_XML_node(xml_node, "IPv4_ADDR")->line);
					return EXIT_FAILURE;
				}
				dev_name = (xmlChar *) XML_node_get_content(xml_node, "DEV_NAME");
				//Check "DEV_NAME" content for illegal characters
				for(int i=0; dev_name[i]!='\0'; i++)
				{
					if(dev_name[i] == ' ' || dev_name[i] == '\'' || dev_name[i] == '\"')
					{
						fprintf(stderr, "Content of \"DEV_NAME\" on line %d is not valid. Contains (%c) Character !!!\n",
										get_XML_node(xml_node, "DEV_NAME")->line,
										dev_name[i]);
						return EXIT_FAILURE;
					}
					if(i>=Dev_or_Bus_name_str_size)
					{
						fprintf(stderr, "Content of \"DEV_NAME\" on line %d is too long (>=%u)!!!\n",
										get_XML_node(xml_node, "DEV_NAME")->line,
										Dev_or_Bus_name_str_size);
						return EXIT_FAILURE;
					}
				}
				check_node = xml_node->next;
				while(check_node)
				{
					if(check_node->type == XML_ELEMENT_NODE)
					{
						if(!strcmp((char*)check_node->name, "MDAQ_HANDLER") ||
						   !strcmp((char*)check_node->name, "IOBOX_HANDLER")||
						   !strcmp((char*)check_node->name, "MTI_HANDLER"))
						{
							if(!strcmp((char*)ipv4_addr, XML_node_get_content(check_node, "IPv4_ADDR")))
							{
								fprintf(stderr, "XML Node with name \"IPv4_ADDR\" and content \"%s\" found multiple times!!!\n",ipv4_addr);
								return EXIT_FAILURE;
							}
							if(!strcmp((char*)dev_name, XML_node_get_content(check_node, "DEV_NAME")))
							{
								fprintf(stderr, "XML Node with name \"DEV_NAME\" and content \"%s\" found multiple times!!!\n",dev_name);
								return EXIT_FAILURE;
							}
						}
					}
					check_node = check_node->next;
				}
			}
		}
		xml_node = xml_node->next;
	}
	return EXIT_SUCCESS;
}
