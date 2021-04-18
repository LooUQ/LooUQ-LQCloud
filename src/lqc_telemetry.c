//	Copyright (c) 2020 LooUQ Incorporated.
//	Licensed under The MIT License. See LICENSE in the root directory.

#define _DEBUG                          // enable/expand 
#include "lqc_internal.h"

// debugging output options             UNCOMMENT one of the next two lines to direct debug (PRINTF) output
#include <jlinkRtt.h>                   // output debug PRINTF macros to J-Link RTT channel
// #define SERIAL_OPT 1                    // enable serial port comm with devl host (1=force ready test)

extern lqCloudDevice_t g_lqCloud;

#define MIN(x, y) (((x)<(y)) ? (x):(y))
#define LQC_DEVICESTATUS_PROPSZ 20


/**
 *	\brief Send telemetry message to LooUQ Cloud.
 * 
 *  \param [in] eventName - Descriptive name for this specific telemetry data.
 *  \param [in] eventValue - Summary string to include with the telemetry data body.
 *  \param [in] body - Message body, JSON formatted
 */
bool lqc_sendTelemetry(const char *evntName, const char *evntSummary, const char *bodyJson)
{
    char msgEvntName[LQC_EVENT_NAME_SZ] = {0};
    char msgEvntSummary[LQC_EVENT_SUMMARY_SZ] = {0};
    char msgTopic[LQMQ_TOPIC_PUB_MAXSZ];
    char msgBody[LQC_EVENT_BODY_SZ]; 
    char deviceStatus[DVCSTATUS_SZ] = {0};
    char dStatusBuild[DVCSTATUS_SZ] = {0};
    
    if (evntName[0] == ASCII_cNULL)
        strncpy(msgEvntName, "telemetry", 9);
    else
        strncpy(msgEvntName, evntName, MIN(strlen(evntName), LQC_EVENT_NAME_SZ-1));

    if (evntSummary[0] != ASCII_cNULL)
        snprintf(msgEvntSummary, LQC_EVENT_SUMMARY_SZ, "\"descr\": \"%s\",", evntSummary);

    // telemetry options, get device status using registered service functions
    if (g_lqCloud.powerStatusCB)
    {
        char optProp[LQC_DEVICESTATUS_PROPSZ];
        double pwrStat = g_lqCloud.powerStatusCB();
        snprintf(optProp, LQC_DEVICESTATUS_PROPSZ, "\"pwrV\": %.2f,", pwrStat);
        strcat(dStatusBuild, optProp);
    }
    if (g_lqCloud.batteryStatusCB)
    {
        char optProp[LQC_DEVICESTATUS_PROPSZ];
        double battStat = g_lqCloud.batteryStatusCB();
        snprintf(optProp, LQC_DEVICESTATUS_PROPSZ, "\"battStatus\":%d,", battStat);
        strcat(dStatusBuild, optProp);
    }
    if (g_lqCloud.memoryStatusCB)
    {
        char optProp[LQC_DEVICESTATUS_PROPSZ];
        int memStat = g_lqCloud.memoryStatusCB();
        snprintf(optProp, LQC_DEVICESTATUS_PROPSZ, "\"memFree\":%d,", memStat);
        strcat(dStatusBuild, optProp);
    }
    uint8_t dStatusSz = strlen(dStatusBuild);
    if (dStatusSz > 0) 
    {
        dStatusBuild[dStatusSz-1] = 0;  // remove trailing ','
        snprintf(deviceStatus, DVCSTATUS_SZ, ",\"deviceStatus\":{%s}", dStatusBuild);
    }

    // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=tdat&evC=%s&evN=%s"
    snprintf(msgTopic, LQMQ_TOPIC_PUB_MAXSZ, LQMQ_MSG_D2CTOPIC_TELEMETRY_TMPLT, g_lqCloud.deviceId, g_lqCloud.msgNm++, "appl", msgEvntName);
    snprintf(msgBody, LQMQ_MSG_MAXSZ, "{%s\"telemetry\": %s%s}", msgEvntSummary, bodyJson, deviceStatus);
    return LQC_mqttTrySend(msgTopic, msgBody, false);
}

