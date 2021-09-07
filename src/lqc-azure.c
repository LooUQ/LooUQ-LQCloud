
#include "lqc-internal.h"
#include "lqc-azure.h"


void lqc_composeTokenSas(char *tokenSAS, uint8_t sasSz, const char* hostUri, const char *deviceId, const char* sigExpiry)
{
    ASSERT(strlen(hostUri) + strlen(deviceId) + strlen(sigExpiry) + 40 < sasSz, srcfile_azure_c);
        
    /* Azure SAS Token Template
    * %s = host URI
    * %s = device ID
    * %s = signature (and expiry)
    "SharedAccessSignature sr=%s%2Fdevices%2F%s&sig=%s"      // cannot use snprintf because of escaped chars in template */

    strcpy(tokenSAS, "SharedAccessSignature sr=");
    strcat(tokenSAS, hostUri);
    strcat(tokenSAS, "%2Fdevices%2F");
    strcat(tokenSAS, deviceId);
    strcat(tokenSAS, "&");
    strcat(tokenSAS, sigExpiry);
}


lqcDeviceConfig_t lqc_decomposeTokenSas(const char* tokenSas)
{
    ASSERT(strncmp(tokenSas, "SharedAccessSignature ", 22) == 0, srcfile_azure_c);
    ASSERT(tokenSas[22] == 's' && tokenSas[24] == '=' && strlen(tokenSas) < 180, srcfile_azure_c);

    lqcDeviceConfig_t dvcCnfg = {0};
    char *start = (char *)tokenSas + 25;
    char *end = NULL;

    dvcCnfg.magicFlag = LOOUQ_MAGIC;
    dvcCnfg.pageKey = LOOUQ_FLASHDICTKEY__LQCDEVICE;

    end = strstr(start, "%2Fdevices");
    memcpy(dvcCnfg.hostUri, start, end - start);

    start = end + 13;
    end = strstr(start, "&sig=");
    memcpy(dvcCnfg.deviceId, start, end - start);

    start = end + 1;
    end = start + strlen(start);
    memcpy(dvcCnfg.tokenSigExpiry, start, end - start);

    return dvcCnfg;
}

