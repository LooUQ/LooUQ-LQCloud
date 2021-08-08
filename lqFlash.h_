/******************************************************************************
 *  \file lqFlash.h
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2021 LooUQ Incorporated.
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
 * LQCloud application flash support
 *****************************************************************************/

#ifndef __LQFLASH_H__
#define __LQFLASH_H__

#include <stdint.h>
#include <stdbool.h>

#include <Adafruit_SPIFlash.h>          // Adafruit SPI Flash extensions

#define LQFLASH_INFOBLK_CNT     16
#define LQFLASH_INFOBLK_SZ      512
#define LQFLASH_INFOBLK_MAGIC   0xa6a7100c


#ifdef __cplusplus
extern "C"
{
#endif

void RegisterFlash(Adafruit_SPIFlash flash);

//bool checkSendDiagnostics(lqDiagResetCause_t rcause, lqDiagInfo_t *diagInfo);
uint8_t findInfoBlkId(uint8_t infoBlkId);
uint8_t findInfoBlkAvailableSlot();

#ifdef __cplusplus
}
#endif // !__cplusplus



#endif  /* !__LQFLASH_H__ */
