/******************************************************************************
 *  \file lqcloud.h
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
 ******************************************************************************
 * Master header for LooUQ Cloud (C99) Client
 *****************************************************************************/

#ifndef __LQCLOUD_PROVISIONING_H__
#define __LQCLOUD_PROVISIONING_H__

#include <lq-types.h>           // LooUQ libraries LQCloud uses
#include <ltemc.h>              // *** TODO remove these dependencies ***
#include <ltemc-http.h>         // provisioning is over HTTPS
#include <lqcloud.h>


#ifdef __cplusplus
extern "C"
{
#endif

bool lqcProvision_readLQCCnfig(const char *hostUrl, const char *imei, const char *validationKey, lqcDeviceConfig_t *deviceConfig);
unsigned int crc32(unsigned char *message);


#ifdef __cplusplus
}
#endif // !__cplusplus

#endif  /* !__LQCLOUD_PROVISIONING_H__ */
