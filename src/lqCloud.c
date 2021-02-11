//	Copyright (c) 2020 LooUQ Incorporated.
//	Licensed under The MIT License. See LICENSE in the root directory.

#define _DEBUG                          // enable PRINTF statements
#include "lqc_internal.h"

/* debugging output options             UNCOMMENT one of the next two lines to direct debug (PRINTF) output */
#include <jlinkRtt.h>                   // output debug PRINTF macros to J-Link RTT channel
// #define SERIAL_OPT 1                    // enable serial port comm with devl host (1=force ready test)

#define MIN(x, y) (((x)<(y)) ? (x):(y))
#define MAX(x, y) (((x)>(y)) ? (x):(y))
#define SEND_RETRIES 3
#define SEND_RETRYWAIT 2000


lqCloudDevice_t g_lqCloud;              /* GLOBAL LQCLOUD OBJECT */

/* Local (static/non-public) functions */
static resultCode_t mqttConnect();
static void mqttDisconnect();
static void mqttReceiver(const char *topic, const char *topicProps, const char *message);
static void mqttSenderDoWork();


#pragma region  Public LooUQ Clouc functions

/**
 *	\brief Creates local instance of cloud client services.
 *
 *	\param[in] faultHandler Function pointer to a local fault handler (perform local indications).
 */
void lqc_create(lqcAppNotification_func appNotificationFunc, pwrStatus_func pwrStatFunc, battStatus_func battStatFunc, memStatus_func memStatFunc)
{
    strncpy(g_lqCloud.deviceShortName, "LQCloud", LQC_SHORTNAME_SZ);
    g_lqCloud.appNotification_func = appNotificationFunc;
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
void lqc_setResetCause(lqcResetCause_t rcause)
{
    g_lqCloud.resetCause = rcause;
}


/**
 *	\brief Set device shortname, override default "LQC Device".
 *
 *	\param[in] shortName The device shortname.
 */
void lqc_setDeviceName(const char *shortName)
{
    memset(g_lqCloud.deviceShortName, 0, LQC_SHORTNAME_SZ);
    strncpy(g_lqCloud.deviceShortName, shortName, LQC_SHORTNAME_SZ-1);
}


/**
 *	\brief Initialize the LTEm1 modem.
 *
 *	\param[in] ltem1_config The LTE modem gpio pin configuration.
 *  \param[in] funcLevel Determines the LTEm1 functionality to create and start.
 */
resultCode_t lqc_start(const char *hubAddr, const char *deviceId, const char *sasToken, const char *actnKey)
{
    strncpy(g_lqCloud.iothubAddr, hubAddr, LQC_URL_SZ);
    strncpy(g_lqCloud.deviceId, deviceId, LQC_DEVICEID_SZ);
    strncpy(g_lqCloud.sasToken, sasToken, LQC_SASTOKEN_SZ);
    strncpy(g_lqCloud.applKey, actnKey, LQC_APPLKEY_SZ);

    uint16_t retries = 0;
    char connectionMsg[80] = {0};

    while (g_lqCloud.connectState != lqcConnectState_subscribed )
    {
        resultCode_t rc = mqttConnect();
        if (g_lqCloud.connectState == lqcConnectState_subscribed)
            break;

        if (rc == RESULT_CODE_FORBIDDEN)
            return rc;


        PRINTFC(dbgColor_white, ".");
        snprintf(connectionMsg, 80, "LQC-Start: connect attempt failed, retries=%d", retries);
        LQC_appNotify(lqcAppNotification_connectStatus, connectionMsg);
        delay(1000);
        retries++;
        if (retries > IOTHUB_CONNECT_RETRIES)
            return RESULT_CODE_UNAVAILABLE; 
    }
    
    LQC_appNotify(lqcAppNotification_connect, "");

    // LQCloud private function in alerts
    LQC_sendDeviceStarted(g_lqCloud.resetCause);
    return RESULT_CODE_SUCCESS;
}


lqcConnectMode_t lqc_getConnectedMode()
{
    return g_lqCloud.connectMode;
}


lqcConnectState_t lqc_getConnectState(const char *hostName, bool forceRead)
{
    return (forceRead) ? mqtt_status(hostName) : g_lqCloud.connectState;
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
    mqttSenderDoWork();

    if (g_lqCloud.connectMode == lqcConnectMode_onDemand && wrkTime_elapsed(g_lqCloud.sendLastAt, g_lqCloud.onDemandTimeoutMinutes))
    {
        // if ConnectionMode == onDemand, cut connection if it has be at least timeout period since last send
        // tobebuilt
    }
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
void LQC_mqttSender(const char *eventType, const char *summary, const char *topic, const char *body)
{
    static millisDuration_t maxExecDuration;
    millisDuration_t execDuration;

    // handle retries, delay between retries, setting global comm status flags, preservation of queued msg, tagging delay msg with delay duration

    g_lqCloud.sendLastAt = millis();
    socketResult_t pubResult = mqtt_publish(topic, mqttQos_1, body);                    // send message (1st attempt)
    execDuration = millis() - g_lqCloud.sendLastAt;
    maxExecDuration = MAX(maxExecDuration, execDuration);
    PRINTFC(dbgColor_cyan, "Publish maxDuration=%d, execDuration=%d\r", maxExecDuration, execDuration);

    /* INTERIM CODE TO SEE RECOVERY BEHAVIOR  !! BLOCKING WAIT !!
     ************************************************************/

    uint8_t tries = 1;
    while (pubResult != RESULT_CODE_SUCCESS && tries < MQTTSEND_RETRIES_MAX)
    {
        PRINTFC(dbgColor_warn, "MQTT publish failed: status=%d try=%d\r", pubResult, tries);
        tries++;
        delay(MQTTSEND_RETRY_WAITMILLIS);                                   // blocking wait

        if (mqtt_status("") != mqttStatus_connected)                        // check connected before attempting resend
        {
            LQC_appNotify(lqcAppNotification_disconnect, "");
            uint8_t connectState = 0;
            while (connectState != lqcConnectState_connected)
            {
                connectState = mqttConnect();
            }
            LQC_appNotify(lqcAppNotification_connect, "");
        }
        g_lqCloud.sendLastAt = millis();
        pubResult = mqtt_publish(topic, mqttQos_1, body);                    // send message retry
        execDuration = millis() - g_lqCloud.sendLastAt;
        maxExecDuration = MAX(maxExecDuration, execDuration);
        PRINTFC(dbgColor_cyan, "Publish maxDuration=%d, execDuration=%d\r", maxExecDuration, execDuration);
    }
    if (pubResult != RESULT_CODE_SUCCESS)
        LQC_faultHandler("Excessive cloud message publish failures, quiting!");

    PRINTFC(dbgColor_info, "MQTT publish: result=%d\r\r", pubResult);
    
    // // FINAL CODE, REPLACES ABOVE WHEN IMPLEMENTING mqttSenderDoWork()
    // if (pubResult != RESULT_CODE_SUCCESS)                                   // send failed
    // {
    //     // queue msg >> lqcPendingMsg_t     (enqueued time, topic, msg)
    // }
}

/**
 *	\brief LQCloud Private: invoke user application notification handler to inform it of LQCloud event.
 *
 *  \param [in] notificationType - Category of the event.
 *  \param [in] notificationMsg - Message string describing event.
 */
void LQC_appNotify(lqcAppNotification_t notificationType, const char *notificationMsg)
{
    if (g_lqCloud.appNotification_func)
        g_lqCloud.appNotification_func(notificationType, notificationMsg);
}


/**
 *	\brief LQCloud Private: fault handler for LQ Cloud client code.
 * 
 *  \param [in] faultMsg - Message string describing fault.
 */
void LQC_faultHandler(const char *faultMsg)
{
    // assume network issue, so reset and reconnect
    //ntwk_resetPdpContexts();
    
    mqttStatus_t mqttStatus = mqtt_status("");

    PRINTFC(dbgColor_error, "LQC-FAULT: mqttState=%d msg=%s\r", mqttStatus, faultMsg);
    LQC_appNotify(lqcAppNotification_hardFault, faultMsg);

    uint8_t waitForever = 1;
    while (waitForever) {}
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
    PRINTF(dbgColor_info, "\r**MQTT--MSG** @tick=%d\r", lMillis());
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

    lqc_getDictValue("$.mid", mqttProps, g_lqCloud.actnMsgId, IOTHUB_MESSAGEID_SZ);
    lqc_getDictValue("evN", mqttProps, g_lqCloud.actnName, LQCACTN_NAME_SZ);
    // strcpy(g_lqCloud.actnMsgId, lqc_getDictValue("$.mid", mqttProps));
    // strcpy(g_lqCloud.actnName, lqc_getDictValue("evN", mqttProps));
    g_lqCloud.actnResult = RESULT_CODE_NOTFOUND;
    LQC_processActionRequest(g_lqCloud.actnName, mqttProps, msgBody);
}


static resultCode_t mqttConnect()
{
    char userId[IOTHUB_USERID_SZ];
    char recvTopic[MQTT_TOPIC_NAME_SZ];

    snprintf(userId, IOTHUB_USERID_SZ, MQTT_IOTHUB_USERID_TMPLT, g_lqCloud.iothubAddr, g_lqCloud.deviceId);
    snprintf(recvTopic, MQTT_TOPIC_NAME_SZ, MQTT_IOTHUB_C2D_RECVTOPIC_TMPLT, g_lqCloud.deviceId);

    resultCode_t rc = mqtt_open(g_lqCloud.iothubAddr, IOTHUB_PORT, sslVersion_tls12, mqttVersion_311);
    if (rc == RESULT_CODE_SUCCESS)
        g_lqCloud.connectState = lqcConnectState_open;
    else if (rc >= 502)                                                 //mqtt local error 500 + error_type
        return RESULT_CODE_CONFLICT;
    else
        return rc;

    rc = mqtt_connect(g_lqCloud.deviceId, userId, g_lqCloud.sasToken, mqttSession_cleanStart);
    if (rc == RESULT_CODE_SUCCESS)
        g_lqCloud.connectState = lqcConnectState_connected;
    else if (rc == 504 || rc == 505)                                    //mqtt local error 500 + error_type
        return RESULT_CODE_FORBIDDEN;
    else
        return rc;

    rc = mqtt_subscribe(recvTopic, mqttQos_1, mqttReceiver);
    if (rc == RESULT_CODE_SUCCESS)
        g_lqCloud.connectState = lqcConnectState_subscribed;
    return rc;
}


static void mqttDisconnect()
{
}


/**
 *	\brief  Background queued message sender.
 */
static void mqttSenderDoWork()
{
    // encode queueing delay, add property to topic props bag
    // attempt send
    // if success : clear pending
    // if failure : inc tries
    return;
}




// /**
//  *	\brief LQCloud Private: reconnect to cloud following communications error.
//  * 
//  *  \return Status code
//  */
// resultCode_t cloudConnect()
// {
//     char userId[IOTHUB_USERID_SZ];
//     char recvTopic[MQTT_TOPIC_NAME_SZ];
//     resultCode_t resultCd;

//     snprintf(userId, IOTHUB_USERID_SZ, MQTT_IOTHUB_USERID_TMPLT, g_lqCloud.iothubAddr, g_lqCloud.deviceId);
//     snprintf(recvTopic, MQTT_TOPIC_NAME_SZ, MQTT_IOTHUB_C2D_RECVTOPIC_TMPLT, g_lqCloud.deviceId);

//     resultCd = mqtt_open(g_lqCloud.iothubAddr, IOTHUB_PORT, sslVersion_tls12, mqttVersion_311); 
//     if (resultCd != RESULT_CODE_SUCCESS)
//     {
//         PRINTFC(dbgColor_warn, "LQC-Recovery: Open failed (%d).\r", resultCd);
//         return resultCd;
//     }
//     resultCd = mqtt_connect(g_lqCloud.deviceId, userId, g_lqCloud.sasToken, mqttSession_cleanStart);
//     if (resultCd != RESULT_CODE_SUCCESS)
//     {
//         PRINTFC(dbgColor_warn, "LQC-Recovery: Connect failed (%d).\r", resultCd);
//         return resultCd;
//     }
//     resultCd = mqtt_subscribe(recvTopic, mqttQos_1, mqttReceiver);
//     if (resultCd != RESULT_CODE_SUCCESS)
//     {
//         PRINTFC(dbgColor_warn, "LQC-Recovery: C2D subscribe failed (%d).\r", resultCd);
//         return resultCd;
//     }

//     // LQCloud private function in alerts
//     LQC_sendDeviceStarted(lqcStartType_recover);
//     return RESULT_CODE_SUCCESS;
// }


#pragma endregion
