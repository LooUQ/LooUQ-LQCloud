#ifndef __LQCLOUD_HTTP_H__
#define __LQCLOUD_HTTP_H__

#include "lqc-internal.h"

#ifdef __cplusplus
extern "C"
{
#endif

bool LQC_connectHttp();
lqcSendResult_t LQC_sendHttp(const char *topic, const char *body, bool queueOnFail, uint8_t timeoutSeconds);


#ifdef __cplusplus
}
#endif // !__cplusplus

#endif  /* !__LQCLOUD_HTTP_H__ */
