/******************************************************************************
 *  \file lqc-telemetry.c
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
 * LooUQ LQCloud Client Telemetry Services
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
    char msgEvntName[lqc__msg_nameSz] = {0};
    char msgEvntSummary[lqc__msg_summarySz] = {0};
    char msgTopic[LQMQ_TOPIC_PUB_MAXSZ];
    char msgBody[lqc__msg_bodySz]; 
    char deviceStatus[DVCSTATUS_SZ] = {0};
    char dStatusBuild[DVCSTATUS_SZ] = {0};
    
    if (evntName[0] == '\0')
        strncpy(msgEvntName, "telemetry", 9);
    else
        strncpy(msgEvntName, evntName, MIN(strlen(evntName), sizeof(msgEvntName)-1));

    if (evntSummary[0] != '\0')
        snprintf(msgEvntSummary, sizeof(msgEvntSummary), "\"descr\": \"%s\",", evntSummary);

    // telemetry options, report device status for only registered service functions
    if (g_lqCloud.powerStatusCB)
    {
        char optProp[LQC_DEVICESTATUS_PROPSZ];
        uint16_t pwrStat = g_lqCloud.powerStatusCB();
        snprintf(optProp, LQC_DEVICESTATUS_PROPSZ, "\"pwrmv\": %d,", pwrStat);
        strcat(dStatusBuild, optProp);
    }
    if (g_lqCloud.batteryStatusCB)
    {
        char optProp[LQC_DEVICESTATUS_PROPSZ];
        uint16_t battStat = g_lqCloud.batteryStatusCB();
        snprintf(optProp, LQC_DEVICESTATUS_PROPSZ, "\"bttmv\":%d,", battStat);
        strcat(dStatusBuild, optProp);
    }
    if (g_lqCloud.memoryStatusCB)
    {
        char optProp[LQC_DEVICESTATUS_PROPSZ];
        uint32_t memStat = g_lqCloud.memoryStatusCB();
        snprintf(optProp, LQC_DEVICESTATUS_PROPSZ, "\"memb\":%d,", memStat);
        strcat(dStatusBuild, optProp);
    }
    uint8_t dStatusSz = strlen(dStatusBuild);
    if (dStatusSz > 0) 
    {
        dStatusBuild[dStatusSz-1] = 0;  // remove trailing ','
        snprintf(deviceStatus, DVCSTATUS_SZ, ",\"deviceStatus\":{%s}", dStatusBuild);
    }

    
    // "devices/%s/messages/events/mId=~%d&mV=1.0&evT=tdat&evC=%s&evN=%s"
    snprintf(msgTopic, LQMQ_TOPIC_PUB_MAXSZ, IotHubTemplate_D2C_topicTelemetry, g_lqCloud.deviceId, LQC_getMsgId(), "appl", msgEvntName);
    snprintf(msgBody, LQMQ_MSG_MAXSZ, "{%s\"telemetry\": %s%s}", msgEvntSummary, bodyJson, deviceStatus);
    return LQC_mqttTrySend(msgTopic, msgBody, false);
}

