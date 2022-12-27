/******************************************************************************
 *  \file lqcloud-ntwk.h
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
 * The lqcloud-ntwk.h header defines the physical communications implmentation
 * layer to be created for a physical communications approach.
 * 
 * lqcloud-ntwk-ltemc.c is the physical implementation of the LooUQ LTEmC dvc 
 *****************************************************************************/
#ifndef __LQCLOUD_NTWK_H__
#define __LQCLOUD_NTWK_H__

//#include <lq-types.h>           // LooUQ libraries LQCloud uses
#include <lqcloud.h>
//#include <lq-network.h>



// typedef struct lqcNtwkInfo_tag
// {
//     char network[10 + 1];
//     char type[3 + 1];
// } lqcNtwkInfo_t;


#ifdef __cplusplus
extern "C"
{
#endif


/* Communications Interface (ex: modem)
 * --------------------------------------------------------------------------------------------- */
//void lqcNtwk_createIntf(yield_func yieldCallback, appEventCallback_func appEventCallback);  /// create and initialize modem data instance

uint8_t lqcNtwk_powerIntfOn(resetAction_t resetAction);
void lqcNtwk_powerIntfOff();
void lqcNtwk_resetIntf(resetAction_t resetAction);

modemInfo_t *lqcNtwk_readIntfInfo();                                                        /// get information about modem
uint8_t lqcNtwk_getIntfState();                                                             /// return modem off,on,appRdy

void lqcNtwk_doWork();                                                                      /// background worker function


/* Provider (cellular network)
 * --------------------------------------------------------------------------------------------- */
providerInfo_t *lqcNtwk_awaitProvider(uint16_t waitDurSeconds);

/** @brief Get the signal strength as RSSI */
int16_t lqcNtwk_signalRSSI();

// /** @brief Get the signal strength, as a percent of maximum signal (0 - 100) */
// uint8_t lqcNtwk_readSignalPercent();
// /** @brief Get the signal strength, as a bar count for visualizations, (like on a smartphone) */
// uint8_t lqcNtwk_readSignalBars(uint8_t displayBarCount);


/* Packet Network (PDP)
 * Usually IPv4 but could be IPv6, PPP, etc.
 * --------------------------------------------------------------------------------------------- */
networkInfo_t *lqcNtwk_activateNetwork(uint8_t pdpContextId);
void lqcNtwk_deactivateNetwork(uint8_t pdpContextId);
networkInfo_t *lqcNtwk_readNetworkInfo(uint8_t pdpContextId);


#ifdef __cplusplus
}
#endif // !__cplusplus

#endif  /* !__LQCLOUD_NTWK_H__ */
