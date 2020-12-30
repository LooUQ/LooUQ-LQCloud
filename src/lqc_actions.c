//	Copyright (c) 2020 LooUQ Incorporated.
//	Licensed under The MIT License. See LICENSE in the root directory.

#define _DEBUG
#include "lqc_internal.h"

// debugging output options             UNCOMMENT one of the next two lines to direct debug (PRINTF) output
#include <jlinkRtt.h>                   // output debug PRINTF macros to J-Link RTT channel
// #define SERIAL_OPT 1                    // enable serial port comm with devl host (1=force ready test)

extern lqCloudDevice_t g_lqCloud;


#pragma region Static Local Declarations
static void tryAsApplAction(const char *actnName, const char *actnKey, const char *actionMsgBody);
static void actionResponse(lqcEventClass_t evntClass, const char *evntName, uint16_t resultCode, const char *responseBody);

// built in cloud actions
static void actionInfoResponse();
static void deviceInfoResponse();
static void networkInfoResponse();
#pragma endregion


#pragma region LQ Cloud Action Public Functions
/* Public Action methods
========================================================================================================================= */
/* Action Parameter Type Codes
 *
 * s = string : string literal (const char *)
 * i = integer : non-decimal numeric value (int)
 * d = decimal : decimal number (double)
 * b = boolean : boolean true/false (int)
*/

/**
 *	\brief Provides mechanism for application code to register functions as actions available to the LooUQ Cloud. 
 *
 *	\param [in] actnName - The name the action is known as in the cloud and APIs
 *  \param [in] actnFunc - Pointer to the function to invoke in order to perform action
 *  \param [in] paramTypes - Character list, where each char indicates a parameter by type 
 */
bool lqc_regApplAction(const char *actnName, lqc_ActnFunc_t actnFunc, const char *paramList)
{
    for (size_t i = 0; i < LQCACTN_CNT; i++)
    {
        if (g_lqCloud.applActions[i].name[0] == '\0')
        {
            strcpy(g_lqCloud.applActions[i].name, actnName);
            g_lqCloud.applActions[i].func = actnFunc;
            strcpy(g_lqCloud.applActions[i].paramList, paramList);
            return true;
        }
    }
    return false;    
}


/**
 *	\brief Validates and sends the result (response) for an application action.
 *
 *	\param [in] resultCode - HTTP style status result, 200 is success, etc.
 *	\param [in] body - Char pointer to body (message) response from the appl action. Can be NULL.
 */
void lqc_sendActionResponse(uint16_t resultCode, const char *bodyJson)
{
    uint16_t bodySz = strlen(bodyJson);
    char bodyRepair[LQC_EVENT_BODY_SZ];

    if (bodySz > 0 && bodyJson[0] == '{' || bodyJson[bodySz-1] == '}')
        actionResponse(lqcEventClass_application, g_lqCloud.actnName, resultCode, bodyJson);

    else if (bodySz == 0)
        actionResponse(lqcEventClass_application, g_lqCloud.actnName, resultCode, "{}");

    else
    {
        snprintf(bodyRepair, LQC_EVENT_BODY_SZ, "{%s,\"applError\":\"jsonInvalid\"}", bodyJson);
        actionResponse(lqcEventClass_application, g_lqCloud.actnName, resultCode, "{}");
    }
}

#pragma endregion


#pragma region LQ Cloud Internal Functions LQC_
/* LQ Cloud Internal Action Subsystem Function
================================================================================================ */

/**
 *	\brief Processes incoming MQTT message as LooUQ Cloud action. 
 *
 *	\param [in] actnName - Name of action in incoming request.
 *  \param [in] actnParams - Parameters received with request.
 *	\param [in] msgBody - Message body received from the MQTT receiver process.
 */
void LQC_processActionRequest(const char *actnName, keyValueDict_t mqttProps, const char *msgBody)
{
    char *actnKey = lqc_getDictValue("aKey", mqttProps);

    if (strlen(actnKey) == 0 || strcmp(actnKey, g_lqCloud.actnKey) == 0)
    {
        char *eventClass = lqc_getDictValue("evC", mqttProps);

        if (strcmp(eventClass, "appl") == 0)
        {
            tryAsApplAction(actnName, actnKey, msgBody);
        }
        else if (strcmp(eventClass, "lqc") == 0 && strcmp(actnName, "getactn") == 0)    // getactninfo: get action info
        {
            actionInfoResponse();
        }
        else if (strcmp(eventClass, "lqc") == 0 && strcmp(actnName, "getdvc") == 0)     // getdvcinfo: get device info
        {
            deviceInfoResponse();
        }
        else if (strcmp(eventClass, "lqc") == 0 && strcmp(actnName, "getntwk") == 0)    // getntwkinfo: get network info
        {
            networkInfoResponse();
        }
        else
            lqc_sendActionResponse(RESULT_CODE_NOTFOUND, "Unable to match action.");
    }
    else
        lqc_sendActionResponse(RESULT_CODE_FORBIDDEN, "Invalid action key.");
}



#pragma endregion


#pragma region Static Local Functions

/**
 *	\brief Application custom action processor. 
 *
 *	\param [in] actionMessage - Message received from the MQTT receiver process.
 */
static void tryAsApplAction(const char *actnName, const char *actnKey, const char *actionMsgBody)
{
    PRINTF(dbgColor_dGreen, "ApplAction: %s\r", actnName);
    lqcJsonPropValue_t paramsProp = lqc_getJsonPropValue(actionMsgBody, "params");
    keyValueDict_t actnParams = lqc_createDictFromQueryString(paramsProp.value);

    for (size_t i = 0; i < LQCACTN_CNT; i++)
    {
        if (strcmp(g_lqCloud.applActions[i].name, actnName) == 0)
        {
            g_lqCloud.applActions[i].func(actnParams);
            if (strlen(g_lqCloud.actnMsgId) > 0)                     // send error, if function failed to send response and clear request msgId
                actionResponse(lqcEventClass_application, g_lqCloud.actnName, RESULT_CODE_ERROR, "Action failed. See eRslt (resultCode).");
            return;
        }
    }
}


static void getApplActions(char applActions[], uint16_t maxSz)
{
    uint16_t actnOffset = 0;

    for (size_t i = 0; i < LQCLOUD_ACTIONS_MAX; i++)
    {
        char actnBuf[80] = {0};
        //char paramBuf[60] = {0};

        if (strlen(g_lqCloud.applActions[i].name) > 0)
        {
            snprintf(actnBuf, 80, "{\"name\":\"%s\",\"params\":\"%s\"},", g_lqCloud.applActions[i].name, g_lqCloud.applActions[i].paramList);

            strncpy(applActions + actnOffset, actnBuf, maxSz);
            actnOffset += strlen(actnBuf);
        }
    }
    applActions[strlen(applActions) - 1] = ASCII_cNULL;         // drop trailing ','
}


static void actionResponse(lqcEventClass_t eventClass, const char *eventName, uint16_t resultCode, const char *responseBody)
{
    char mqttTopic[TOPIC_SZ];
    char actnClass[5];

    strncpy(actnClass, (eventClass == lqcEventClass_application) ? "appl":"lqc", 5);
    g_lqCloud.msgNm++;
    // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=aRsp&aCId=%s&evC=%s&evN=%s&aRslt=%d"
    snprintf(mqttTopic, MQTT_TOPIC_SZ, MQTT_MSG_D2CTOPIC_ACTIONRESP_TMPLT, g_lqCloud.deviceId, g_lqCloud.msgNm, g_lqCloud.actnMsgId, actnClass, eventName, resultCode);
    LQC_mqttSend("aRsp", eventName, mqttTopic, responseBody);

    g_lqCloud.actnMsgId[0] = ASCII_cNULL;
    g_lqCloud.actnResult = resultCode;
}

#pragma endregion


#pragma region LQ Cloud Built-In Action Functions

/**
 *	\brief Generate and send action response about device actions 
 */
static void actionInfoResponse()
{
    char mqttBody[LQC_EVENT_BODY_SZ] = {0};
    char applActns[400] = {0};

    getApplActions(applActns, 400);
    snprintf(mqttBody, LQC_EVENT_BODY_SZ, "{\"getactn\":{" 
                    "\"lqc\":[{\"name\":\"getactn\",\"params\":\"\"},{\"name\":\"getntwk\",\"params\":\"\"},{\"name\":\"getdvc\",\"params\":\"\"}],"
                    "\"appl\":[%s]}}", applActns);
                
    actionResponse(lqcEventClass_lqcloud, "getactn", ACTION_RESULT_SUCCESS, mqttBody);
}


/**
 *	\brief Generate and send action response about device 
 */
static void deviceInfoResponse()
{
    char topic[TOPIC_SZ] = {0};
    char propertyBag[PROPS_SZ] = {0};
    char body[LQC_EVENT_BODY_SZ] = {0};

    snprintf(body, LQC_EVENT_BODY_SZ, "{\"getdvc\": {\"dId\": \"%s\",\"codeVer\": \"LooUQ-Cloud MQTT v1.1\",\"msgVer\": \"1.0\"}}", g_lqCloud.deviceId);
    actionResponse(lqcEventClass_lqcloud, "getdvc", ACTION_RESULT_SUCCESS, body);
}


/**
 *	\brief Generate and send action response about device network 
 */
static void networkInfoResponse()
{
    char topic[TOPIC_SZ] = {0};
    char propertyBag[PROPS_SZ] = {0};
    char body[LQC_EVENT_BODY_SZ] = {0};
    int rssi = -99;

    snprintf(body, LQC_EVENT_BODY_SZ, "{\"getntwk\": {\"ntwkType\": \"%s\",\"ntwkName\": \"%s\", \"rssi\":%d}}", g_lqCloud.networkType, g_lqCloud.networkName, rssi);
    actionResponse(lqcEventClass_lqcloud, "getntwk", ACTION_RESULT_SUCCESS, body);
}

#pragma endregion