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

#include "Morfeas_Types.h"
/*
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
*/

char * XML_node_get_content(xmlNode *node, const char *node_name)
{
    xmlNode *cur_node;
	if (node->type == XML_ELEMENT_NODE)
	{
		for (cur_node = node->children; cur_node; cur_node = cur_node->next)
		{
			if (cur_node->type == XML_ELEMENT_NODE)
			{
				if(!strcmp((const char *)cur_node->name, node_name))
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

#define max_arg_range 2
#define anchor_check_buff_size 100
int Morfeas_opc_ua_config_valid(xmlNode *root_element)
{
	xmlNode *check_element;
	char *content, *anchor_arg[max_arg_range], *iso_channel;
	char anchor_check[anchor_check_buff_size], arg_check[max_arg_range][anchor_check_buff_size];
	int arg_to_int[max_arg_range]= {1};
	for(check_element = root_element->children; check_element; check_element = check_element->next)
	{
		if((iso_channel = XML_node_get_content(check_element, "ISO_CHANNEL")))
		{
			if(strstr(iso_channel,"."))
			{
				fprintf(stderr, "\nISO_CHANNEL : \"%s\" is NOT valid !!!!\n\n", iso_channel);
				return 1;
			}
		}
		if((content = XML_node_get_content(check_element, "ANCHOR")))
		{
			strcpy(anchor_check, content);
			anchor_arg[0] = strtok(anchor_check, ".");
			anchor_arg[1] = strtok(NULL, "CH");
			for(int i=0; i<max_arg_range; i++)
				if(anchor_arg[i])
					arg_to_int[i] = atoi(anchor_arg[i]);
			sprintf(arg_check[0], "%u", arg_to_int[0]);
			sprintf(arg_check[1], *anchor_arg[1]=='0'?"%02u":"%u", arg_to_int[1]);
			if((!arg_to_int[0] || strcmp(arg_check[0], anchor_arg[0]))
			|| (!arg_to_int[1] || strcmp(arg_check[1], anchor_arg[1])))
			{
				fprintf(stderr, "\nANCHOR : \"%s\" of ISO_CHANNEL : \"%s\" is NOT valid !!!!\n\n", content, iso_channel);
				return 1;
			}
		}
	}
	return 0;
}

//GCompareFunc used in g_slist_find_custom, compare
gint ISO_Channel_cmp (gconstpointer a, gconstpointer b)
{
	const char *node_data = ((struct ISO_Channel_name*)a)->ISO_channel_name_str, *ISO_Channel_name_str = b;
	return strcmp(node_data, ISO_Channel_name_str);
}
//Constructor of Entry for List with data type "struct ISO_Channel_name"
struct ISO_Channel_name* new_ISO_Channel_entry()
{
    struct ISO_Channel_name *new_node = (struct ISO_Channel_name *) g_slice_alloc0(sizeof(struct ISO_Channel_name));
    return new_node;
}

//Deconstructor for Data of Lists with data type "struct ISO_Channel_name"
void free_ISO_Channel_name(gpointer data)
{
	g_slice_free(struct ISO_Channel_name, data);
}

void print_List (gpointer data, gpointer user_data)
{
	struct ISO_Channel_name *node_data = data;
	printf("List %s, Data of Node = %s\n", (char*)user_data, node_data->ISO_channel_name_str);
}

int Morfeas_OPC_UA_calc_diff_of_ISO_Channel_node(xmlNode *root_element, GSList **cur_ISOChannels)
{
	xmlNode *check_element;
	GSList *node;
	char *iso_channel;
	if(cur_ISOChannels)
	{
		for(check_element = root_element->children; check_element; check_element = check_element->next)
		{
			if((iso_channel = XML_node_get_content(check_element, "ISO_CHANNEL")))
			{
				if((node = g_slist_find_custom(*cur_ISOChannels, iso_channel, ISO_Channel_cmp)))
				{
					free_ISO_Channel_name(node->data);
					*cur_ISOChannels = g_slist_delete_link(*cur_ISOChannels, node);
				}
			}
		}
	}
	return EXIT_SUCCESS;
}

int XML_doc_to_List_ISO_Channels(xmlNode *root_element, GSList **cur_ISOChannels)
{
	xmlNode *check_element;
	struct ISO_Channel_name *list_cur_ISOChannels_node_data;
	char *iso_channel;

	g_slist_free_full(*cur_ISOChannels, free_ISO_Channel_name);//Free List cur_ISOChannels
	*cur_ISOChannels = NULL;
	for(check_element = root_element->children; check_element; check_element = check_element->next)
	{
		if((iso_channel = XML_node_get_content(check_element, "ISO_CHANNEL")))
		{
			list_cur_ISOChannels_node_data = new_ISO_Channel_entry();
			if(list_cur_ISOChannels_node_data)
			{
				memccpy(&(list_cur_ISOChannels_node_data->ISO_channel_name_str), iso_channel, '\0', sizeof(struct ISO_Channel_name));
				*cur_ISOChannels = g_slist_append(*cur_ISOChannels, list_cur_ISOChannels_node_data);
			}
			else
			{
				fprintf(stderr,"Memory error!\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	return EXIT_SUCCESS;
}















