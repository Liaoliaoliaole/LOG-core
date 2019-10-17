#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

void parseSearch (xmlDocPtr doc, xmlNodePtr cur, const xmlChar *search) {

	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) 
	{
	    if (!xmlStrcmp(cur->name, search)) 
		{
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("keyword: %s\n", key);
		    xmlFree(key);
 	    }
	cur = cur->next;
	}
    return;
}

static void parseDoc(char *docname, char* search) {

	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);
	
	if (doc == NULL ) {
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
		parseSearch (doc, cur, (const xmlChar *) search);	 
		cur = cur->next;
	}
	
	xmlFreeDoc(doc);
	return;
}

int
main(int argc, char **argv) {
		
	if (argc != 3) {
		printf("Usage: %s docname search_key\n", argv[0]);
		return(0);
	}

	parseDoc (argv[1],argv[2]);

	return (1);
}

