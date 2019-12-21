/*
File: Morfeas_XML.c, Implementation of functions for read XML files
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include <glib.h>
#include <gmodule.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


void print_XML_node(xmlNode * node)
{
    xmlNode *cur_node;
	if (node->type == XML_ELEMENT_NODE)
	{
		printf("Node name: %s\n", node->name);
		for (cur_node = node->children; cur_node; cur_node = cur_node->next)
		{
			if (cur_node->type == XML_ELEMENT_NODE)
			{
				printf("\tChild Node name: %s\n", cur_node->name);
				printf("\t\tHave contents: %s\n", cur_node->children->content);
			}
		}
	}
}

char * XML_node_get_content(xmlNode *node, const char *node_name)
{
    xmlNode *cur_node;
	if (node->type == XML_ELEMENT_NODE)
	{
		for (cur_node = node->children; cur_node; cur_node = cur_node->next)
		{
			if (cur_node->type == XML_ELEMENT_NODE)
			{
				if(!strcmp((char *)(cur_node->name), node_name))
					return (char *)(cur_node->children->content);
			}
		}
	}
	return NULL;
}

int Morfeas_XML_parsing(const char *filename, xmlDocPtr *doc)
{
    xmlParserCtxtPtr ctxt;
	//xmlNode *root_element = NULL;
    //--- create a parser context ---//
    if (!(ctxt = xmlNewParserCtxt()))
    {
        fprintf(stderr, "Failed to allocate parser context\n");
		return EXIT_FAILURE;
    }
    //--- parse the file, activating the DTD validation option ---//
    if (!(*doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID)))
    {
        fprintf(stderr, "Failed to parse %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return EXIT_FAILURE;
    }
    else
    {
		//check if validation succeeded
		if(!(ctxt->valid))
		{
        	fprintf(stderr, "Failed to validate %s\n", filename);
        	xmlFreeParserCtxt(ctxt);
        	xmlFreeDoc(*doc);
        	return EXIT_FAILURE;
        }
		xmlFreeParserCtxt(ctxt);
    }
    return EXIT_SUCCESS;
}

int Morfeas_opc_ua_config_valid(xmlNode *root_element)
{
	xmlNode *check_element;
	char *content, *anchor_arg1, *anchor_arg2, *iso_channel;
	char anchor_check[100], arg_check[2][100];
	int arg1_to_int, arg2_to_int;
	for(check_element = root_element->children; check_element; check_element = check_element->next)
	{
		if((content = XML_node_get_content(check_element, "ANCHOR")))
		{
			iso_channel = XML_node_get_content(check_element, "ISO_CHANNEL");
			strcpy(anchor_check, content);
			anchor_arg1 = strtok(anchor_check, ".");
			anchor_arg2 = strtok(NULL, ".CH");
			arg1_to_int = atoi(anchor_arg1);
			arg2_to_int = atoi(anchor_arg2);
			sprintf(arg_check[0], "%u", arg1_to_int);
			sprintf(arg_check[1], "%u", arg2_to_int);
			if((!arg1_to_int || strcmp(arg_check[0], anchor_arg1)) || (!arg2_to_int || arg2_to_int>62 || strcmp(arg_check[1], anchor_arg2)))
			{
				fprintf(stderr, "\nANCHOR : \"%s\" of ISO_CHANNEL : \"%s\" is NOT valid !!!!\n\n", content, iso_channel);
				return 1;
			}
		}
	}
	return 0;
}










