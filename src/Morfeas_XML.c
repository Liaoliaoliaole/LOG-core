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
//#include "Morfeas_IPC.h"

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
				if(cur_node->children)
					printf("\t\tHave contents: %s\n", cur_node->children->content);
				else
					printf("\t\tEmpty\n");
			}
		}
	}
}
*/
char * XML_node_get_content(xmlNode *node, const char *node_name)
{
    xmlNode *cur_node;
	if (node)
	{
		for (cur_node = node->children; cur_node; cur_node = cur_node->next)
		{
			if (cur_node->type == XML_ELEMENT_NODE)
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

int Morfeas_XML_parsing(const char *filename, xmlDocPtr *doc)
{
    xmlParserCtxtPtr ctxt;
    //--- create a parser context ---//
    if (!(ctxt = xmlNewParserCtxt()))
    {
        fprintf(stderr, "Failed to allocate parser context\n");
		return EXIT_FAILURE;
    }
    //--- parse the file, activating the DTD validation option ---//
    if (!(*doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID | XML_PARSE_NOBLANKS)))
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
	xmlNode *check_element, *element;
	char *content, *iso_channel, *anchor_arg[max_arg_range] = {NULL};
	char anchor_check[anchor_check_buff_size], arg_check[max_arg_range][anchor_check_buff_size];
	unsigned int arg_to_int[max_arg_range]= {0}, if_name_okay;
	//Check for Empty XML nodes content and for Invalid Interface_name content
	for(element = root_element->children; element; element = element->next)
	{
		if (element->type == XML_ELEMENT_NODE)
		{
			for(check_element = element->children; check_element; check_element = check_element->next)
			{
				if (check_element->type == XML_ELEMENT_NODE)
				{
					if(!check_element->children)//Empty Check
					{
						fprintf(stderr, "\nNode: %s (on line: %d) found to have zero content!!!!\n\n", check_element->name, check_element->line);
						return EXIT_FAILURE;
					}
					else if(!strcmp((char *)(check_element->name), "INTERFACE_TYPE"))//Invalid Interface name
					{
						if_name_okay = 0;
						for(int i=0; Morfeas_IPC_handler_type_name[i]; i++)
							if(!strcmp(Morfeas_IPC_handler_type_name[i], (char *)(check_element->children->content)))
								if_name_okay = 1;
						if(!if_name_okay)
						{
							fprintf(stderr, "\nContent: \"%s\" of Node: \"INTERFACE_TYPE\" (on line: %d) is Out of Range (",
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
		if (element->type == XML_ELEMENT_NODE)
		{
			if((iso_channel = XML_node_get_content(element, "ISO_CHANNEL")))
			{
				for(check_element = element->next; check_element; check_element = check_element->next)
				{
					if((content = XML_node_get_content(check_element, "ISO_CHANNEL")))
					{
						if(!strcmp(content, iso_channel))
						{
							fprintf(stderr, "\nISO_CHANNEL: \"%s\" Found multiple times !!!!\n\n", iso_channel);
							return EXIT_FAILURE;
						}
					}
				}
			}
		}
	}
	//Check for invalid ANCHOR
	for(check_element = root_element->children; check_element; check_element = check_element->next)
	{
		if((iso_channel = XML_node_get_content(check_element, "ISO_CHANNEL")))
		{
			if(strstr(iso_channel,"."))//'.' is illegal character for the ISO_Channel
			{
				fprintf(stderr, "\nISO_CHANNEL : \"%s\" is NOT valid (contains '.') !!!!\n\n", iso_channel);
				return EXIT_FAILURE;
			}
		}
		if((content = XML_node_get_content(check_element, "ANCHOR")))
		{
			strcpy(anchor_check, content);
			if(strstr(content,".CH"))
			{
				anchor_arg[0] = strtok(anchor_check, ".");
				anchor_arg[1] = strtok(NULL, "CH");
				sscanf(content,"%u.CH%u", &arg_to_int[0], &arg_to_int[1]);
				sprintf(arg_check[0], "%u", arg_to_int[0]);
				sprintf(arg_check[1], *anchor_arg[1]=='0'?"%02u":"%u", arg_to_int[1]);
			}
			if(!arg_to_int[0] || !arg_to_int[1])
				goto exit;
			if(strcmp(arg_check[0], anchor_arg[0]) || strcmp(arg_check[1], anchor_arg[1]))
			{
			exit:
				fprintf(stderr, "\nANCHOR : \"%s\" of ISO_CHANNEL : \"%s\" is NOT valid !!!!\n\n", content, iso_channel);
				return EXIT_FAILURE;
			}
		}
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
}
*/
int Morfeas_OPC_UA_calc_diff_of_ISO_Channel_node(xmlNode *root_element, GSList **cur_Links)
{
	xmlNode *check_element;
	GSList *node;
	char *iso_channel;
	if(cur_Links)
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
	xmlNode *check_element;
	struct Link_entry *list_cur_Links_node_data;
	char *iso_channel_str = NULL, *dev_type_str = NULL, *anchor_ptr = NULL;

	g_slist_free_full(*cur_Links, free_Link_entry);//Free List cur_Links
	*cur_Links = NULL;
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
				memccpy(&(list_cur_Links_node_data->interface_type), dev_type_str, '\0', sizeof(list_cur_Links_node_data->interface_type));
				sscanf(anchor_ptr, "%u.CH%hhu", &(list_cur_Links_node_data->identifier), &(list_cur_Links_node_data->channel));
				*cur_Links = g_slist_append(*cur_Links, list_cur_Links_node_data);
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

int Morfeas_daemon_config_valid(xmlNode *root_element)
{

}
