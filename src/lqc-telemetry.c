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

#include <jlinkRtt.h>                   // if you have issues with Intellisense on PRINTF() or DBGCOLOR_ try temporarily placing jlinkRtt.h outside conditional
                                        // or add "_DEBUG = 2" to c_cpp_properties.json "defines" section list

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

