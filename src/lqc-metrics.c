/******************************************************************************
 *  \file lqc-metrics.c
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
 * LQCloud metrics and diagnostics support. Requires lq-assert and lq-SAMDcode
 * subsystems.
 *****************************************************************************/

#define FORCE_DIAG_SEND 0

#define SRCFILE "MTR"                           // create SRCFILE (3 char) MACRO for lq-diagnostics ASSERT
#include "lqc-internal.h"

extern lqCloudDevice_t g_lqCloud;
extern diagnosticControl_t g_diagControl;


// bool lqc_reportResetToCloud(uint8_t rcause)
// {
//     if (rcause != diagRcause_watchdog && rcause != diagRcause_system)           // if non-error reset, nothing to report
//     {
//         lqc_reportMetrics();
//         lqc_reportDiagnostics();
//         lqc_reportFault();
//     }
//     return false;
// }


/**
 *	\brief Metrics reporting 
 * 
 *  Send LQCloud performance metrics information.
 */
void lqc_reportCommMetrics()
{
    char alrtSummary[40];
    char alrtBody[120] = {0};

    snprintf(alrtSummary, sizeof(alrtSummary), "%s CommMetrics", g_lqCloud.deviceCnfg->deviceLabel);
    LQC_composeCommMetricsReport(alrtBody, sizeof(alrtBody));
    LQC_sendAlert(lqcEventClass_lqcloud, "CommMetricsReport", alrtSummary, alrtBody);

    g_lqCloud.commMetrics.metricsStart = pMillis;                       // reset LQC comm metrics
    g_lqCloud.commMetrics.connectResets = 0;
    g_lqCloud.commMetrics.sendLastDuration = 0;
    g_lqCloud.commMetrics.sendMaxDuration = 0;
    g_lqCloud.commMetrics.sendSucceeds = 0;
    g_lqCloud.commMetrics.sendFailures = 0;
}


/**
 *	\brief Diagnostics reporting 
 * 
 *  Send LQCloud diagnostics information.
 */
void lqc_reportDiagnostics()
{
    char alrtSummary[40];
    char alrtBody[120] = {0};

    snprintf(alrtSummary, sizeof(alrtSummary), "%s Diags", g_lqCloud.deviceCnfg->deviceLabel);
    LQC_composeDiagnosticsReport(alrtBody, sizeof(alrtBody));

    LQC_sendAlert(lqcEventClass_lqcloud, "DiagnosticsReport", alrtSummary, alrtBody);
}


/**
 *	\brief Fault reporting 
 * 
 *  Send LQCloud fault information.
 */
void lqc_reportFault()
{
    char alrtSummary[40];
    char alrtBody[120] = {0};

    snprintf(alrtSummary, sizeof(alrtSummary), "%s Diags", g_lqCloud.deviceCnfg->deviceLabel);
    LQC_composeFaultReport(alrtBody, sizeof(alrtBody));

    LQC_sendAlert(lqcEventClass_lqcloud, "FaultReport", alrtSummary, alrtBody);
}


void LQC_composeCommMetricsReport(char *report, uint8_t bufferSz)
{
    snprintf(report, bufferSz, "{\"resets\":%d,\"sndMaxDur\":%d,\"sndLstDur\":%d,\"succeedCnt\":%d, \"failCnt\":%d}\r", 
             g_lqCloud.commMetrics.connectResets,
             g_lqCloud.commMetrics.sendMaxDuration,
             g_lqCloud.commMetrics.sendLastDuration,
             g_lqCloud.commMetrics.sendSucceeds,
             g_lqCloud.commMetrics.sendFailures
    );
}

void LQC_clearMetrics(lqcMetricsType_t metricType)
{
    if (metricType == lqcMetricsType_metrics || metricType == lqcMetricsType_all)
    {
        memset(&g_lqCloud.commMetrics, 0, sizeof(lqcCommMetrics_t));
    }
    if (metricType == lqcMetricsType_diagnostics || metricType == lqcMetricsType_all)
    {
        memset(&g_diagControl.diagnosticInfo, 0, sizeof(diagnosticControl_t));
    }
}


