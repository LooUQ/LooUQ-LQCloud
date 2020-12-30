//	Copyright (c) 2020 LooUQ Incorporated.
//	Licensed under The MIT License. See LICENSE in the root directory.

#define _DEBUG                          // enable/expand 
#include "lqc_internal.h"

// debugging output options             UNCOMMENT one of the next two lines to direct debug (PRINTF) output
#include <jlinkRtt.h>                   // output debug PRINTF macros to J-Link RTT channel
// #define SERIAL_OPT 1                    // enable serial port comm with devl host (1=force ready test)


lqCloudDevice_t g_lqCloud;

#define MIN(x, y) (((x)<(y)) ? (x):(y))

/* Local (static/non-public) functions */
static void mqttReceiver(const char *topic, const char *topicProps, const char *message);

#pragma region  Public LooUQ Clouc functions


/**
 *	\brief Creates local instance of cloud client services.
 *
 *	\param[in] faultHandler Function pointer to a local fault handler (perform local indications).
 */
void lqc_create(reconnectHndlr_func reconnectHandler, faultHndlr_func faultHandler, pwrStatus_func pwrStatFunc, battStatus_func battStatFunc, memStatus_func memStatFunc)
{
    g_lqCloud.applReconnectHandler_func = reconnectHandler;
    g_lqCloud.applFaultHandler_func = faultHandler;
    g_lqCloud.powerStatus_func = pwrStatFunc;
    g_lqCloud.batteryStatus_func = battStatFunc;
    g_lqCloud.memoryStatus_func = memStatFunc;
}


/**
 *	\brief Initialize the LTEm1 modem.
 *
 *	\param[in] ltem1_config The LTE modem gpio pin configuration.
 *  \param[in] funcLevel Determines the LTEm1 functionality to create and start.
 */
void lqc_connect(const char *hubAddr, const char *deviceId, const char *sasToken, const char *actnKey)
{
    char userId[IOTHUB_USERID_SZ];
    char recvTopic[MQTT_TOPIC_NAME_SZ];

    strncpy(g_lqCloud.iothubAddr, hubAddr, IOTHUB_URL_SZ);
    strncpy(g_lqCloud.deviceId, deviceId, LQCLOUD_DEVICEID_SZ);
    strncpy(g_lqCloud.sasToken, sasToken, IOTHUB_SASTOKEN_SZ);
    strncpy(g_lqCloud.actnKey, actnKey, LQCACTN_AKEY_SZ);
    
    snprintf(userId, IOTHUB_USERID_SZ, MQTT_IOTHUB_USERID_TMPLT, g_lqCloud.iothubAddr, g_lqCloud.deviceId);
    snprintf(recvTopic, MQTT_TOPIC_NAME_SZ, MQTT_IOTHUB_C2D_RECVTOPIC_TMPLT, g_lqCloud.deviceId);

    // recovery returns on full open\connect\subscribe complete, don't exit to fault handler until past
    uint8_t lqcConnectState = 0;
    if (mqtt_open(g_lqCloud.iothubAddr, IOTHUB_PORT, sslVersion_tls12, mqttVersion_311) == RESULT_CODE_SUCCESS)
        lqcConnectState++;
    if (lqcConnectState == 1 && mqtt_connect(g_lqCloud.deviceId, userId, g_lqCloud.sasToken, mqttSession_cleanStart) == RESULT_CODE_SUCCESS)
        lqcConnectState++;
    if (lqcConnectState == 2 && mqtt_subscribe(recvTopic, mqttQos_1, mqttReceiver) == RESULT_CODE_SUCCESS)
        lqcConnectState++;

    if (lqcConnectState < 3)
        LQC_faultHandler(lqcConnectState == 0 ? "MQTT open failed." : (lqcConnectState == 1) ? "MQTT connect failed." :  "MQTT subscribe to IoTHub C2D messages failed.");
    // faultHandler returns here after network\mqtt recovered.

    // ASSERT(mqtt_open(g_lqCloud.iothubAddr, IOTHUB_PORT, sslVersion_tls12, mqttVersion_311) == RESULT_CODE_SUCCESS, "MQTT open failed.");
    // ASSERT(mqtt_connect(g_lqCloud.deviceId, userId, g_lqCloud.sasToken, mqttSession_cleanStart) == RESULT_CODE_SUCCESS, "MQTT connect failed.");
    // ASSERT(mqtt_subscribe(recvTopic, mqttQos_1, mqttReceiver) == RESULT_CODE_SUCCESS, "MQTT subscribe to IoTHub C2D messages failed.");

    // LQCloud private function in alerts
    LQC_sendDeviceStarted(false);
}


lqcState_t lqc_getState(const char *hostName)
{
    return (lqcState_t)mqtt_status(hostName);
}


char *lqc_getDeviceId()
{
    return &g_lqCloud.deviceId;
}


char *lqc_getDeviceShortName()
{
    return &g_lqCloud.deviceShortName;
}


/**
 *	\brief Perform background processing.
 */
void lqc_doWork()
{
    ltem1_doWork();
}

#pragma endregion



#pragma region  LQ Cloud internal functions
/* --------------------------------------------------------------------------------------------- */

/**
 *	\brief LQCloud Private: send device message via MQTT.
 * 
 *  \param [in] eventType - Event type: telemetry, alert, action response.
 *  \param [in] summary - Event summary description (used for fault reporting).
 *  \param [in] topic - MQTT message topic (expected as fully formed).
 *  \param [in] body - MQTT message body (expected as fully formed).
 * 
 *  \return No return, if failure duing send LQCloud handles recovery\discard.
 */
void LQC_mqttSend(const char *eventType, const char *summary, const char *topic, const char *body)
{
    // char topic[TOPIC_SZ];
    // const char msgType[5];
    // const char msgClass[5] = (eventClass == lqcEventClass_application) ? "appl" : "lqc";

    // g_lqCloud.msgNm++;
    // switch (eventType)
    // {
    // case lqcEventType_telemetry:
    //     strncpy(msgType, "tdat", 5);
    //     // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=tdat&evC=%s&evN=%s"
    //     snprintf(topic, MQTT_TOPIC_SZ, MQTT_MSG_D2CTOPIC_TELEMETRY_TMPLT, g_lqCloud.deviceId, g_lqCloud.msgNm, msgClass, eventName);
    //     break;
    
    // case lqcEventType_alert:
    //     strncpy(msgType, "alrt", 5);
    //     // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=alrt&evC=%s&evN=%s"
    //     snprintf(topic, MQTT_TOPIC_SZ, MQTT_MSG_D2CTOPIC_ALERT_TMPLT, g_lqCloud.deviceId, g_lqCloud.msgNm, msgClass, eventName);
    //     break;
    
    // case lqcEventType_actnResp:
    //     strncpy(msgType, "aRsp", 5);
    //     // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=aRsp&aCId=%s&evC=%s&evN=%s&aRslt=%d"
    //     snprintf(topic, MQTT_TOPIC_SZ, MQTT_MSG_D2CTOPIC_ACTIONRESP_TMPLT, g_lqCloud.deviceId, g_lqCloud.msgNm, msgType, msgClass, eventName);
    //     break;
    
    // }



    socketResult_t pubResult = mqtt_publish(topic, mqttQos_1, body);
    if (pubResult != RESULT_CODE_SUCCESS)
    {
        char faultMsg[120];
        snprintf(faultMsg, 120, "Send failed (%s), summary=%s", eventType, summary);
        LQC_faultHandler(faultMsg);

        // return from faultHandler indicates communications reestablished
        pubResult = mqtt_publish(topic, mqttQos_1, body);
        if (pubResult != RESULT_CODE_SUCCESS)
        {
            LQC_faultHandler("Send FINAL failed, MESSAGE DISCARDED!");
        }
    }
}



/**
 *	\brief LQCloud Private: fault handler for LQ Cloud client code.
 * 
 *  \param [in] faultMsg - Message string describing fault.
 */
void LQC_faultHandler(const char *faultMsg)
{
    PRINTFC(dbgColor_error, "LQC-FAULT: %s\r", faultMsg);

    // assume network issue, so reset and reconnect
    ntwk_resetPdpContexts();

    if (g_lqCloud.applFaultHandler_func)                    // give application local FAULT handler an opportunity to display fault
        g_lqCloud.applFaultHandler_func(faultMsg);

    workSchedule_t faultRecovery_intvl = workSched_createPeriodic(PERIOD_FROM_SECONDS(LQC_RECOVERY_WAIT_SECONDS));
    while (true)
    {
        /* LQCloud uses reconnect\recovery handler callback rather than _Noreturn to prevent stack creep. Using a no return strategy
         * could be implemented if the call is to a new main() type function. Making this function _Noreturn requires duplication of main() logic
         * here -or- the reexecution of the real main() and its setup() functions which fully restarts the application. 
        */
        if (workSched_doNow(&faultRecovery_intvl) && LQC_cloudReconnect())
        {
            if (g_lqCloud.applReconnectHandler_func)        // give application local RECONNECT\RECOVERY handler an opportunity reset application conditions
                g_lqCloud.applReconnectHandler_func();
            break;
        }
    }
}



/**
 *	\brief LQCloud Private: reconnect to cloud following communications error.
 * 
 *  \return Status code
 */
resultCode_t LQC_cloudReconnect()
{
    char userId[IOTHUB_USERID_SZ];
    char recvTopic[MQTT_TOPIC_NAME_SZ];

    snprintf(userId, IOTHUB_USERID_SZ, MQTT_IOTHUB_USERID_TMPLT, g_lqCloud.iothubAddr, g_lqCloud.deviceId);
    snprintf(recvTopic, MQTT_TOPIC_NAME_SZ, MQTT_IOTHUB_C2D_RECVTOPIC_TMPLT, g_lqCloud.deviceId);

    if (mqtt_open(g_lqCloud.iothubAddr, IOTHUB_PORT, sslVersion_tls12, mqttVersion_311) != RESULT_CODE_SUCCESS)
    {
        PRINTFC(dbgColor_warn, "LQC-Reconnect: MQTT open failed.\n");
        return RESULT_CODE_ERROR;
    }
    if (mqtt_connect(g_lqCloud.deviceId, userId, g_lqCloud.sasToken, mqttSession_cleanStart) != RESULT_CODE_SUCCESS)
    {
        PRINTFC(dbgColor_warn, "LQC-Reconnect: MQTT connect failed.\n");
        return RESULT_CODE_ERROR;
    }
    if (mqtt_subscribe(recvTopic, mqttQos_1, mqttReceiver) != RESULT_CODE_SUCCESS)
    {
        PRINTFC(dbgColor_warn, "LQC-Reconnect: MQTT subscribe to IoTHub C2D messages failed.\n");
        return RESULT_CODE_ERROR;
    }
    // LQCloud private function in alerts
    LQC_sendDeviceStarted(true);
    return RESULT_CODE_SUCCESS;
}

#pragma endregion



#pragma region  Local static (private) functions
/* --------------------------------------------------------------------------------------------- */

/**
 *	\brief MQTT message receiver.
 * 
 *  \param [in] topic - MQTT topic for incoming message, equals one of the subscribed topics.
 *  \param [in] topicProps - MQTT topic postamble properties as HTTP query string
 *  \param [in] msgBody - Data received from the cloud.
 */
static void mqttReceiver(const char *topic, const char *topicProps, const char *msgBody)
{
    keyValueDict_t mqttProps = lqc_createDictFromQueryString(topicProps);

    #ifdef _DEBUG
    PRINTF(dbgColor_info, "\r**MQTT--MSG** @tick=%d\r", timing_millis());
    PRINTF(dbgColor_cyan, "\rt(%d): %s", strlen(topic), topic);
    PRINTF(dbgColor_cyan, "\rp(%d): %s", strlen(topicProps), topicProps);
    PRINTF(dbgColor_cyan, "\rm(%d): %s", strlen(msgBody), msgBody);
    PRINTF(dbgColor_info, "\rProps(%d)\r", mqttProps.count);
    for (size_t i = 0; i < mqttProps.count; i++)
    {
        PRINTF(dbgColor_cyan, "%s=%s\r", mqttProps.keys[i], mqttProps.values[i]);
    }
    PRINTF(0, "\r");
    #endif

    strcpy(g_lqCloud.actnMsgId, lqc_getDictValue("$.mid", mqttProps));
    strcpy(g_lqCloud.actnName, lqc_getDictValue("evN", mqttProps));
    g_lqCloud.actnResult = RESULT_CODE_NOTFOUND;
    LQC_processActionRequest(g_lqCloud.actnName, mqttProps, msgBody);
}

#pragma endregion
