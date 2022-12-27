/******************************************************************************
 *  \file lqc_internal.h
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2020 LooUQ Incorporated.
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
 ******************************************************************************
 * LQ Cloud Internal Functions Declarations
 *****************************************************************************/

#ifndef __LQC_INTERNAL_H__
#define __LQC_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <lq-types.h>
#include <lq-diagnostics.h>

#include <lqcloud.h>

#include <ltemc-tls.h>
#include <ltemc-mqtt.h>                             // REQUIRED communications for now!
#include <ltemc-http.h>

#include "lqc-ntwk.h"
// #include "lqc-mqtt.h"
// #include "lqc-http.h"

#define PRODUCT "LC"

enum 
{
    LQC__connectionRetryCnt = 3,
    LQC__messageIdSz = 36,

    LQC__publishQueueSz = 2,
    LQC__publishDefaultTimeoutS = 15,                       /// how long to wait for publish to complete
    LQC__publishRetryDelayMS = 30000,                       /// how long to wait before attempting publish retr
    LQC__connectionRetryWaitSec = 120,

    LQMQ_MSG_MAXSZ = 1548,                                  /// largest MQTT send message for BGx
    LQMQ_TOPIC_PUB_MAXSZ = 437,                             /// total buffer size to construct topic for pub\sub AT actions
    LQMQ_SEND_QUEUE_SZ = 2,
    LOOUQ_FLASHDICTKEY__LQCDEVICECONFIG = 201,
    DVCSTATUS_SZ = 61,
    LQC_EVNTCLASS_SZ = 5,
    LQCACTN_BUF_SZ = 80,                                    /// good averaged for up to a couple params
    LQCACTN_LQCACTIONS_BODY_SZ = 180                        /// calculated from actual JSON
};


typedef enum lqcMetricsType_tag
{
    lqcMetricsType_metrics = 0,
    lqcMetricsType_diagnostics = 1,
    lqcMetricsType_fault = 2,
    lqcMetricsType_all = 3
} lqcMetricsType_t;


typedef enum lqcStartType_tag
{
    lqcStartType_cold = 0,
    lqcStartType_recover = 1
} lqcStartType_t;


typedef struct lqcApplAction_tag
{
    char name[LQC__action_nameSz];              /// Action name, know by LQ Cloud.
    lqcAction_func actionCB;                    /// Action function to be invoked, to perform device action.
    char paramList[LQC__action_paramsListSz];   /// this is used for registration with LQ Cloud, function will receive a propsDict_t parameter.
} lqcApplAction_t;


typedef struct lqcPendingEvents_tag
{
    bool startAlert;
    bool diagnosticsAlert;
    bool commMetrics;
} lqcPendingEvents_t;


typedef struct lqcCommMetrics_tag       /// Note: diagnostic counters below can be optionally reset with remote action
{
    uint32_t metricsStart;              /// Tick count at metric cycle start (cycle is 24 hours)
    uint8_t connectResets;              /// Number of connection resets initiated by LQCloud connection manager. Reset by system reset.
    uint16_t sendLastDuration;          /// Duration (in millis) of last mqtt publish operation
    uint16_t sendMaxDuration;           /// Maximum duration for any mqtt publish operation. Reset on connection reset.
    uint16_t sendSucceeds;              /// tally of successful sends
    uint8_t sendFailures;               /// tally of failed sends, consective or not in metrics period
    uint8_t consecutiveSendFails;       /// consecutive send failures; not reset on metrics reset, reset on send successful
} lqcCommMetrics_t;


typedef struct lqcRecoveryQueue_tag
{
    char * queueBuffer;
    uint16_t bufferSz;
    uint8_t queueCnt;

    uint32_t lastTryAt;
} lqcRecoveryQueue_t;


#define QUEUED_TOPIC_AT(P) (P + 10)
#define QUEUED_MSG_AT(P) (P + 10 + 0)

typedef struct lqcMqttQueuedMsg_tag
{
    uint32_t queuedAt;
    uint8_t queueIndx;
    char msgType;                       // a = alert, t = telemetry

    uint16_t topicSz;
    uint16_t msgSz;

    uint32_t lastTryAt;
    uint8_t retries;
    char topic[mqtt__topic_bufferSz];
    char msg[mqtt__messageSz];
} lqcMqttQueuedMsg_t;


/** 
 *  \brief typedef of MQTT subscription receiver function (required signature for appl callback).
*/
typedef void (*lqcRecv_func_t)(char *topic, char *props, char *message);


/** 
 *  \brief Struct the respresents a connection to the LooUQ Cloud - IoT Application Integration Platform.
*/
typedef struct lqCloudDevice_tag
{
    lqcDeviceType_t deviceType;                                     /// Controller (MQTT/interactive) -OR- Sensor (HTTP/data pump)
    lqcDeviceConfig_t *deviceCnfg;                                  /// device communications settings
    char deviceKey[SET_PROPLEN(lqc__identity_deviceKeySz)];

    // mqttCtrl_t *mqttCtrl;                                        /// MQTT connection to access LQCloud (interactive)
    // httpCtrl_t *httpCtrl;                                        /// HTTP connection to access LQCloud (sensor)
    // streamCtrl_t *protoCtrl;

    uint8_t resetCause;
    bool isOnline;                                              /// Indicates that a viable "connection" exists. HTTP last request/MQTT connection succeeded
    lqcDeviceState_t deviceState;                                   
    uint32_t deviceStateChangeAt;
    uint16_t lastMsgId;

    lqcApplAction_t applActions[LQC__actionCnt];                /// Application invokable public methods (registered with LQ Cloud). LQ Cloud validates requests prior to messaging device.
    char actnMsgId[SET_PROPLEN(LQC__action_MsgIdSz)];           /// Action request mId, will be aCId (correlation ID).
    char actnName[SET_PROPLEN(LQC__action_nameSz)];             /// Last action requested by cloud. Is reset on action request receive.
    uint16_t actnResult;                                        /// Action result code for last action request. 
    diagnosticInfo_t *diagnosticsInfo;
    lqcCommMetrics_t commMetrics;                               /// Internal operations tracking counters
    appEventResponse_t appEventResponse;                        /// struct containing optional application response to an appEvent message (callback)

    lqcSendMessage_func sendMessageCB;
    applEvntNotify_func applEvntNotifyCB;                       /// Application notification and action/info request callback
    applInfoRequest_func applInfoRequestCB;
    yield_func yieldCB;                                         /// Callback into application for watchdog/background operations during long running cloud processes

    lqcRecoveryQueue_t recoveryQueue;
    uint16_t droppedAlrtMsgCnt;
    uint16_t droppedTeleMsgCnt;
} lqCloudDevice_t;


#ifdef __cplusplus
extern "C"
{
#endif


/* Version 0.2.1
*/
bool LQC_tryConnect();
lqcSendResult_t LQC_trySend(const char *topic, const char *body, bool queueOnFail, uint8_t timeoutSeconds);





/* LQCloud Internal Use Functions LQC_ prefix
* These are not defined static because they are used across compilation units.
------------------------------------------------------------------------------------------------ */
void LQC_invokeAppEventCBRequest(const char *eventTag, const char *eventMsg);
void LQC_faultHandler(const char *faultMsg);


// cloud messaging
uint16_t LQC_getMsgId();
lqcSendResult_t LQC_sendToCloud(const char *topic, const char *body, bool retryFailed, uint8_t timeoutSeconds);

void LQC_doStartEvents();

// cloud alerts
/**
 *  \brief Send device started Alert message to LQCloud.
 *  \return lqcSendResult enum value describing outcome
*/
lqcSendResult_t LQC_sendDeviceStarted();

lqcSendResult_t LQC_sendDiagnosticsAlert(diagnosticInfo_t * diagInfo);

lqcSendResult_t LQC_sendAlert(lqcEventClass_t alrtClass, const char *alrtName, const char *alrtSummary, const char *bodyJson);

// cloud actions
void LQC_processIncomingActionRequest(const char *actnName, keyValueDict_t actnParams, const char *msgBody);

// metrics
void LQC_composeCommMetricsReport(char *report, uint8_t bufferSz);
void LQC_clearCommMetrics();

#ifdef __cplusplus
}
#endif // !__cplusplus

#endif  /* !__LQC_INTERNAL_H__ */
