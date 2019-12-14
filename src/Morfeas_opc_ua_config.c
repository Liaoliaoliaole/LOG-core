/*
Program: Morfeas_opc_ua_config.c, OPC-UA server's configuration.
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

#include <open62541/client_config_default.h>
#include <open62541/network_tcp.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

UA_StatusCode Morfeas_OPC_UA_config(UA_ServerConfig *config)
{
    UA_StatusCode retval;

    if(!config)
		return UA_STATUSCODE_BADINVALIDARGUMENT;

    retval = UA_ServerConfig_setBasics(config);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_ServerConfig_clean(config);
        return retval;
    }

    config->applicationDescription.applicationUri = UA_STRING_ALLOC("urn:Morfeas.open62541.server.application");
    config->buildInfo.manufacturerName = UA_STRING_ALLOC("Sam-Harry-Tzavaras");
    config->buildInfo.productName = UA_STRING_ALLOC("Morfeas-OPC_UA Server (Based on Open62541)");
	config->applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", "Morfeas-OPC_UA");

    retval = UA_ServerConfig_addNetworkLayerTCP(config, 4840, 0, 0);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }

    // Allocate the SecurityPolicies
    retval = UA_ServerConfig_addSecurityPolicyNone(config, NULL);// const UA_ByteString *certificate
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }


    // Initialize the Access Control plugin
    retval = UA_AccessControl_default(config, true,
                &config->securityPolicies[config->securityPoliciesSize-1].policyUri,
                usernamePasswordsSize, usernamePasswords);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }

    /* Allocate the endpoint */
    retval = UA_ServerConfig_addEndpoint(config, UA_SECURITY_POLICY_NONE_URI,
                                         UA_MESSAGESECURITYMODE_NONE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}


