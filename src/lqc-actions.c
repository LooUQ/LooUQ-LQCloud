/******************************************************************************
 *  \file lqc-alerts.c
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2020, 2021 LooUQ Incorporated.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 * LooUQ LQCloud Client Device Actions (command) Services
 *****************************************************************************/

#define _DEBUG 2                        // set to non-zero value for PRINTF debugging output, 
// debugging output options             // LTEm1c will satisfy PRINTF references with empty definition if not already resolved
#if defined(_DEBUG)
    asm(".global _printf_float");       // forces build to link in float support for printf
    #if _DEBUG == 2
    #include <jlinkRtt.h>               // output debug PRINTF macros to J-Link RTT channel
    #define PRINTF(c_,f_,__VA_ARGS__...) do { rtt_printf(c_, (f_), ## __VA_ARGS__); } while(0)
    #else
    #define SERIAL_DBG _DEBUG           // enable serial port output using devl host platform serial, _DEBUG 0=start immediately, 1=wait for port
    #endif
#else
#define PRINTF(c_, f_, ...) ;
#endif


#include "lqc-internal.h"
#include "lqc-azure.h"

extern lqCloudDevice_t g_lqCloud;


#pragma region Static Local Declarations
static void S_tryAsApplAction(const char *actnName, const char *actnKey, const char *actionMsgBody);
static void S_actionResponse(lqcEventClass_t evntClass, const char *evntName, uint16_t resultCode, const char *responseBody);
static void S_metricsInfoResponse(keyValueDict_t params);


// built in cloud actions
static void actionInfoResponse();
static void deviceInfoResponse();
static void networkInfoResponse();
static void diagInfoResponse(keyValueDict_t params);
static void setDeviceNameResponse(keyValueDict_t params);
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
    ASSERT(strlen(actnName) < LQC__action_nameSz, srcfile_actions_c);
    ASSERT(strlen(paramList) < LQC__action_paramsListSz, srcfile_actions_c);

    for (size_t i = 0; i < LQC__actionCnt; i++)
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
void lqc_sendActionResponse(uint16_t resultCd, const char *bodyJson)
{
    uint16_t bodySz = strlen(bodyJson);

    if (bodySz == 0)                                                                        // empty, send empty JSON object
        S_actionResponse(lqcEventClass_application, g_lqCloud.actnName, resultCd, "{}");
    else
        S_actionResponse(lqcEventClass_application, g_lqCloud.actnName, resultCd, bodyJson);
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
    char appKey[lqc__identity_organizationKeySz] = {0};
    char eventClass[8] = {0};

    lq_getQryStrDictionaryValue("appk", mqttProps, appKey, sizeof(appKey));

    if (strlen(g_lqCloud.orgKey) == 0 || strcmp(appKey, g_lqCloud.orgKey) == 0)
    {
        lq_getQryStrDictionaryValue("evC", mqttProps, eventClass, 8);

        if (strcmp(eventClass, "app") == 0)
        {
            S_tryAsApplAction(actnName, appKey, msgBody);
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
        else if (strcmp(eventClass, "lqc") == 0 && strcmp(actnName, "setdname") == 0)    // setdname: set (or get) device name
        {
            lqJsonPropValue_t paramsProp = lq_getJsonPropValue(msgBody, "params");
            keyValueDict_t actnParams = lq_createQryStrDictionary(paramsProp.value, paramsProp.len);
            setDeviceNameResponse(actnParams);
        }
        else if (strcmp(eventClass, "lqc") == 0 && strcmp(actnName, "getdiag") == 0)    // getdiaginfo: get cloud diagnostics info
        {
            lqJsonPropValue_t paramsProp = lq_getJsonPropValue(msgBody, "params");
            keyValueDict_t actnParams = lq_createQryStrDictionary(paramsProp.value, paramsProp.len);
            S_metricsInfoResponse(actnParams);
        }
        else
            lqc_sendActionResponse(resultCode__notFound, "Unable to match action.");
    }
    else
        lqc_sendActionResponse(resultCode__forbidden, "Invalid action key.");
}



#pragma endregion


#pragma region Static Local Functions

/**
 *	\brief Application custom action processor. 
 *
 *	\param [in] actionMessage - Message received from the MQTT receiver process.
 */
static void S_tryAsApplAction(const char *actnName, const char *actnKey, const char *actionMsgBody)
{
    PRINTF(dbgColor__dGreen, "ApplAction: %s\r", actnName);
    lqJsonPropValue_t paramsProp = lq_getJsonPropValue(actionMsgBody, "params");
    keyValueDict_t actnParams = lq_createQryStrDictionary(paramsProp.value, paramsProp.len);

    for (size_t i = 0; i < LQC__actionCnt; i++)
    {
        if (strcmp(g_lqCloud.applActions[i].name, actnName) == 0)
        {
            g_lqCloud.applActions[i].func(actnParams);
            if (strlen(g_lqCloud.actnMsgId) > 0)                     // send error, if function failed to send response and clear request msgId
                S_actionResponse(lqcEventClass_application, g_lqCloud.actnName, resultCode__internalError, "Action failed. See eRslt (resultCode).");
            return;
        }
    }
}


static void S_getApplActions(char applActions[], uint16_t maxSz)
{
    uint16_t actnOffset = 0;

    for (size_t i = 0; i < LQC__actionCnt; i++)
    {
        char actnBuf[80] = {0};
        //char paramBuf[60] = {0};

        if (strlen(g_lqCloud.applActions[i].name) > 0)
        {
            snprintf(actnBuf, sizeof(actnBuf), "{\"n\":\"%s\",\"p\":\"%s\"},", g_lqCloud.applActions[i].name, g_lqCloud.applActions[i].paramList);

            strncpy(applActions + actnOffset, actnBuf, maxSz - actnOffset);
            actnOffset += strlen(actnBuf);
        }
    }
    applActions[strlen(applActions) - 1] = '\0';         // drop trailing '&'
}


static void S_actionResponse(lqcEventClass_t eventClass, const char *eventName, uint16_t resultCode, const char *responseBody)
{
    char mqttTopic[LQMQ_TOPIC_PUB_MAXSZ];
    char actnClass[LQC_EVNTCLASS_SZ];

    PRINTF(0, "ActnRespBodySz=%d\r", strlen(responseBody));

    strncpy(actnClass, (eventClass == lqcEventClass_application) ? "appl":"lqc", 5);
    uint16_t msgId = mqtt_getMsgId(g_lqCloud.mqttCtrl);
    // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=aRsp&aCId=%s&evC=%s&evN=%s&aRslt=%d"
    snprintf(mqttTopic, LQMQ_TOPIC_PUB_MAXSZ, IotHubTemplate_D2C_topicActionResponse, g_lqCloud.deviceId, msgId, g_lqCloud.actnMsgId, actnClass, eventName, resultCode);
    LQC_mqttTrySend(mqttTopic, responseBody, false);

    g_lqCloud.actnMsgId[0] = '\0';
    g_lqCloud.actnResult = resultCode;
}

#pragma endregion


#pragma region LQ Cloud Built-In Action Functions

/**
 *	\brief Generate and send action response about device actions 
 */
static void actionInfoResponse()
{
    char appActns[LQC__actionCnt * LQCACTN_BUF_SZ] = {0};
    //char mqttBody[(LQCACTN_CNT * LQCACTN_BUF_SZ) + LQCACTN_CACTIONS_BODY_SZ] = {0};
    char mqttBody[LQMQ_MSG_MAXSZ] = {0};

    S_getApplActions(appActns, LQC__actionCnt * LQCACTN_BUF_SZ);
    snprintf(mqttBody, (LQC__actionCnt * LQCACTN_BUF_SZ) + LQCACTN_LQCACTIONS_BODY_SZ, "{\"getactn\":"
                    "{\"lqc\":["
                    "{\"n\":\"getactn\",\"p\":\"\"},"
                    "{\"n\":\"getntwk\",\"p\":\"\"},"
                    "{\"n\":\"getdvc\",\"p\":\"\"},"
                    "{\"n\":\"getdiag\",\"p\":\"reset=bool\"},"
                    "{\"n\":\"setdname\",\"p\":\"name=text\"}"
                    "],"
                    "\"app\":[%s]}}", appActns);
                
    S_actionResponse(lqcEventClass_lqcloud, "getactn", resultCode__success, mqttBody);
}


/**
 *	\brief Generate and send action response about device 
 */
static void deviceInfoResponse()
{
    char topic[LQMQ_TOPIC_PUB_MAXSZ] = {0};
    char body[LQMQ_MSG_MAXSZ] = {0};

    snprintf(body, 
             sizeof(body), 
             "{\"getdvc\": {\"dId\": \"%s\",\"codeVer\": \"LooUQ-Cloud MQTT v1.1\",\"msgVer\": \"1.0\"}}", 
             g_lqCloud.deviceId);
    S_actionResponse(lqcEventClass_lqcloud, "getdvc", resultCode__success, body);
}


/**
 *	\brief Generate and send action response about device network 
 */
static void networkInfoResponse()
{
    char topic[LQMQ_TOPIC_PUB_MAXSZ] = {0};
    char body[LQMQ_MSG_MAXSZ] = {0};
    int rssi = -99;

    if (g_lqCloud.ntwkSignalCB)
        rssi = g_lqCloud.ntwkSignalCB();
    
    snprintf(body,
             sizeof(body),
             "{\"getntwk\": {\"ntwkType\": \"%s\",\"ntwkName\": \"%s\", \"rssi\":%d}}", 
             g_lqCloud.networkType, 
             g_lqCloud.networkName, 
             rssi);
    S_actionResponse(lqcEventClass_lqcloud, "getntwk", resultCode__success, body);
}


/**
 *	\brief Gather LQCloud diagnostic struct members and notify user 
 */
static void S_metricsInfoResponse(keyValueDict_t params)
{
    #define KVALUESZ 8
    char kValue[KVALUESZ] = {0};
    char topic[LQMQ_TOPIC_PUB_MAXSZ] = {0};
    char body[LQMQ_MSG_MAXSZ] = {0};

    bool resetDiags = false;
    lq_getQryStrDictionaryValue("reset", params, kValue, KVALUESZ);
    PRINTF(dbgColor__cyan, "resetDiags: %s\r", kValue);
    resetDiags = atoi(kValue);

    LQC_composeMetricsReport(body, sizeof(body));
    S_actionResponse(lqcEventClass_lqcloud, "getdiag", resultCode__success, body);

    if (resetDiags)
    {
        LQC_clearMetrics(lqcMetricsType_metrics);
        LQC_clearMetrics(lqcMetricsType_diagnostics);
    }
}


/**
 *	\brief Gather LQCloud diagnostic struct members and notify user 
 */
static void setDeviceNameResponse(keyValueDict_t params)
{
    #define KVALUESZ 8
    char kValue[KVALUESZ] = {0};
    char topic[LQMQ_TOPIC_PUB_MAXSZ] = {0};
    char body[LQMQ_MSG_MAXSZ] = {0};

    lq_getQryStrDictionaryValue("name", params, kValue, KVALUESZ);
    PRINTF(dbgColor__cyan, "dvc name: %s\r", kValue);

    uint8_t kValLen = strlen(kValue);

    if (kValLen != 0 && (kValLen < 3 || kValLen >= lqc__identity_deviceLabelSz))
    {
        S_actionResponse(lqcEventClass_lqcloud, "setdname", resultCode__badRequest, "Invalid dname param, length must be 3 to 12 chars");
        return;
    }

    if (kValLen > 0)
        strncpy(g_lqCloud.deviceLabel, kValue, lqc__identity_deviceLabelSz);

    S_actionResponse(lqcEventClass_lqcloud, "setdname", resultCode__success, g_lqCloud.deviceLabel);
    PRINTF(dbgColor__info, "Device Label: %s\r", g_lqCloud.deviceLabel);
}

#pragma endregion
