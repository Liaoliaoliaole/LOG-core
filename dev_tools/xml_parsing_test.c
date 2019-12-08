#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

int parseSearch (xmlDocPtr doc, xmlNodePtr cur, const xmlChar *search_Node, const xmlChar *search) {

	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
	    if (!xmlStrcmp(cur->name, search_Node))
		{
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    if(!xmlStrcmp(key, search))//printf("keyword: %s\n", key);
			{
				xmlFree(key);
				return 0;
			}
 	    }
		cur = cur->next;
	}
    return -1;
}

void print_node_elements(xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar *Node_value;
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		Node_value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if(Node_value)
		{
			printf("%s: %s\n", cur->name, Node_value);
			xmlFree(Node_value);
		}
		cur = cur->next;
	}
	return;
}

static void parseDoc(char *docname, char* search_node,char* search) {

	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);

	if (doc == NULL )
	{
		xmlParserError();
		fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}

	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if (!parseSearch (doc, cur, (const xmlChar *) search_node, (const xmlChar *) search))
		{
			print_node_elements(doc, cur);
			printf("\n");
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	return;
}

int
main(int argc, char **argv) {

	if (argc != 4) {
		printf("Usage: %s docname Search_Node search_key\n", argv[0]);
		return(0);
	}

	parseDoc (argv[1],argv[2],argv[3]);

	return (1);
}

