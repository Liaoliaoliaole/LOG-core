/*
File: Morfeas_XML.h, Declaration of functions for read XML files
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
#include <glib.h>
#include <gmodule.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

//*** All the function Returns: EXIT_SUCCESS on success, or EXIT_FAILURE or failure. ***//

//Function that parser and validate the Nodeset configuration XML
int Morfeas_XML_parsing(const char *filename, xmlDocPtr *doc);
int Morfeas_opc_ua_config_valid(xmlNode *root_element);
char * XML_node_get_content(xmlNode *node, const char *node_name);
//Build list diff with content the ISO_Channels that will be removed
int Morfeas_OPC_UA_calc_diff_of_ISO_Channel_node(xmlNode *root_element, GSList **cur_ISOChannels);
//Clean all elements of List "cur_ISOChannels" with Re-Build it with data from xmlDoc doc
int XML_doc_to_List_ISO_Channels(xmlNode *root_element, GSList **cur_ISOChannels);
//Deconstructor for Data of Lists with data type "struct ISO_Channel_name"
void free_ISO_Channel_name(gpointer data);
//Debugging function Print node from List with data type "struct ISO_Channel_name"
//void print_List (gpointer data, gpointer user_data);
