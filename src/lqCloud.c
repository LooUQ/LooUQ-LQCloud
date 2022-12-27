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

#define SRCFILE LQPRODUCT_LQC ## "LQC"

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

#include <lq-types.h>

#define SRCFILE "LQC"                           // create SRCFILE (3 char) MACRO for lq-diagnostics ASSERT
#include "lqc-internal.h"
#include "lqc-azure.h"
#include <lq-SAMDutil.h>

#define MIN(x, y) (((x)<(y)) ? (x):(y))
#define MAX(x, y) (((x)>(y)) ? (x):(y))
#define SEND_RETRIES 3
#define SEND_RETRYWAIT 2000

/* ------------------------------------------------------------------------------------------------
 * GLOBAL LQCloud Client Object (one LQCloud supported)
 * --------------------------------------------------------------------------------------------- */
lqCloudDevice_t g_lqCloud;


/* Local (static/non-public) functions */
//static bool S_ntwkConnectionManager(bool reqstConn, bool resetConn);
//static lqcConnectState_t S__connectToCloudMqtt();
//static void S__cloudDisconnect();

//static void S__cloudReceiver(dataCntxt_t dataCntxt, uint16_t msgId, const char *topic, char *topicProps, char *message, uint16_t messageSz);
static void S__cloudReceiver(uint16_t msgId, const char *topic, char *topicProps, char *message, uint16_t messageSz);

//static inline void S__ChangeLQCConnectState(uint8_t newState);


#pragma region  Public LooUQ Cloud functions

/**
 *	@brief Creates local instance of cloud client services.
 *  @param [in] connectMode MQTT connection type (continuous, on-demand, etc.)
 *  @param [in] deviceLabel provides local identification and indications in device-2-cloud messaging  device
 *  @param [in] deviceKey 12-char alpha-numeric key used to validate configuration and binary resources
 *	@param [in] appEventCB callback (function ptr) to app notification and request function
 *  @param [in] yieldCB callback to relinquish MCU to other tasks 
 */
void lqc_create(lqcDeviceType_t deviceType, lqcDeviceConfig_t *deviceConfig, lqcSendMessage_func sendMessageCB, applEvntNotify_func applEvntNotifyCB, applInfoRequest_func applInfoRequestCB, yield_func yieldCB, char *deviceKey)
{
    ASSERT(sendMessageCB != NULL);
    ASSERT(deviceConfig != NULL);
    ASSERT(strlen(deviceKey) > 0);
    ASSERT(strlen(deviceKey) <= lqc__identity_deviceKeySz);

    g_lqCloud.deviceType = deviceType;
    g_lqCloud.sendMessageCB = sendMessageCB;
    g_lqCloud.applEvntNotifyCB = applEvntNotifyCB;
    g_lqCloud.applInfoRequestCB = applInfoRequestCB;
    g_lqCloud.yieldCB = yieldCB;
    g_lqCloud.deviceCnfg = deviceConfig;

    strncpy( g_lqCloud.deviceKey, deviceKey, lqc__identity_deviceKeySz);

    /* **** Failed send msg recovery queue creation **** 
     ********************************************************************************** */
    /*
    PRINTF(0,"MQTT QbufSz=%d\r", sizeof(lqcMqttQueuedMsg_t));                        // was 1964 (2021-02-24)
    lqcMqttQueuedMsg_t mQ[MQTTSEND_QUEUE_SZ] = calloc(1, sizeof(lqcMqttQueuedMsg_t[MQTTSEND_QUEUE_SZ]));      // create mqtt send (recovery) queue
    *g_lqCloud.mqttQueued = mQ;
    if (g_lqCloud.mqttQueued == NULL)
	{
        LQC_appNotify(notificationType_hardFault, "MsgBuf Alloc Failed");
        free(g_lqCloud.mqttQueued);
	}
    */
}


/**
 *	@brief Initialize the LTEx modem.
 *
 *	@param [in] protoCtrl The protocol control object for the LQCloud connection
 *  @param [in] deviceCnfg Device connection settings (host, label, id, etc.)
 */
void lqc_start(uint8_t resetCause)
{
    g_lqCloud.deviceState = lqcDeviceState_offline;
    LQC_doStartEvents(resetCause);

    // g_lqCloud.deviceCnfg = (*g_lqCloud.getDeviceCfgCB)(true);

    // memset(&g_lqCloud.connectEventsStatus, 0, sizeof(lqcConnectEvents_t));

    // if (g_lqCloud.protoCtrl->protocol == protocol_mqtt)
    // {
    //     // g_lqCloud.mqttCtrl = (mqttCtrl_t*)protoCtrl;
    //     // g_lqCloud.connectInfo.msgProto = lqcMessagingProto_mqtt;
    //     ((mqttCtrl_t*)g_lqCloud.protoCtrl)->dataRecvCB = S__cloudReceiver;              // override any local receiver, LQCloud takes that for device actions
    //     g_lqCloud.deviceType = lqcDeviceType_ctrllr;
    // }
    // else
    // {
    //     // TBD
    //     g_lqCloud.deviceType = lqcDeviceType_sensor;
    // }


    // if (LQC_tryConnect())
    // {
        // if (started)
        //      sendStartedMessage()
        //      notifyApp()
        //      if (queuedMsgs)
        //          emptyRecoveryQueue()
    // }


    // // lqcDeviceConfig_t deviceCnfg = lqc_decomposeTokenSas(tokenSas);
    // strncpy(g_lqCloud.hostUrl, deviceCnfg.hostUrl, lqc__identity_hostUrlSz);
    // strncpy(g_lqCloud.deviceId, deviceCnfg.deviceId, lqc__identity_deviceIdSz);
    // strncpy(g_lqCloud.tokenSigExpiry, deviceCnfg.tokenSigExpiry, lqc__identity_tokenSigExpirySz);

    // uint16_t retries = 0;
    // char connectionMsg[80] = {0};
    // g_lqCloud.pendingEvents.startAlert = true;
    // g_lqCloud.pendingEvents.commMetrics = true;
}


/**
 *	@brief Set device shortname, override default "LQC Device".
 *
 *	@param [in] label The device local shortname/label used in some device-2-cloud messaging.
 */
void lqc_setDeviceLabel(const char *label)
{
    memset(g_lqCloud.deviceCnfg->deviceLabel, 0, lqc__identity_deviceLabelSz);
    strncpy(g_lqCloud.deviceCnfg->deviceLabel, label, lqc__identity_deviceLabelSz);
}


void lqc_setDeviceKey(const char *dvcKey)
{
    memset(g_lqCloud.deviceKey, 0, lqc__identity_deviceKeySz);
    strncpy(g_lqCloud.deviceKey, dvcKey, lqc__identity_deviceKeySz);
}


void lqc_enableDiagnostics(diagnosticInfo_t *diagnosticsInfoBlock)
{
    g_lqCloud.diagnosticsInfo = diagnosticsInfoBlock;
}


void lqc_setEventResponse(uint8_t requestEvent, uint16_t result, const char *response)
{
    g_lqCloud.appEventResponse.requestCode = requestEvent;
    g_lqCloud.appEventResponse.resultCode = result;
    strncpy(g_lqCloud.appEventResponse.message, response, sizeof(g_lqCloud.appEventResponse.message));
}


// /**
//  *	@brief Sets connection mode for cloud MQTT connection. Default at LQC init is Continuous. 
//  *
//  *	@param [in] connMode Enum specifying the type of connection between the local device and the LQCloud.
//  *  @param [in] interConnectMin Number of minutes between cloud connections (only applies to periodic connect mode)
//  *  @param [in] holdConnectMin Once a periodic connection is established, the number of minutes to hold it open (only applies to periodic connect mode)
//  */
// void lqc_setConnectMode(lqcConnect_t connectMode, uint16_t interConnectMin, uint16_t holdConnectMin)
// {
//     ASSERT(connectMode < lqcConnect__invalid);
//     g_lqCloud.connectInfo.connectMode = connectMode;
//     g_lqCloud.odConnectInfo.odc_interConnectTicks = interConnectMin * 60000;
//     g_lqCloud.odConnectInfo.odc_holdConnectTicks = holdConnectMin * 60000;
//     g_lqCloud.odConnectInfo.odc_connectAt = 0;
//     g_lqCloud.odConnectInfo.odc_disconnectAt = 0;
// }


// /**
//  *	@brief Query for mode of connectivity to the LQCloud.
//  *  @return Enum representing device to cloud connection mode
//  */
// lqcConnect_t lqc_getConnectMode()
// {
//     return g_lqCloud.connectInfo.connectMode;
// }


// /**
//  *	@brief Query for connectivity to the LQCloud.
//  *  @return Enum representing device to cloud connection state
//  */
// lqcConnectState_t lqc_getConnectState()
// {
//     char subscribeTopic[mqtt__topic_nameSz] = {0};
//     snprintf(subscribeTopic, sizeof(subscribeTopic), IoTHubTemplate_C2D_recvTopic, g_lqCloud.deviceCnfg->deviceId);

//     // NOTE: BGx MQTT has no check for an existing subscription (only change actions of subscribe\unsubscribe)
//     // Here the subscribed test must resubscribe to Azure C2D topic and test for success. Azure and the BGx don't seem to mind resubscribing to a subscribed topic/qos

//     lqcConnectState_t mqttConnState = (lqcConnectState_t)mqtt_getStatus(g_lqCloud.mqttCtrl);                        // get underlying MQTT connection status

//     g_lqCloud.connectInfo.state = (mqttConnState == mqttState_connected) ? g_lqCloud.connectInfo.state : mqttConnState;       // LQC status unchanged on mqtt connected unless...

//     if (g_lqCloud.connectInfo.state == lqcConnectState_messagingReady)                                              // subscription verification indicated
//     {
//         if (mqtt_subscribe(g_lqCloud.mqttCtrl, subscribeTopic, mqttQos_1) == resultCode__success)
//             g_lqCloud.connectInfo.state = lqcConnectState_messagingReady;
//     }
//     return g_lqCloud.connectInfo.state;
// }


// bool lqc_isConnected()
// {
//     return g_lqCloud.connectInfo.state == lqcConnectState_messagingReady;
// }


bool lqc_isOnline()
{
    //return g_lqCloud.connectInfo.state == lqcConnectState_messagingReady;
    return true;
}




// char *lqc_getDeviceId()
// {
//     return &g_lqCloud.deviceCnfg->deviceId;
// }


char *lqc_getDeviceLabel()
{

    return &g_lqCloud.deviceCnfg->deviceLabel;
}


/**
 *	@brief Evaluate diagnostics information block and optionally request a diagnostics alert.
 */
lqcSendResult_t lqc_diagnosticsCheck(diagnosticInfo_t *diagInfo)
{
    if (diagInfo->diagMagic == assert__diagnosticsMagic)
    {
        g_lqCloud.diagnosticsInfo = diagInfo;
        // g_lqCloud.pendingEvents.diagnosticsAlert = true;
    }
}


/**
 *	@brief Perform background processing.
 */
void lqc_doWork()
{
    if (!g_lqCloud.isOnline &&                                                                  // if not online
        (pMillis() -  g_lqCloud.deviceStateChangeAt) > LQC__connection_retryIntervalSecs)       // and retry wait satisfied
    {
        if (g_lqCloud.deviceState == lqcDeviceState_online)
            LQC_doStartEvents();
    }

}


void LQC_doStartEvents()
{
    do
    {
        if (LQC_sendDeviceStarted() != lqcSendResult_sent)        // sending will attempt to bring LQC online, if device is not already
            break;

        if (LQC_sendDiagnosticsAlert(g_lqCloud.diagnosticsInfo) != lqcSendResult_sent)
            break;

        g_lqCloud.deviceState = lqcDeviceState_running;
        return;
    } while (true);
        
    g_lqCloud.deviceState = lqcDeviceState_offline;
}

#pragma endregion


// bool LQC_tryConnect()
// {
//     resultCode_t result;
//     lqcDeviceConfig_t *deviceCnfg;

//     if (g_lqCloud.lastFailedConnectMs == 0 || pMillis() - g_lqCloud.lastFailedConnectMs > PERIOD_FROM_SECONDS(LQC__connection_retryIntervalSecs))
//     {
//         g_lqCloud.lastFailedConnectMs = 0;                              // start fresh

//         deviceCnfg = (*g_lqCloud.getDeviceCfgCB)(true);

//         if (STRCMP(deviceCnfg->magicFlag, "LQCC"))
//             return true;

//         // if (g_lqCloud.deviceType == lqcDeviceType_ctrllr)           // controller: MQTT
//         //     if (LQC_connectMqtt())
//         //         return true;
//         // else                                                        // sensor: HTTP
//         //     if (LQC_connectHttp())
//         //         return true;

//         g_lqCloud.lastFailedConnectMs = pMillis();
//     }
//     return false;
// }


lqcSendResult_t LQC_trySend(const char *topic, const char *body, bool queueOnFail, uint8_t timeoutSeconds)
{
    resultCode_t cbResult = g_lqCloud.sendMessageCB(topic, body, timeoutSeconds);

    if (cbResult != resultCode__success)
        g_lqCloud.deviceState = lqcDeviceState_offline;

    return cbResult == resultCode__success ;
}



#pragma region  LQ Cloud internal functions
/* --------------------------------------------------------------------------------------------- */

/**
 *	@brief Pass event notification up-the-stack, optionally process them.
 */
void LQC_invokeAppEventCBRequest(const char *eventTag, const char *eventMsg)
{
    g_lqCloud.applEvntNotifyCB(eventTag, eventMsg);
}


/**
 *	@brief LQCloud Private: fault handler for LQ Cloud client code.
 * 
 *  @param [in] faultMsg - Message string describing fault.
 */
void LQC_faultHandler(const char *faultMsg)
{
//     // assume network issue, so reset and reconnect
//     //ntwk_resetPdpContexts();
    
//     mqttState_t mqttStatus = mqtt_status("", false);

//     PRINTF(dbgColor__error, "LQC-FAULT: mqttState=%d msg=%s\r", mqttStatus, faultMsg);
//     LQC_appNotify(notificationType_hardFault, faultMsg);

//     uint8_t waitForever = 1;
//     while (waitForever) {}
}



// /**
//  *	@brief LQCloud Private: Get message ID.
//  *  @return message ID number from underlying MQTT stack.
//  */
// uint16_t LQC_getMsgId()
// {
//     return mqtt_getMsgId(g_lqCloud.mqttCtrl);
// }


// /**
//  *	@brief LQCloud Private: send device message to cloud with retry queue management; dequeue previous failed messages (FIFO) and queue failed sends.
//  *  @param [in] topic message topic (expected as fully formed).
//  *  @param [in] body message body (expected as fully formed).
//  *  @param [in] timeout Local override of command timeout, uses default if set to 0.
//  *  @return true if sent or queued, false if dropped.
//  */
// lqcSendResult_t LQC_sendToCloud(const char *topic, const char *body, bool queueOnFail, uint8_t timeoutSeconds)
// {
//     // patch for CloudMelt (2-6-2022): no retry queue, always respond with lqcSendResult_sent

//     // TODO: retry fetch, queue on fail
//     // TODO: check/recover cloud connection on excessive send errors

//     uint16_t msgId = mqtt_getMsgId(g_lqCloud.mqttCtrl);
//     uint32_t pubTimerStart = millis();

//     resultCode_t pubResult = mqtt_publish(g_lqCloud.mqttCtrl, topic, mqttQos_1, body, timeoutSeconds);   // send message
//     g_lqCloud.commMetrics.sendLastDuration = millis() - pubTimerStart;

//     PRINTF(dbgColor__cyan, "LQC_sendToCloud:mId=%d,rc=%d,dur=%d,maxDur=%d\r", 
//                            msgId,
//                            pubResult, 
//                            g_lqCloud.commMetrics.sendLastDuration, 
//                            g_lqCloud.commMetrics.sendMaxDuration);

//     if (pubResult == resultCode__success)
//     {
//         g_lqCloud.commMetrics.sendSucceeds++;
//         g_lqCloud.commMetrics.consecutiveSendFails = 0;
//         // only calc MAX on success, keep timeouts from skewing send durations
//         g_lqCloud.commMetrics.sendMaxDuration = MAX(g_lqCloud.commMetrics.sendMaxDuration, g_lqCloud.commMetrics.sendLastDuration);
//         return lqcSendResult_sent;
//     }

//     g_lqCloud.commMetrics.sendFailures++;
//     g_lqCloud.commMetrics.consecutiveSendFails++;


//     if (queueOnFail)
//     {
//         // queue failed message
//         //     if (pubResult == resultCode__success && fromQueue)
//         //     {
//         //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt = 0;
//         //         g_lqCloud.mqttQueuedTail = ++g_lqCloud.mqttQueuedTail % LQMQ_SEND_QUEUE_SZ;
//         //     }
//         //     if (deferredSend || (pubResult != resultCode__success && !fromQueue))                       // -in FIFO mode -OR- send failed and not already in queue
//         //     {
//         //         if (g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt != 0)                                    
//         //         {
//         //             PRINTF(dbgColor__warn, "MQTTtrySend:ovrflw\r", pubResult);
//         //             g_lqCloud.mqttSendOverflowCnt++;                                                    // tally overflow and silently drop
//         //             return false;
//         //         }
//         //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].retries = 0;
//         //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt = pMillis();                 // allow for timed backoff
//         //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].lastTryAt = pMillis();
//         //         strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].topic, topic, LQMQ_TOPIC_PUB_MAXSZ);
//         //         strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].msg, body, LQMQ_MSG_MAXSZ);
//         //         PRINTF(dbgColor__warn, "MQTTtrySend:msgQueued\r");
//         //         g_lqCloud.mqttQueuedHead = ++g_lqCloud.mqttQueuedHead % LQMQ_SEND_QUEUE_SZ;  
//         //     }

//         return lqcSendResult_queued;
//     }
//     return lqcSendResult_dropped;
// }


#pragma endregion



#pragma region  Local static (private) functions
/* --------------------------------------------------------------------------------------------- */

/**
 *	@brief  Background retry queue message sender.
 */
// static void S_retrySendQueue()
// {
//     if (g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt)                                                     // there is a message to retry
//     {
//         if (wrkTime_isElapsed(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].lastTryAt, LQC__publishRetryDelayMS))   // we have waited sufficiently
//         {
//             g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries++;
//             g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].lastTryAt = pMillis();
//             bool excessRetriesResetConnection = g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries > LQC__publishQueueSz;
//             if (S_ntwkConnectionManager(true, excessRetriesResetConnection))
//             {
//                 PRINTF(dbgColor__dCyan, "RecoverySend retries=%d--\r", g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries);
//                 g_lqCloud.commMetrics.sendMaxRetries = MAX(g_lqCloud.commMetrics.sendMaxRetries, g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].retries);
//                 LQC_mqttTrySend(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].topic,
//                                 g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].msg,
//                                 true,
//                                 0);
//             }
//         }
//     }
// }

//typedef void (*mqttRecvFunc_t)(socket_t sckt, uint16_t msgId, const char *topic, char *topicProps, char *message, uint16_t messageSz);


/**
 *	@brief MQTT LQCloud message receiver.
 * 
 *  @param [in] topic MQTT topic for incoming message, equals one of the subscribed topics.
 *  @param [in] topicProps MQTT topic postamble properties as HTTP query string
 *  @param [in] msgBody Data received from the cloud.
 */
void lqc_receiveMsg(char *message, uint16_t messageSz, const char *props)
{
    // uint16_t msgId = 99;
    // char *topicProps = "";
   //S__cloudReceiver(msgId, topicProps, message, messageSz, props);


    keyValueDict_t propsDict = lq_createQryStrDictionary(props, strlen(props));

    #ifdef _DEBUG
    PRINTF(dbgColor__info, "\r**MQTT--MSG** \tick=%d\r", pMillis());
    PRINTF(dbgColor__cyan, "\rt(%d): %s", strlen(props), props);
    PRINTF(dbgColor__cyan, "\rm(%d): %s", strlen(message), message);
    PRINTF(dbgColor__info, "\rProps(%d)\r", propsDict.count);
    for (size_t i = 0; i < propsDict.count; i++)
    {
        PRINTF(dbgColor__cyan, "%s=%s\r", propsDict.keys[i], propsDict.values[i]);
    }
    PRINTF(0, "\r");
    #endif

    lq_getQryStrDictionaryValue("$.mid", propsDict, g_lqCloud.actnMsgId, LQC__messageIdSz);
    lq_getQryStrDictionaryValue("evN", propsDict, g_lqCloud.actnName, LQC__action_nameSz);
    // strcpy(g_lqCloud.actnMsgId, lqc_getDictValue("$.mid", mqttProps));
    // strcpy(g_lqCloud.actnName, lqc_getDictValue("evN", mqttProps));
    g_lqCloud.actnResult = resultCode__notFound;
    LQC_processIncomingActionRequest(g_lqCloud.actnName, propsDict, message);

}

// /**
//  *	@brief Update the LQC connection state and timestamp the change
//  *  @param [in] newState New LQC connection state
//  */
// static inline void S__ChangeLQCConnectState(uint8_t newState)
// {
//     g_lqCloud.connectInfo.stateEnteredAt = pMillis();
//     g_lqCloud.connectInfo.state = newState;
// }


// /**
//  *	@brief  Manage MQTT connection to server. If continous mode disconnect: attempt reconnect, on-demand mode connect based on timers.
//  *  @param [in] reqstConn Connect without checking connection mode
//  *  @return True if ready to send message(s).
//  */
// void lqc_manageConnection()
// {
    // bool resetConn = false;
    // bool performConnect = g_lqCloud.connectInfo.connectMode == lqcConnect_mqttContinuous && g_lqCloud.connectInfo.state != lqcConnectState_messagingReady;

    // if (g_lqCloud.connectInfo.connectMode == lqcConnect_mqttOnDemand) 
    // {
    //     performConnect = performConnect || wrkTime_isElapsed(g_lqCloud.odConnectInfo.odc_disconnectAt, g_lqCloud.odConnectInfo.odc_interConnectTicks);

    //     // perform disconnect
    //     if (wrkTime_isElapsed(g_lqCloud.odConnectInfo.odc_connectAt, g_lqCloud.odConnectInfo.odc_holdConnectTicks))
    //     {
    //         // future: enter sleep/PSM mode via callback to application function
    //         g_lqCloud.odConnectInfo.odc_connectAt = 0;
    //         g_lqCloud.odConnectInfo.odc_disconnectAt = pMillis();
    //         mqtt_close(g_lqCloud.mqttCtrl);                                                 // close MQTT (cloud messaging proto) session
    //         g_lqCloud.appEventCB(appEvent_ntwk_disconnected, "");
    //     }
    // }

    // if (g_lqCloud.commMetrics.consecutiveSendFails >= LQC__send_resetAtConsecutiveFailures)
    // {
    //     S__ChangeLQCConnectState(lqcConnectState_idleClosed);
    //     lqcNtwk_resetIntf(true);
    // }

    // // on-demand and failure detection complete, if messaging ready done here
    // if (g_lqCloud.connectInfo.state == lqcConnectState_messagingReady)
    //     return;
    
    // // work needed to bring communications up and messaging
    // providerInfo_t *provider = lqcNtwk_awaitProvider(10);
    // if (g_lqCloud.connectInfo.state == lqcConnectState_idleClosed)                      // returned here for another try
    // {
    //     if (strlen(provider->name) == 0)
    //         return;                                                                     // try again next pass

    //     g_lqCloud.connectInfo.state = lqcConnectState_providerReady;
    // }

    // if (g_lqCloud.connectInfo.state == lqcConnectState_providerReady)
    // {
    //     networkInfo_t *network = lqcNtwk_activateNetwork(provider->defaultContext);
    //     if (!network->isActive)
    //         return;                                                                     // try again next pass

    //     g_lqCloud.connectInfo.state = lqcConnectState_networkReady;
    //     performConnect = true;
    // }

    // /*  Cloud Messaging Protocol Connect
    //  * ----------------------------------------------------------------------------------------- */
    // if (performConnect)
    // {
    //     lqcConnectState_t connState;

    //     if (g_lqCloud.connectInfo.msgProto == lqcMessagingProto_mqtt)
    //        connState = S__connectToCloudMqtt();
    // }
// }


// static bool S_ntwkConnectionManager(bool connRequested, bool resetConn)
// {
//     static uint32_t onConnectAt;              // if in on-demand mode: when did the connection start
//     static uint32_t lastReconnectAt;          // if in reconnect condition: wait short interval between reconnection attempts

//     bool connRequired = g_lqCloud.connectInfo.mode == lqcConnectMode_continuous;
//     bool performConnect =  (g_lqCloud.connectState != lqcConnectState_ready && (connRequested)) || resetConn;
//     bool performDisconnect = resetConn;

//     if (g_lqCloud.connectInfo.mode == lqcConnectMode_onDemand) 
//     {
//         connRequired || wrkTime_doNow(&g_lqCloud.connectSched);
//         performDisconnect = wrkTime_isElapsed(onConnectAt, PERIOD_FROM_SECONDS(lqc__connection_onDemand_connDurationSecs));
//     }

//     mqttState_t mqttState = mqtt_getStatusForContext(dataContext_0);

//     PRINTF(0, "NtwkConnMgr-State:lqc=%d/mqtt=%d connRqst=%d connRqrd=%d willConn=%d reset=%d\r", g_lqCloud.connectState, mqttState, connRequested, connRequired, performConnect, resetConn);

//     /* required connection actions determined above, now change state if required
//      --------------------------------------------------------------------------------- */

//     if (performDisconnect)
//     {
//         onConnectAt = 0;                        // stop connection duration timer (applicable to on-demand)
//         mqtt_close(g_lqCloud.mqttCtrl);         // close MQTT messaging session
//         LQC_appReqst(appReqst_ntwk_disconnect, "");
//     }

//     if (performConnect)
//     {
//         uint8_t attempts = 0;
//         bool connectError = false;
//         bool ntwkStarted = true;                                        // assume true, networkStartCB() will override if invoked
//         appReqstResult_t rqstResult;

//         while (attempts < LQC__connection_attemps)                      // do connect(), if not successful wait briefly for conditions to change then retry up to 3 times
//         {
//             rqstResult = LQC_appReqst(appReqst_ntwk_connect, "");
//             ntwkStarted = rqstResult.statusCode == resultCode__success;

//             if (ntwkStarted && S_connectToCloudMqtt(mqttState) == lqcConnectState_ready)
//             {
//                 LQC_appNotif(appNotif_ntwk_connected, "NtwkCMgr");
//                 return true;
//             }
//             connectError = true;
//             LQC_appNotif(appNotif_ntwk_disconnected, "NtwkCMgr");
//         }
//     }
//     return false;
// }



#pragma endregion
