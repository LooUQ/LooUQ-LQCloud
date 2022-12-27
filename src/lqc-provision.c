/******************************************************************************
 *  \file lqc-provision.c
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2022 LooUQ Incorporated.
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
 *************************************************************************** */

#define _DEBUG 2                        // set to non-zero value for PRINTF debugging output, 
// debugging output options             // LTEm1c will satisfy PRINTF references with empty definition if not already resolved
#if _DEBUG > 0
    asm(".global _printf_float");       // forces build to link in float support for printf
    #if _DEBUG == 1
    #define SERIAL_DBG 1                // enable serial port output using devl host platform serial, 1=wait for port
    #elif _DEBUG == 2
    #include <jlinkRtt.h>               // output debug PRINTF macros to J-Link RTT channel
    #define PRINTF(c_,f_,__VA_ARGS__...) do { rtt_printf(c_, (f_), ## __VA_ARGS__); } while(0)
    #endif
#else
#define PRINTF(c_, f_, ...) ;
#endif

#define SRCFILE "PRO"                           // create SRCFILE (3 char) MACRO for lq-diagnostics ASSERT
#include "lqc-provision.h"


#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))


char *pProvBffr;
static void httpRecvCB(dataCntxt_t dataCntxt, uint16_t httpStatus, char *recvData, uint16_t dataSz);

bool lqcProvision_readLQCCnfig(const char *hostUrl, const char *deviceId, const char *validation, lqcDeviceConfig_t * deviceConfig)
{
    /* This uses a local (stack) control\buffer for HTTP, also the application recv buffer (provBuf) is local here and referenced by ptr.
     * This allows the minimum RAM object lifetime.

    /* 00^CMDevl^SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2F864508030074113&sig=aEPPyLTUKMj29HBGfrwZ20uPzGOxDlW1XwnmtEKaLvc%3D&se=1788816293^088410a1
       f^kkk^CMDevl^SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2F864508030074113&sig=aEPPyLTUKMj29HBGfrwZ20uPzGOxDlW1XwnmtEKaLvc%3D&se=1788816293^088410a1

        f = flags: bit map of options
        f:1 = cacheable value (can be stored locally and reused)
        kkk = packageKey (number as char string)

        Packages are limited to 253 bytes in length
     */
    httpCtrl_t httpCtrl;
    char httpCtrlBffr[512];
    char provBffr[512];
    char cstmHeaders[80];
    char packageKey[4] = {0};
    char lqcToken[200] = {0};
    pProvBffr = &provBffr;

    http_initControl(&httpCtrl, dataCntxt_5, httpCtrlBffr, sizeof(httpCtrlBffr), httpRecvCB);
    http_setConnection(&httpCtrl, hostUrl, 0);
    http_enableCustomHdrs(&httpCtrl, cstmHeaders, sizeof(cstmHeaders));

    // Get LQC connection configs from provisioning service
    // HARD CODED for LQC provisioning (WebAPI)
    memset(provBffr, 0, sizeof(provBffr));
    snprintf(cstmHeaders, sizeof(cstmHeaders), "lqc-dvc-val: %s\r\n", validation);             // add req'd validation header
    //http_addCustomHdr(&httpCtrl, cstmHeaders);

    memset(deviceConfig, 0, sizeof(lqcDeviceConfig_t));
    char relativeUrl[60] = "/provisioning/config/";
    strcat(relativeUrl, deviceId);

    #define FIELD_SEP '^'
    #define CHECK_SEP '~'

    uint8_t tries = 0;
    bool provisioningSuccess = false;
    bool retryFailure = true;
    char *pNext = NULL;
    char *pStop = NULL;

    do // retry block
    {
        do // sequence block
        {
            tries++;
            resultCode_t rslt = http_get(&httpCtrl, relativeUrl, false, 20);
            if (rslt != resultCode__success)
            {
                if (rslt != resultCode__timeout)
                    retryFailure = false;
                break; // exit sequence block
            }

            rslt = http_readPage(&httpCtrl, 20);
            if (rslt != resultCode__success)
                break;

            if (!STRNCMP(provBffr, LQC_PROVISIONING_MAGICFLAG, 4) ||
                memchr(provBffr, FIELD_SEP, sizeof(provBffr)) == NULL ||
                memchr(provBffr, CHECK_SEP, sizeof(provBffr)) == NULL)
                break;                                                          // exit block with prov success false

            memcpy(deviceConfig->magicFlag, provBffr , 4);                      // grab package identification
            memcpy(deviceConfig->packageId, provBffr + 5, 4);
            strncpy(deviceConfig->deviceId, deviceId, lqc__identity_deviceIdSz);

            /* this function only fetches LQC configs */
            if (!STRCMP(deviceConfig->magicFlag, "LQCP") ||
                !STRCMP(deviceConfig->packageId, "LQCC"))
                break;

            /* parse provisioning service response into device config
             */
            pNext = pProvBffr + 10;                                             // package version (variable width)past package id
            pNext = strchr(pNext, FIELD_SEP) + 1;                               // device label
            pStop = strchr(pNext, FIELD_SEP);
            if (pStop - pNext > lqc__identity_deviceLabelSz) 
                break;
            memcpy(deviceConfig->deviceLabel, pNext, pStop - pNext);

            pNext = pStop + 1;                                                  // host URL
            pStop = strchr(pNext, FIELD_SEP);
            if (pStop - pNext > lqc__identity_hostUrlSz) 
                break;
            memcpy(deviceConfig->hostUrl, pNext, pStop - pNext);

            pStop++;                                                            // host port
            deviceConfig->hostPort = strtol(pStop, &pNext, 10);

            pNext++;                                                            // signature
            pStop = strchr(pNext, CHECK_SEP);
            if (pStop - pNext > lqc__identity_signatureSz)
                break;
            memcpy(deviceConfig->signature, pNext, pStop - pNext);

            pStop[0] = '\0';                                                    // remove recv'd CRC from provBuf scope for CRC calc
            pNext = ++pStop;
            if (strlen(pNext) >= 8)                                                                
            {
                uint32_t provCrc = strtoul(pNext, &pStop, 16);
                uint32_t crcHash = crc32(provBffr);  
                provisioningSuccess = (provCrc == crcHash);                     // recv'd CRC matches locally calc'd

                pStop[0] = '\0';
                PRINTF(dbgColor__dMagenta, "CRC rHex=%s, provCrc=%lu lclCrc=%lu\r", pNext, provCrc, crcHash);
            }
        } while (!provisioningSuccess);
    }
    while (!provisioningSuccess && retryFailure && tries < 2);

    return provisioningSuccess;
}


static void httpRecvCB(dataCntxt_t dataCntxt, uint16_t httpStatus, char *recvData, uint16_t dataSz)
{
    PRINTF(dbgColor__dMagenta, "ProvisionCB %d new chars\r", dataSz);
    uint16_t prevRcvd = strlen(pProvBffr);
    strncpy(pProvBffr + prevRcvd, recvData, dataSz);
}


/*  This is the basic CRC-32 calculation with some optimization but no table lookup. The the byte 
 *  reversal is avoided by shifting the crc reg right instead of left and by using a reversed 
 *  32-bit word to represent the polynomial.
 *
 *  http://www.hackersdelight.org/hdcodetxt/crc.c.txt
 */

unsigned int crc32(unsigned char *message) {
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (message[i] != 0) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;                     // reverse bit order (see above)
}


