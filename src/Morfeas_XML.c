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

int Morfeas_OPC_UA_conf_XML_parsing_validation(const char *filename, xmlDocPtr *doc)
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
