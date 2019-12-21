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
//Function that parser and validate the Nodeset configuration XML

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

int Morfeas_XML_parsing(const char *filename, xmlDocPtr *doc);
int Morfeas_opc_ua_config_valid(xmlNode *root_element);
void print_XML_node(xmlNode * a_node);
char * XML_node_get_content(xmlNode *node, const char *node_name);
