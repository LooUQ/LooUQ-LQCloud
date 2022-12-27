/******************************************************************************
 *  \file lqc-mqtt.c
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2020-2022 LooUQ Incorporated.
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

#include <lq-types.h>

#include "lqc-internal.h"
#include "lqc-azure.h"
#include <lq-SAMDutil.h>


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
//static inline void S__ChangeLQCConnectState(uint8_t newState);

/**
 *	\brief Connect via MQTT to the LQCloud.
 *  \return resultCode (HTTPstyle) indicating success or type of failure. If failure, caller should also use lqc_getConnectState() for details.
 */
bool LQC_connectMqtt(/*mqttState_t mqttState*/)
{
    //ASSERT(pdp Contxt >= lqcConnectState_networkReady, srcfile_lqc_lqcloud_c);

    // if (wrkTime_isElapsed(g_lqCloud.connectInfo.stateEnteredAt, PERIOD_FROM_SECONDS(30)))       // detect state change stalled (BGx MQTT connect can do this)
    // {
    //     lqcNtwk_resetIntf(true);                                                                // MQTT is not known to recover from this (Mar2022-further testing reqd)
    //     S__ChangeLQCConnectState(lqcConnectState_idleClosed);
    // }

    // mqttState_t mqttState = mqtt_getStatus(g_lqCloud.mqttCtrl);
    // if (mqttState == mqttState_pending)
    //     return lqcConnectState_connFault;

    // g_lqCloud.connectInfo.state = mqttState;

    resultCode_t openRslt = 0;
    resultCode_t connRslt = 0;
    resultCode_t subsRslt = 0;

    openRslt = mqtt_open(g_lqCloud.mqttCtrl);
    switch (openRslt)
    {
        case resultCode__success:
            break;

        // hard errors
        case resultCode__badRequest:
        case resultCode__notFound:
            g_lqCloud.appEventCB(appEvent_fault_hardFault, "Invalid Settings");
            break;
        case resultCode__gone:
            g_lqCloud.appEventCB(appEvent_ntwk_disconnected, "MQTT(410)");
            break;

        // soft errors
        case resultCode__timeout:
        case resultCode__conflict:
            g_lqCloud.appEventCB(appEvent_info, "No Connect, Retrying");
            break;
    }

    if (openRslt == resultCode__success)                     // open, if not connected (or ready)... connect
    {
        char userId[lqc__identity_userIdSz];
        char tokenSAS[lqc__identity_tokenSasSz];

        snprintf(userId, sizeof(userId), IotHubTemplate_sas_userId, g_lqCloud.deviceCnfg->hostUrl, g_lqCloud.deviceCnfg->deviceId);
        lqc_composeSASToken(tokenSAS, sizeof(tokenSAS), g_lqCloud.deviceCnfg->hostUrl, g_lqCloud.deviceCnfg->deviceId, g_lqCloud.deviceCnfg->tokenSigExpiry);

        connRslt = mqtt_connect(g_lqCloud.mqttCtrl, true);
        switch (connRslt)
        {
            case resultCode__success:
                break;

            // hard errors
            case resultCode__unavailable:
            case resultCode__badRequest:
                g_lqCloud.appEventCB(appEvent_fault_hardFault, "Invalid Settings");
                break;
            case resultCode__forbidden:
                g_lqCloud.appEventCB(appEvent_fault_hardFault, "Not Authorized");
                break;

            // soft errors
            case resultCode__timeout:
                break;
        }
    }

    if (connRslt == resultCode__success)                // mqtt status agrees, verify subscribed
    {
        char subscribeTopic[mqtt__topic_nameSz];
        snprintf(subscribeTopic, sizeof(subscribeTopic), IoTHubTemplate_C2D_recvTopic, g_lqCloud.deviceCnfg->deviceId);

        subsRslt = mqtt_subscribe(g_lqCloud.mqttCtrl, subscribeTopic, mqttQos_1);
    }
    return openRslt == resultCode__success && connRslt == resultCode__success && subsRslt == resultCode__success;
}


/**
 *	\brief LQCloud Private: send device message to cloud with retry queue management; dequeue previous failed messages (FIFO) and queue failed sends.
 *  \param [in] topic message topic (expected as fully formed).
 *  \param [in] body message body (expected as fully formed).
 *  \param [in] timeout Local override of command timeout, uses default if set to 0.
 *  \return true if sent or queued, false if dropped.
 */
lqcSendResult_t LQC_sendMqtt(const char *topic, const char *body, bool queueOnFail, uint8_t timeoutSeconds)
{
    lqcSendResult_t sendResult;
    sendResult.msgId = ++g_lqCloud.msgId;
    uint32_t pubTimerStart = millis();

    resultCode_t pubResult = mqtt_publish(g_lqCloud.mqttCtrl, topic, mqttQos_1, body, timeoutSeconds);   // send message
    g_lqCloud.commMetrics.sendLastDuration = millis() - pubTimerStart;

    PRINTF(dbgColor__cyan, "LQC_sendToCloud:mId=%d,rc=%d,dur=%d,maxDur=%d\r", 
                           sendResult.msgId,
                           pubResult, 
                           g_lqCloud.commMetrics.sendLastDuration, 
                           g_lqCloud.commMetrics.sendMaxDuration);

    if (pubResult == resultCode__success)
    {
        g_lqCloud.commMetrics.sendSucceeds++;
        g_lqCloud.commMetrics.consecutiveSendFails = 0;
        // only calc MAX on success, keep timeouts from skewing send durations
        g_lqCloud.commMetrics.sendMaxDuration = MAX(g_lqCloud.commMetrics.sendMaxDuration, g_lqCloud.commMetrics.sendLastDuration);
        sendResult.sent = true;
        return sendResult;
    }

    g_lqCloud.commMetrics.sendFailures++;
    g_lqCloud.commMetrics.consecutiveSendFails++;

    if (queueOnFail)
    {
        // queue failed message
        //     if (pubResult == resultCode__success && fromQueue)
        //     {
        //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedTail].queuedAt = 0;
        //         g_lqCloud.mqttQueuedTail = ++g_lqCloud.mqttQueuedTail % LQMQ_SEND_QUEUE_SZ;
        //     }
        //     if (deferredSend || (pubResult != resultCode__success && !fromQueue))                       // -in FIFO mode -OR- send failed and not already in queue
        //     {
        //         if (g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt != 0)                                    
        //         {
        //             PRINTF(dbgColor__warn, "MQTTtrySend:ovrflw\r", pubResult);
        //             g_lqCloud.mqttSendOverflowCnt++;                                                    // tally overflow and silently drop
        //             return false;
        //         }
        //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].retries = 0;
        //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].queuedAt = pMillis();                 // allow for timed backoff
        //         g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].lastTryAt = pMillis();
        //         strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].topic, topic, LQMQ_TOPIC_PUB_MAXSZ);
        //         strncpy(g_lqCloud.mqttSendQueue[g_lqCloud.mqttQueuedHead].msg, body, LQMQ_MSG_MAXSZ);
        //         PRINTF(dbgColor__warn, "MQTTtrySend:msgQueued\r");
        //         g_lqCloud.mqttQueuedHead = ++g_lqCloud.mqttQueuedHead % LQMQ_SEND_QUEUE_SZ;  
        //     }
        sendResult.queued = true;
    }
    return sendResult;
}


#pragma endregion



#pragma region  Local static (private) functions
/* --------------------------------------------------------------------------------------------- */

/**
 *	\brief  Background retry queue message sender.
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
 *	\brief MQTT LQCloud message receiver.
 * 
 *  \param [in] topic MQTT topic for incoming message, equals one of the subscribed topics.
 *  \param [in] topicProps MQTT topic postamble properties as HTTP query string
 *  \param [in] msgBody Data received from the cloud.
 */
static void S__cloudReceiver(dataCntxt_t dataCntxt, uint16_t msgId, const char *topic, char *topicProps, char *message, uint16_t messageSz)
{
    keyValueDict_t mqttProps = lq_createQryStrDictionary(topicProps, strlen(topicProps));

    #ifdef _DEBUG
    PRINTF(dbgColor__info, "\r**MQTT--MSG** \tick=%d\r", pMillis());
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
    LQC_processIncomingActionRequest(g_lqCloud.actnName, mqttProps, message);
}


// /**
//  *	\brief Update the LQC connection state and timestamp the change
//  *  \param [in] newState New LQC connection state
//  */
// static inline void S__ChangeLQCConnectState(uint8_t newState)
// {
//     g_lqCloud.connectInfo.stateEnteredAt = pMillis();
//     g_lqCloud.connectInfo.state = newState;
// }


/**
 *	\brief  Manage MQTT connection to server. If continous mode disconnect: attempt reconnect, on-demand mode connect based on timers.
 *  \param [in] reqstConn Connect without checking connection mode
 *  \return True if ready to send message(s).
 */
void lqc_manageConnection()
{
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
}


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

// /**
//  *	\brief Connect via MQTT to the LQCloud.
//  *  \return resultCode (HTTPstyle) indicating success or type of failure. If failure, caller should also use lqc_getConnectState() for details.
//  */
// static lqcConnectState_t S__connectToCloudMqtt(/*mqttState_t mqttState*/)
// {
//     ASSERT(g_lqCloud.connectInfo.state >= lqcConnectState_networkReady, srcfile_lqc_lqcloud_c);

//     if (wrkTime_isElapsed(g_lqCloud.connectInfo.stateEnteredAt, PERIOD_FROM_SECONDS(30)))       // detect state change stalled (BGx MQTT connect can do this)
//     {
//         lqcNtwk_resetIntf(true);                                                                // MQTT is not known to recover from this (Mar2022-further testing reqd)
//         S__ChangeLQCConnectState(lqcConnectState_idleClosed);
//     }

//     // mqttState_t mqttState = mqtt_getStatus(g_lqCloud.mqttCtrl);
//     // if (mqttState == mqttState_pending)
//     //     return lqcConnectState_connFault;

//     // g_lqCloud.connectInfo.state = mqttState;

//     resultCode_t mqttRslt;
//     if (g_lqCloud.connectInfo.state == lqcConnectState_networkReady)
//     {
//         mqttRslt = mqtt_open(g_lqCloud.mqttCtrl);
//         switch (mqttRslt)
//         {
//             case resultCode__success:
//                 S__ChangeLQCConnectState(lqcConnectState_messagingOpen);
//                 break;

//             // hard errors
//             case resultCode__badRequest:
//             case resultCode__notFound:
//                 g_lqCloud.appEventCB(appEvent_fault_hardFault, "Invalid Settings");
//                 break;
//             case resultCode__gone:
//                 g_lqCloud.appEventCB(appEvent_ntwk_disconnected, "MQTT(410)");
//                 break;

//             // soft errors
//             case resultCode__timeout:
//             case resultCode__conflict:
//                 g_lqCloud.appEventCB(appEvent_info, "No Connect, Retrying");
//                 break;
//         }
//     }

//     if (g_lqCloud.connectInfo.state == lqcConnectState_messagingOpen)                     // open, if not connected (or ready)... connect
//     {
//         char userId[lqc__identity_userIdSz];
//         char tokenSAS[lqc__identity_tokenSasSz];

//         snprintf(userId, sizeof(userId), IotHubTemplate_sas_userId, g_lqCloud.deviceCnfg->hostUrl, g_lqCloud.deviceCnfg->deviceId);
//         lqc_composeSASToken(tokenSAS, sizeof(tokenSAS), g_lqCloud.deviceCnfg->hostUrl, g_lqCloud.deviceCnfg->deviceId, g_lqCloud.deviceCnfg->tokenSigExpiry);

//         mqttRslt = mqtt_connect(g_lqCloud.mqttCtrl, true);
//         switch (mqttRslt)
//         {
//             case resultCode__success:
//                 g_lqCloud.connectInfo.state = lqcConnectState_messagingConnected;
//                 break;

//             // hard errors
//             case resultCode__unavailable:
//             case resultCode__badRequest:
//                 g_lqCloud.appEventCB(appEvent_fault_hardFault, "Invalid Settings");
//                 break;
//             case resultCode__forbidden:
//                 g_lqCloud.appEventCB(appEvent_fault_hardFault, "Not Authorized");
//                 break;

//             // soft errors
//             case resultCode__timeout:
//                 break;
//         }
//     }

//     if (g_lqCloud.connectInfo.state == lqcConnectState_messagingConnected)                // mqtt status agrees, verify subscribed
//     {
//         char subscribeTopic[mqtt__topic_nameSz];
//         snprintf(subscribeTopic, sizeof(subscribeTopic), IoTHubTemplate_C2D_recvTopic, g_lqCloud.deviceCnfg->deviceId);

//         mqttRslt = mqtt_subscribe(g_lqCloud.mqttCtrl, subscribeTopic, mqttQos_1);
//         if (mqttRslt == resultCode__success)
//             g_lqCloud.connectInfo.state = lqcConnectState_messagingReady;
//     }
//     return g_lqCloud.connectInfo.state;
// }


#pragma endregion
