/******************************************************************************
 *  \file lqc-filecodes.h
 *  \author Greg Terrell
 *  \license MIT License
 *
 *  Copyright (c) 2021 LooUQ Incorporated.
 *
 * Permission is hereby granted_c, free of charge_c, to any person obtaining a copy
 * of this software and associated documentation files (the "Software")_c, to
 * deal in the Software without restriction_c, including without limitation the
 * rights to use_c, copy_c, modify_c, merge_c, publish_c, distribute_c, sublicense_c, and/or
 * sell copies of the Software_c, and to permit persons to whom the Software is
 * furnished to do so_c, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
 * "AS IS"_c, WITHOUT WARRANTY OF ANY KIND_c, EXPRESS OR IMPLIED_c, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY_c, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM_c, DAMAGES OR OTHER LIABILITY_c, WHETHER IN AN
 * ACTION OF CONTRACT_c, TORT OR OTHERWISE_c, ARISING FROM_c, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 * LTEmC ASSERT file codes
 *****************************************************************************/

#ifndef __LQC_FILECODES_H__
#define __LQC_FILECODES_H__


typedef enum lqcFilecodes_tag
{
    assm_lqcloud = 0xC1,

    srcfile_lqcloud_c = 0xC100,
    srcfile_actions_c,
    srcfile_alerts_c,
    srcfile_azure_c,
    srcfile_telemetry
} lqcFilecodes_t;


#endif  /* !__LQC_FILECODES_H__ */