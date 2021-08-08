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
#include <lq-SAMDcore.h>

#define MIN(x, y) (((x)<(y)) ? (x):(y))
#define MAX(x, y) (((x)>(y)) ? (x):(y))
#define SEND_RETRIES 3
#define SEND_RETRYWAIT 2000


lqCloudDevice_t g_lqCloud;              /* GLOBAL LQCLOUD OBJECT */


/* Local (static/non-public) functions */
static bool S_connectToCloudMqttionManager(bool forceConnect, bool resetConnection);
static lqcConnectState_t S_connectToCloudMqtt();
static void S_cloudDisconnect();
static void S_cloudReceiver(dataContext_t dataCntxt, uint16_t msgId, const char *topic, char *topicProps, char *message, uint16_t messageSz);
static void S_cloudSenderDoWork();


#pragma region  Public LooUQ Clouc functions

/**
 *	\brief Creates local instance of cloud client services.
 *
 *  \param organizationKey [in] 12-char alpha-numeric key used to validate configuration and binary resources.
 *	\param notificationCB [in] Callback (func ptr) to application notification method used to report events.
 *  \param ntwkStartCB [in] Callback (func ptr) to application to start/reset network services.
 *  \param ntwkStopCB [in] Callback (func ptr) to application to stop network services.
 *  \param pwrStatCB [in] Callback to register with LQCloud to determine external power status, returns bool.
 *  \param battStatCB [in] Callback to register with LQCloud to determine battery status, returns enum.
 *  \param memoryStatCB [in] Callback to register with LQCloud to determine memory status, returns int.
 */
//void lqc_create(notificationType_func appNotificationCB, pwrStatus_func pwrStatCB, battStatus_func battStatCB, memStatus_func memStatCB)
void lqc_create(const char *organizationKey,
                appNotify_func notificationCB, 
                ntwkStart_func ntwkStartCB,
                ntwkStop_func ntwkStopCB,
                pwrStatus_func pwrStatCB, 
                battStatus_func battStatCB, 
                memStatus_func memoryStatCB)
{
    g_lqCloud.connectMode = lqcConnectMode_continuous;
    strncpy(g_lqCloud.deviceLabel, "LQC-Device", lqc__identity_deviceLabelSz-1);
    strncpy(g_lqCloud.orgKey, organizationKey, lqc__identity_organizationKeySz-1);

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
    memset(g_lqCloud.deviceLabel, 0, lqc__identity_deviceLabelSz);
    strncpy(g_lqCloud.deviceLabel, shortName, lqc__identity_deviceLabelSz-1);
}


/**
 *	\brief Initialize the LTEm1 modem.
 *
 *	\param ltem1_config [in] The LTE modem gpio pin configuration.
 *  \param funcLevel [in] Determines the LTEm1 functionality to create and start.
 */
// void lqc_start(const char *hubAddr, const char *deviceId, const char *sasToken)
void lqc_start(mqttCtrl_t *mqttCtrl, const char *tokenSas)
{
    g_lqCloud.mqttCtrl = mqttCtrl;
    g_lqCloud.mqttCtrl->dataRecvCB = S_cloudReceiver;                               // override any local receiver, LQCloud takes that for device actions

    lqcDeviceConfig_t deviceCnfg = lqc_decomposeTokenSas(tokenSas);
    strncpy(g_lqCloud.hostUri, deviceCnfg.hostUri, lqc__identity_hostUriSz);
    strncpy(g_lqCloud.deviceId, deviceCnfg.deviceId, lqc__identity_deviceIdSz);
    strncpy(g_lqCloud.tokenSigExpiry, deviceCnfg.tokenSigExpiry, lqc__identity_tokenSigExpirySz);

    uint16_t retries = 0;
    char connectionMsg[80] = {0};

    while (!S_connectToCloudMqttionManager(true, false))
    {
        PRINTF(dbgColor__white, ".");
        snprintf(connectionMsg, 80, "LQC Start Retry=%d", retries);
        LQC_notifyApp(lqNotifType__lqcloud_connect, connectionMsg);
        pDelay(15000);
        retries++;
        if (retries > LQC__connectionRetryCnt)
        {
            LQC_notifyApp(lqNotifType__lqcloud_disconnect, "");
            retries = 0;
            pDelay(PERIOD_FROM_MINUTES(60));
        }
    }
    LQC_notifyApp(lqNotifType__lqcloud_connect, "");
    LQC_sendDeviceStarted(lqSAMD_getResetCause());
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
    char subscribeTopic[mqtt__topic_nameSz] = {0};
    snprintf(subscribeTopic, sizeof(subscribeTopic), IoTHubTemplate_C2D_recvTopic, g_lqCloud.deviceId);

    // NOTE: BGx MQTT has no check for an existing subscription (only change actions of subscribe\unsubscribe)
    // Here the subscribed test must resubscribe to Azure C2D topic and test for success. Azure and the BGx don't seem to mind resubscribing to a subscribed topic/qos

    lqcConnectState_t mqttConnState = (lqcConnectState_t)mqtt_status(g_lqCloud.mqttCtrl, hostName, forceRead);                          // get underlying MQTT connection status
    g_lqCloud.connectState = (mqttConnState == mqttStatus_connected) ? g_lqCloud.connectState : mqttConnState;      // LQC status unchanged on mqtt connected unless...

    if (mqttConnState == lqcConnectState_connected && g_lqCloud.connectState == lqcConnectState_ready && forceRead) // subscription verification indicated
    {
        if (mqtt_subscribe(g_lqCloud.mqttCtrl, subscribeTopic, mqttQos_1) == resultCode__success)
            g_lqCloud.connectState = lqcConnectState_ready;
    }
    return g_lqCloud.connectState;
}


char *lqc_getDeviceId()
{
    return &g_lqCloud.deviceId;
}


char *lqc_getDeviceLabel()
{

    return &g_lqCloud.deviceLabel;
}


/**
 *	\brief Perform background processing.
 */
void lqc_doWork()
{
    // TODO: needs to be abstracted, currently LQCloud is tied to LTEm1\LTEmC
    ltem_doWork();
    
    S_cloudSenderDoWork();       // internally calls mqttConnectionManager() to set-up/tear-down LQC connection
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

//     PRINTF(dbgColor__error, "LQC-FAULT: mqttState=%d msg=%s\r", mqttStatus, faultMsg);
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
    resultCode_t pubResult;

    if (!deferredSend)
    {
        uint32_t pubTimerStart = millis();
        pubResult = mqtt_publish(g_lqCloud.mqttCtrl, topic, mqttQos_1, body);                                   // send message
        if (pubResult != resultCode__success)
        {
            g_lqCloud.perfMetrics.publishLastFailResult = pubResult;
        }
        g_lqCloud.perfMetrics.publishLastDuration = millis() - pubTimerStart;
        g_lqCloud.perfMetrics.publishMaxDuration = MAX(g_lqCloud.perfMetrics.publishMaxDuration, g_lqCloud.perfMetrics.publishLastDuration);
        PRINTF(dbgColor__cyan, "MQTTtrySend:rc=%d,queued=%d,dur=%d,maxDur=%d\r", 
                                pubResult, 
                                fromQueue, 
                                g_lqCloud.perfMetrics.publishLastDuration, 
                                g_lqCloud.perfMetrics.publishMaxDuration);
    }

    if (pubResult == resultCode__success && fromQueue)
    {
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt = 0;
        g_lqCloud.mqttQueuedTail = ++g_lqCloud.mqttQueuedTail % LQMQ_SEND_QUEUE_SZ;
    }

    if (deferredSend ||                                                                     // in FIFO mode
        (pubResult != resultCode__success && !fromQueue))                                   // -or- send failed and not already in queue
    {
        if (g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt != 0)                                    
        {
            PRINTF(dbgColor__warn, "MQTTtrySend:ovrflw\r", pubResult);
            g_lqCloud.mqttSendOverflowCnt++;                                                // tally overflow and silently drop
            return false;
        }
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].retries = 0;
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt = pMillis();             // allow for timed backoff
        g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].lastTryAt = pMillis();
        strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].topic, topic, LQMQ_TOPIC_PUB_MAXSZ);
        strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].msg, body, LQMQ_MSG_MAXSZ);
        PRINTF(dbgColor__warn, "MQTTtrySend:msgQueued\r");

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
static void S_cloudSenderDoWork()
{
    if (g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt)                                                     // there is a message to retry
    {
        if (wrkTime_isElapsed(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].lastTryAt, LQMQ_SEND_RETRY_DELAY))      // we have waited sufficiently
        {
            g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries++;
            g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].lastTryAt = pMillis();

            bool excessRetriesResetConnection = !(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries % LQMQ_SEND_RETRY_CONNECTIONRESETRETRIES);
            bool readyToSend = S_connectToCloudMqttionManager(false, excessRetriesResetConnection);

            if (readyToSend)
            {
                PRINTF(dbgColor__dCyan, "RecoverySend retries=%d--\r", g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries);
                g_lqCloud.perfMetrics.publishMaxRetries = MAX(g_lqCloud.perfMetrics.publishMaxRetries, g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries);

                LQC_mqttTrySend(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].topic,
                                g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].msg,
                                true);
            }
        }
    }
}

//typedef void (*mqttRecvFunc_t)(dataContext_t dataCntxt, uint16_t msgId, const char *topic, char *topicProps, char *message, uint16_t messageSz);


/**
 *	\brief MQTT LQCloud message receiver.
 * 
 *  \param [in] topic - MQTT topic for incoming message, equals one of the subscribed topics.
 *  \param [in] topicProps - MQTT topic postamble properties as HTTP query string
 *  \param [in] msgBody - Data received from the cloud.
 */
static void S_cloudReceiver(dataContext_t dataCntxt, uint16_t msgId, const char *topic, char *topicProps, char *message, uint16_t messageSz)
{
    keyValueDict_t mqttProps = lq_createQryStrDictionary(topicProps, strlen(topicProps));

    #ifdef _DEBUG
    PRINTF(dbgColor__info, "\r**MQTT--MSG** @tick=%d\r", pMillis());
    PRINTF(dbgColor__cyan, "\rt(%d): %s", strlen(topic), topic);
    PRINTF(dbgColor__cyan, "\rp(%d): %s", strlen(topicProps), topicProps);
    PRINTF(dbgColor__cyan, "\rm(%d): %s", strlen(message), message);
    PRINTF(dbgColor__info, "\rProps(%d)\r", mqttProps.count);
    for (size_t i = 0; i < mqttProps.count; i++)
    {
        PRINTF(dbgColor__cyan, "%s=%s\r", mqttProps.keys[i], mqttProps.values[i]);
    }
    PRINTF(0, "\r");
    #endif

    lq_getQryStrDictionaryValue("$.mid", mqttProps, g_lqCloud.actnMsgId, LQC__messageIdSz);
    lq_getQryStrDictionaryValue("evN", mqttProps, g_lqCloud.actnName, LQC__action_nameSz);
    // strcpy(g_lqCloud.actnMsgId, lqc_getDictValue("$.mid", mqttProps));
    // strcpy(g_lqCloud.actnName, lqc_getDictValue("evN", mqttProps));
    g_lqCloud.actnResult = resultCode__notFound;
    LQC_processActionRequest(g_lqCloud.actnName, mqttProps, message);
}


/**
 *	\brief  Manage MQTT connection to server. If continous mode disconnect: attempt reconnect, on-demand mode connect based on timers.
 *
 *  \param forceConnect [in] Connect without checking connection mode
 *
 *  \return True if ready to send message(s).
 */
static bool S_connectToCloudMqttionManager(bool forceConnect, bool resetConnection)
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

            if (wrkTime_isElapsed(onConnectAt, PERIOD_FROM_SECONDS(lqc__connection_onDemand_connDurationSecs)))
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
                LQC_notifyApp(lqNotifType__lqcloud_disconnect, "Awaiting LQC Connect");
            }
        }
    }

    /* required connection actions determined above, now change state if required
     --------------------------------------------------------------------------------- */
    if (performDisconnect || resetConnection)   // disconnect first to support reset
    {
        onConnectAt = 0;                        // stop connection duration timer (applicable to on-demand)
        mqtt_close(g_lqCloud.mqttCtrl);                           // close MQTT messaging session

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

            if (ntwkStarted && S_connectToCloudMqtt() == lqcConnectState_ready)
                return true;

            if (forceConnect ||
                g_lqCloud.connectMode == lqcConnectMode_required)
            {
                LQC_notifyApp(lqNotifType__lqcloud_disconnect, "");
                pDelay(lqc__connection_required_retryIntrvlSecs);
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
static lqcConnectState_t S_connectToCloudMqtt()
{

    g_lqCloud.connectState = lqcConnectState_closed;
    
    resultCode_t rslt = mqtt_open(g_lqCloud.mqttCtrl, g_lqCloud.hostUri, lqc__connection_hostPort);
    if (rslt == resultCode__success)
        g_lqCloud.connectState = lqcConnectState_opened;
    else
    {
        switch (rslt)
        {
            // hard errors
            case resultCode__badRequest:
            case resultCode__notFound:
                LQC_notifyApp(lqNotifType__hardFault, "Invalid Settings");
                break;
            case resultCode__gone:
                LQC_notifyApp(lqNotifType__lqcloud_disconnect, "Network error");
                break;

            // soft errors
            case resultCode__timeout:
            case resultCode__conflict:
                LQC_notifyApp(lqNotifType__info, "No Connect, Retrying");
                break;
        }
    }

    if (g_lqCloud.connectState == lqcConnectState_opened)
    {
        char userId[lqc__identity_userIdSz];
        char tokenSAS[lqc__identity_tokenSasSz];

        snprintf(userId, sizeof(userId), IotHubTemplate_sas_userId, g_lqCloud.hostUri, g_lqCloud.deviceId);
        lqc_composeTokenSas(tokenSAS, sizeof(tokenSAS), g_lqCloud.hostUri, g_lqCloud.deviceId, g_lqCloud.tokenSigExpiry);

        rslt = mqtt_connect(g_lqCloud.mqttCtrl, g_lqCloud.deviceId, userId, tokenSAS, mqttSession_cleanStart);
        if (rslt == resultCode__success)
            g_lqCloud.connectState = lqcConnectState_connected;
        else
        {
            switch (rslt)
            {
                // hard errors
                case resultCode__unavailable:
                case resultCode__badRequest:
                    LQC_notifyApp(lqNotifType__hardFault, "Invalid Settings");
                    break;
                case resultCode__forbidden:
                    LQC_notifyApp(lqNotifType__hardFault, "Not Authorized");
                    break;
                // soft errors
                case resultCode__timeout:
                    break;
            }
        }
    }

    if (g_lqCloud.connectState == lqcConnectState_connected)
    {
        char subscribeTopic[mqtt__topic_nameSz];
        snprintf(subscribeTopic, sizeof(subscribeTopic), IoTHubTemplate_C2D_recvTopic, g_lqCloud.deviceId);

        rslt = mqtt_subscribe(g_lqCloud.mqttCtrl, subscribeTopic, mqttQos_1);
        if (rslt == resultCode__success)
            g_lqCloud.connectState = lqcConnectState_ready;
    }

    return g_lqCloud.connectState;
}


static void s_mqttDisconnect()
{
    mqtt_close(g_lqCloud.mqttCtrl);
}


#pragma endregion
