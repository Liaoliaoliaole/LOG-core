#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>


UA_Float i;


static void updateCurrentValue(UA_Server *server) 
{
	i=0;
	UA_Variant value;
    UA_Variant_setScalar(&value, &i, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "A float");
    UA_Server_writeValue(server, currentNodeId, value);
}

/*
static void beforeRead(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeid, void *nodeContext,
               const UA_NumericRange *range, const UA_DataValue *data) 
{
    i++;
	UA_Variant value;
	UA_Variant_setScalar(&value, &i, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "An_Integer");
    UA_Server_writeValue(server, currentNodeId, value);
}

static void
afterWrite(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext,
               const UA_NumericRange *range, const UA_DataValue *data) 
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "The variable was updated");
}

static void
addValueCallbackToCurrentTimeVariable(UA_Server *server) {
    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "An_Integer");
    UA_ValueCallback callback ;
    callback.onRead = beforeRead;
    callback.onWrite = afterWrite;
    UA_Server_setVariableNode_valueCallback(server, currentNodeId, callback);
}

static void addInteger(UA_Server *server) 
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Integer_value_test");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&attr.value, &i, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "An_Integer");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "An_Integer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_Server_addVariableNode(server, currentNodeId, parentNodeId,
                              parentReferenceNodeId, currentName,
                              variableTypeNodeId, attr, NULL, NULL);

    updateCurrentValue(server);
}
*/

static UA_StatusCode readCurrentInteger(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) 
{
	//i = i + .001;
    UA_Variant_setScalarCopy(&dataValue->value, &i,&UA_TYPES[UA_TYPES_FLOAT]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeCurrentInteger(UA_Server *server,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *data)
{
    //UA_Variant_setScalarCopy(&dataValue->value, &dataValue->value,&UA_TYPES[UA_TYPES_INT32]);
	//i=data->value;
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"THE VALUE HAS CHANGED");
	
    return UA_STATUSCODE_GOOD;
}


static void addDataSourceVariable(UA_Server *server) 
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Var_float_value_test");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "a float");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "a float");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
	UA_Variant_setScalar(&attr.value, 0,&UA_TYPES[UA_TYPES_INT32]);
    UA_DataSource timeDataSource;
    timeDataSource.read = readCurrentInteger;
    timeDataSource.write = writeCurrentInteger;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        timeDataSource, NULL, NULL);
	updateCurrentValue(server);
}

static void addVariable(UA_Server *server) 
{
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","An_Integer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","An_Integer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "An_Integer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "An_Integer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static volatile UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
	
	//addInteger(server);
	//addValueCallbackToCurrentTimeVariable(server);
	addVariable(server);
	addDataSourceVariable(server);
    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}