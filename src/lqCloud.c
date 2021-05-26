/******************************************************************************
 *  \file lqCloud.c
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
 * LooUQ LQCloud provides for rapid development of robust and secure IoT device
 * applications.
 *****************************************************************************/

#define _DEBUG 2                        // set to non-zero value for PRINTF debugging output, 
// debugging output options             // LTEm1c will satisfy PRINTF references with empty definition if not already resolved
#if defined(_DEBUG) && _DEBUG > 0
    asm(".global _printf_float");       // forces build to link in float support for printf
    #if _DEBUG == 1
    #define SERIAL_DBG 1                // enable serial port output using devl host platform serial, 1=wait for port
    #elif _DEBUG == 2
    #include <jlinkRtt.h>               // output debug PRINTF macros to J-Link RTT channel
    #endif
#else
#define PRINTF(c_, f_, ...) ;
#endif

#include <lqcloud.h>
#include "lqc-internal.h"

#define MIN(x, y) (((x)<(y)) ? (x):(y))
#define MAX(x, y) (((x)>(y)) ? (x):(y))
#define SEND_RETRIES 3
#define SEND_RETRYWAIT 2000


lqCloudDevice_t g_lqCloud;              /* GLOBAL LQCLOUD OBJECT */


/* Local (static/non-public) functions */
static bool s_cloudConnectionManager(bool forceConnect, bool resetConnection);
static lqcConnectState_t s_cloudConnect();
static void s_cloudDisconnect();
static void s_cloudReceiver(const char *topic, const char *topicProps, const char *message);
static void s_cloudSenderDoWork();


#pragma region  Public LooUQ Clouc functions

/**
 *	\brief Creates local instance of cloud client services.
 *
 *	\param notificationCB [in] Callback (func ptr) to application notification method used to report events.
 *  \param pwrStatCB [in] Callback to register with LQCloud to determine external power status, returns bool.
 *  \param battStatCB [in] Callback to register with LQCloud to determine battery status, returns enum.
 *  \param memoryStatCB [in] Callback to register with LQCloud to determine memory status, returns int.
 */
//void lqc_create(notificationType_func appNotificationCB, pwrStatus_func pwrStatCB, battStatus_func battStatCB, memStatus_func memStatCB)
void lqc_create(lqDiagResetCause_t resetCause,
                appNotify_func notificationCB, 
                ntwkStart_func ntwkStartCB,
                ntwkStop_func ntwkStopCB,
                pwrStatus_func pwrStatCB, 
                battStatus_func battStatCB, 
                memStatus_func memoryStatCB)
{
    g_lqCloud.connectMode = lqcConnectMode_continuous;
    strncpy(g_lqCloud.deviceName, "LQC-Device", LQC_DEVICENAME_SZ);
    g_lqCloud.diagnostics.resetCause = resetCause;

    g_lqCloud.notificationCB = notificationCB;
    g_lqCloud.networkStartCB = ntwkStartCB;
    g_lqCloud.networkStopCB = ntwkStopCB;
    g_lqCloud.powerStatusCB = pwrStatCB;
    g_lqCloud.batteryStatusCB = battStatCB;
    g_lqCloud.memoryStatusCB = memoryStatCB;

    //PRINTF(0,"MQTT QbufSz=%d\r", sizeof(lqcMqttQueuedMsg_t));                        // was 1964 (2021-02-24)
    // lqcMqttQueuedMsg_t mQ[MQTTSEND_QUEUE_SZ] = calloc(1, sizeof(lqcMqttQueuedMsg_t[MQTTSEND_QUEUE_SZ]));      // create mqtt send (recovery) queue
    // *g_lqCloud.mqttQueued = mQ;
    // if (g_lqCloud.mqttQueued == NULL)
	// {
    //     LQC_appNotify(notificationType_hardFault, "MsgBuf Alloc Failed");
    //     free(g_lqCloud.mqttQueued);
	// }
}


// /**
//  *	\brief Record MCU\CPU reset type in LQCloud structure.
//  *
//  *	\param rcause [in] The byte value reported at reset/start.
//  */
// void lqc_setResetCause(lqDiagResetCause_t rcause)
// {
//     g_lqCloud.diagnostics.resetCause = rcause;
// }


/**
 *	\brief Set device shortname, override default "LQC Device".
 *
 *	\param shortName [in] The device shortname.
 */
void lqc_setDeviceName(const char *shortName)
{
    memset(g_lqCloud.deviceName, 0, LQC_DEVICENAME_SZ);
    strncpy(g_lqCloud.deviceName, shortName, LQC_DEVICENAME_SZ-1);
}


/**
 *	\brief Initialize the LTEm1 modem.
 *
 *	\param ltem1_config [in] The LTE modem gpio pin configuration.
 *  \param funcLevel [in] Determines the LTEm1 functionality to create and start.
 */
void lqc_start(const char *hubAddr, const char *deviceId, const char *sasToken, const char *actnKey)
{
    strncpy(g_lqCloud.iothubAddr, hubAddr, LQC_URL_SZ);
    strncpy(g_lqCloud.deviceId, deviceId, LQC_DEVICEID_SZ);
    strncpy(g_lqCloud.sasToken, sasToken, LQC_SASTOKEN_SZ);
    strncpy(g_lqCloud.appKey, actnKey, LQC_APPKEY_SZ);


    uint16_t retries = 0;
    char connectionMsg[80] = {0};

    while (!s_cloudConnectionManager(true, false))
    {
        PRINTF(DBGCOLOR_white, ".");
        snprintf(connectionMsg, 80, "LQC Start Retry=%d", retries);
        LQC_notifyApp(lqcNotifType_info, connectionMsg);
        lDelay(15000);
        retries++;
        if (retries > IOTHUB_CONNECT_RETRIES)
        {
            LQC_notifyApp(lqcNotifType_disconnect, "");
            retries = 0;
            lDelay(PERIOD_FROM_MINUTES(60));
        }
    }
    LQC_notifyApp(lqcNotifType_connect, "");

    // LQCloud private function in alerts
    LQC_sendDeviceStarted(g_lqCloud.diagnostics.resetCause);
}


/**
 *	\brief Sets connection mode for cloud MQTT connection. Default at LQC init is Continuous. 
 *
 *	\param connMode [in] Enum specifying the type of connection between the local device and the LQCloud.
 *  \param schedObj  [in] wrkTime schedule object initialized to the period of "reconnect" intervals.
 */
void lqc_setConnectMode(lqcConnectMode_t connMode, wrkTime_t *schedObj)
{
    g_lqCloud.connectMode = connMode;
    g_lqCloud.connectSched = schedObj;
}


/**
 *	\brief Query for mode of connectivity to the LQCloud.
 * 
 *  \return Enum representing device to cloud connection mode
 */
lqcConnectMode_t lqc_getConnectMode()
{
    return g_lqCloud.connectMode;
}


/**
 *	\brief Query for connectivity to the LQCloud.

 *  \param hostname [in] If not empty string, validate that is the host connected to.
 *  \param forceRead [in] If true, inquire with underlying MQTT transport; if false return internal property state 
 * 
 *  \return Enum representing device to cloud connection state
 */
lqcConnectState_t lqc_getConnectState(const char *hostName, bool forceRead)
{
    char subscribeTopic[LQMQ_TOPIC_SUB_MAXSZ] = {0};
    snprintf(subscribeTopic, LQMQ_TOPIC_SUB_MAXSZ, LQMQ_IOTHUB_C2D_RECVTOPIC_TMPLT, g_lqCloud.deviceId);

    // NOTE: BGx MQTT has no check for an existing subscription (only change actions of subscribe\unsubscribe)
    // Here the subscribed test must resubscribe to Azure C2D topic and test for success. Azure and the BGx don't seem to mind resubscribing to a subscribed topic/qos

    lqcConnectState_t mqttConnState = (lqcConnectState_t)mqtt_status(hostName, forceRead);                          // get underlying MQTT connection status
    g_lqCloud.connectState = (mqttConnState == mqttStatus_connected) ? g_lqCloud.connectState : mqttConnState;      // LQC status unchanged on mqtt connected unless...

    if (mqttConnState == lqcConnectState_connected && g_lqCloud.connectState == lqcConnectState_ready && forceRead) // subscription verification indicated
    {
        if (mqtt_subscribe(subscribeTopic, mqttQos_1, s_cloudReceiver) == RESULT_CODE_SUCCESS)
            g_lqCloud.connectState = lqcConnectState_ready;
    }
    return g_lqCloud.connectState;
}


char *lqc_getDeviceId()
{
    return &g_lqCloud.deviceId;
}


char *lqc_getDeviceShortName()
{

    return &g_lqCloud.deviceName;
}


/**
 *	\brief Perform background processing.
 */
void lqc_doWork()
{
    // TODO: needs to be abstracted, currently LQCloud is tied to LTEm1\LTEmC
    ltem_doWork();
    
    s_cloudSenderDoWork();       // internally calls mqttConnectionManager() to set-up/tear-down LQC connection
}

#pragma endregion



#pragma region  LQ Cloud internal functions
/* --------------------------------------------------------------------------------------------- */

/**
 *	\brief LQCloud Private: invoke user application notification handler to inform it of LQCloud event.
 *
 *  \param [in] notificationType - Category of the event.
 *  \param [in] notificationMsg - Message string describing event.
 */
void LQC_notifyApp(uint8_t notifType, const char *notifMsg)
{
    if (g_lqCloud.notificationCB)
        g_lqCloud.notificationCB(notifType, notifMsg);
}


// /**
//  *	\brief LQCloud Private: fault handler for LQ Cloud client code.
//  * 
//  *  \param [in] faultMsg - Message string describing fault.
//  */
// void LQC_faultHandler(const char *faultMsg)
// {
//     // assume network issue, so reset and reconnect
//     //ntwk_resetPdpContexts();
    
//     mqttStatus_t mqttStatus = mqtt_status("", false);

//     PRINTF(DBGCOLOR_error, "LQC-FAULT: mqttState=%d msg=%s\r", mqttStatus, faultMsg);
//     LQC_appNotify(notificationType_hardFault, faultMsg);

//     uint8_t waitForever = 1;
//     while (waitForever) {}
// }


/**
 *	\brief LQCloud Private: send device message via MQTT. Integrates with the mqttSenderDoWork() to manage FIFO queue.
 * 
 *  \param [in] topic - MQTT message topic (expected as fully formed).
 *  \param [in] body - MQTT message body (expected as fully formed).
 *  \param [in] fromQueue - Indicates the message is from FIFO queue and if send fails should not be requeued.
 * 
 *  \return true if sent or queued, false if dropped.
 */
bool LQC_mqttTrySend(const char *topic, const char *body, bool fromQueue)
{
    bool deferredSend = !fromQueue && g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt != 0;
    socketResult_t pubResult;

    if (!deferredSend)
    {
        g_lqCloud.diagnostics.publishLastAt = millis();
        pubResult = mqtt_publish(topic, mqttQos_1, body);                                   // send message
        if (pubResult != RESULT_CODE_SUCCESS)
            g_lqCloud.diagnostics.publishLastFaultType = pubResult;
        g_lqCloud.diagnostics.publishDurLast = millis() - g_lqCloud.diagnostics.publishLastAt;
        g_lqCloud.diagnostics.publishDurMax = MAX(g_lqCloud.diagnostics.publishDurMax, g_lqCloud.diagnostics.publishDurLast);
        PRINTF(DBGCOLOR_cyan, "MQTTtrySend:rc=%d,queued=%d,dur=%d,maxDur=%d\r", pubResult, fromQueue, g_lqCloud.diagnostics.publishDurLast, g_lqCloud.diagnostics.publishDurMax);
    }

    if (pubResult == RESULT_CODE_SUCCESS && fromQueue)
    {
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt = 0;
        g_lqCloud.mqttQueuedTail = ++g_lqCloud.mqttQueuedTail % LQMQ_SEND_QUEUE_SZ;
    }

    if (deferredSend ||                                                                     // in FIFO mode
        (pubResult != RESULT_CODE_SUCCESS && !fromQueue))                                   // -or- send failed and not already in queue
    {
        if (g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt != 0)                                    
        {
            PRINTF(DBGCOLOR_warn, "MQTTtrySend:ovrflw\r", pubResult);
            g_lqCloud.mqttSendOverflowCnt++;                                                // tally overflow and silently drop
            return false;
        }
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].retries = 0;
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt = lMillis();             // allow for timed backoff
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].lastTryAt = lMillis();
        strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].topic, topic, LQMQ_TOPIC_PUB_MAXSZ);
        strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].msg, body, LQMQ_MSG_MAXSZ);
        PRINTF(DBGCOLOR_warn, "MQTTtrySend:msgQueued\r");

        g_lqCloud.mqttQueuedHead = ++g_lqCloud.mqttQueuedHead % LQMQ_SEND_QUEUE_SZ;  
    }
    return true;
}

#pragma endregion



#pragma region  Local static (private) functions
/* --------------------------------------------------------------------------------------------- */

/**
 *	\brief  Background queued message sender.
 */
static void s_cloudSenderDoWork()
{
    if (g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt)                                                     // there is a message to retry
    {
        if (wrkTime_isElapsed(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].lastTryAt, LQMQ_SEND_RETRY_DELAY))      // we have waited sufficiently
        {
            g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries++;
            g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].lastTryAt = lMillis();

            bool excessRetriesResetConnection = !(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries % LQMQ_SEND_RETRY_CONNECTIONRESETRETRIES);
            bool readyToSend = s_cloudConnectionManager(false, excessRetriesResetConnection);

            if (readyToSend)
            {
                PRINTF(DBGCOLOR_dCyan, "RecoverySend retries=%d--", g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries);
                g_lqCloud.diagnostics.publishRetryMax = MAX(g_lqCloud.diagnostics.publishRetryMax, g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries);

                LQC_mqttTrySend(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].topic,
                                g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].msg,
                                true);
            }
        }
    }
}


/**
 *	\brief MQTT LQCloud message receiver.
 * 
 *  \param [in] topic - MQTT topic for incoming message, equals one of the subscribed topics.
 *  \param [in] topicProps - MQTT topic postamble properties as HTTP query string
 *  \param [in] msgBody - Data received from the cloud.
 */
static void s_cloudReceiver(const char *topic, const char *topicProps, const char *msgBody)
{
    keyValueDict_t mqttProps = lqc_createDictFromQueryString(topicProps, strlen(topicProps));

    #ifdef _DEBUG
    PRINTF(DBGCOLOR_info, "\r**MQTT--MSG** @tick=%d\r", lMillis());
    PRINTF(DBGCOLOR_cyan, "\rt(%d): %s", strlen(topic), topic);
    PRINTF(DBGCOLOR_cyan, "\rp(%d): %s", strlen(topicProps), topicProps);
    PRINTF(DBGCOLOR_cyan, "\rm(%d): %s", strlen(msgBody), msgBody);
    PRINTF(DBGCOLOR_info, "\rProps(%d)\r", mqttProps.count);
    for (size_t i = 0; i < mqttProps.count; i++)
    {
        PRINTF(DBGCOLOR_cyan, "%s=%s\r", mqttProps.keys[i], mqttProps.values[i]);
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


/**
 *	\brief  Manage MQTT connection to server. If continous mode disconnect: attempt reconnect, on-demand mode connect based on timers.
 *
 *  \param forceConnect [in] Connect without checking connection mode
 *
 *  \return True if ready to send message(s).
 */
static bool s_cloudConnectionManager(bool forceConnect, bool resetConnection)
{
    static uint32_t onConnectAt;              // if in on-demand mode: when did the connection start
    static uint32_t lastReconnectAt;          // if in reconnect condition: wait short interval between reconnection attempts

    bool performConnect = false;
    bool performDisconnect = false;
    bool isReady = false;

    //PRINTF(0, "CloudConnMgr force=%d, reset=%d\r", forceConnect, resetConnection);

    lqcConnectState_t connState = lqc_getConnectState("", forceConnect);
    performConnect = forceConnect && connState != lqcConnectState_ready;

    if (!performConnect)
    {
        if (g_lqCloud.connectMode == lqcConnectMode_onDemand) 
        {
            if (wrkTime_doNow(&g_lqCloud.connectSched && connState != lqcConnectState_connected))
                performConnect = true;

            if (wrkTime_isElapsed(onConnectAt, PERIOD_FROM_SECONDS(LQCCONN_ONDEMAND_CONNDURATION)))
                performDisconnect = true;
        }

        else if (g_lqCloud.connectMode == lqcConnectMode_continuous)
        {
            isReady = connState == lqcConnectState_ready;
            performConnect = !isReady;
        }

        else    // g_lqCloud.connectMode == lqcConnectMode_required
        {
            if (connState != lqcConnectState_ready)
            {
                performConnect = true;
                LQC_notifyApp(lqcNotifType_disconnect, "Awaiting LQC Connect");
            }
        }
    }

    /* required connection actions determined above, now change state if required
     --------------------------------------------------------------------------------- */
    if (performDisconnect || resetConnection)   // disconnect first to support reset
    {
        onConnectAt = 0;                        // stop connection duration timer (applicable to on-demand)
        mqtt_close();                           // close MQTT messaging session

        if (g_lqCloud.networkStopCB)            // put communications hardware to sleep (or turn-off)
            g_lqCloud.networkStopCB();
    }


    if (performConnect || resetConnection)
    {
        bool ntwkStarted = true;                // assume true, networkStartCB() will be authoritive if invoked
        while (true)                            // do connect(), if not successful wait briefly for conditions to change then retry: FOREVER!
        {
            if (g_lqCloud.networkStartCB)
                ntwkStarted = g_lqCloud.networkStartCB();

            if (ntwkStarted && s_cloudConnect() == lqcConnectState_ready)
                return true;

            if (forceConnect ||
                g_lqCloud.connectMode == lqcConnectMode_required)
            {
                LQC_notifyApp(lqcNotifType_disconnect, "");
                lDelay(LCQCONN_REQUIRED_RETRYINTVL);
                continue;
            }
            break;
        }
    }

    return isReady;
}


/**
 *	\brief Connect via MQTT to the LQCloud.
 * 
 *  \return resultCode (HTTPstyle) indicating success or type of failure. If failure, caller should also use lqc_getConnectState() for details.
 */
static lqcConnectState_t s_cloudConnect()
{
    char userId[IOTHUB_USERID_SZ];
    char subscribeTopic[LQMQ_TOPIC_SUB_MAXSZ];

    g_lqCloud.connectState = lqcConnectState_closed;
    snprintf(userId, IOTHUB_USERID_SZ, LQMQ_IOTHUB_USERID_TMPLT, g_lqCloud.iothubAddr, g_lqCloud.deviceId);
    snprintf(subscribeTopic, LQMQ_TOPIC_SUB_MAXSZ, LQMQ_IOTHUB_C2D_RECVTOPIC_TMPLT, g_lqCloud.deviceId);
    
    resultCode_t rslt = mqtt_open(g_lqCloud.iothubAddr, IOTHUB_PORT, sslVersion_tls12, mqttVersion_311);
    if (rslt == RESULT_CODE_SUCCESS)
        g_lqCloud.connectState = lqcConnectState_opened;
    else
    {
        switch (rslt)
        {
            // hard errors
            case RESULT_CODE_BADREQUEST:
            case RESULT_CODE_NOTFOUND:
                LQC_notifyApp(lqcNotifType_hardFault, "Invalid Settings");
                break;
            case RESULT_CODE_GONE:
                LQC_notifyApp(lqcNotifType_disconnect, "Network error");
                break;

            // soft errors
            case RESULT_CODE_TIMEOUT:
            case RESULT_CODE_CONFLICT:
                LQC_notifyApp(lqcNotifType_info, "No Connect, Retrying");
                break;
        }
    }

    if (g_lqCloud.connectState == lqcConnectState_opened)
    {
        rslt = mqtt_connect(g_lqCloud.deviceId, userId, g_lqCloud.sasToken, mqttSession_cleanStart);
        if (rslt == RESULT_CODE_SUCCESS)
            g_lqCloud.connectState = lqcConnectState_connected;
        else
        {
            switch (rslt)
            {
                // hard errors
                case RESULT_CODE_UNAVAILABLE:
                case RESULT_CODE_BADREQUEST:
                    LQC_notifyApp(lqcNotifType_hardFault, "Invalid Settings");
                    break;
                case RESULT_CODE_FORBIDDEN:
                    LQC_notifyApp(lqcNotifType_hardFault, "Not Authorized");
                    break;
                // soft errors
                case RESULT_CODE_TIMEOUT:
                    break;
            }
        }
    }

    if (g_lqCloud.connectState == lqcConnectState_connected)
    {
        rslt = mqtt_subscribe(subscribeTopic, mqttQos_1, s_cloudReceiver);
        if (rslt == RESULT_CODE_SUCCESS)
            g_lqCloud.connectState = lqcConnectState_ready;
    }

    return g_lqCloud.connectState;
}


static void s_mqttDisconnect()
{
    mqtt_close();
}


#pragma endregion
