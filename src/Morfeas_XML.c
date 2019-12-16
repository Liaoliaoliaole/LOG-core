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

int Morfeas_OPC_UA_conf_XML_parser_val(const char *filename, xmlDocPtr doc)
{
    xmlParserCtxtPtr ctxt; /* the parser context */
    //xmlDocPtr doc; /* the resulting document tree */

    /* create a parser context */
    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL)
    {
        fprintf(stderr, "Failed to allocate parser context\n");
		return EXIT_FAILURE;
    }
    /* parse the file, activating the DTD validation option */
    doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID);
    /* check if parsing succeeded */
    if (doc == NULL)
    {
        fprintf(stderr, "Failed to parse %s\n", filename);
        return EXIT_FAILURE;
    }
    else
    {
		/* check if validation succeeded */
		if (!(ctxt->valid))
		{
        	fprintf(stderr, "Failed to validate %s\n", filename);
        	return EXIT_FAILURE;
        }
		/* free up the resulting document */
		xmlFreeDoc(doc);
    }
    /* free up the parser context */
    xmlFreeParserCtxt(ctxt);
    return EXIT_SUCCESS;
}
