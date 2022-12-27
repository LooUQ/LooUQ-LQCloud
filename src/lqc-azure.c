
#define SRCFILE "AZR"                           // create SRCFILE (3 char) MACRO for lq-diagnostics ASSERT
#include "lqc-internal.h"
#include "lqc-azure.h"


void lqc_composeIothUserId(char *userId, uint8_t uidSz, const char* hostUrl, const char *deviceId)
{
    // "<hostUrl>/<deviceId>/?api-version=2021-04-12"
    ASSERT(strlen(hostUrl) + strlen(deviceId) + 25 < uidSz);

    strcpy(userId, hostUrl);
    strcat(userId, "/");
    strcat(userId, deviceId);
    strcat(userId, "/?api-version=2021-04-12");
}


void lqc_composeIothSasToken(char *sasToken, uint8_t sasSz, const char* hostUrl, const char *deviceId, const char* signature)
{
    // "SharedAccessSignature sr=<hostUrl>%2Fdevices%2F<deviceId>&sig=<signature+expiry>"
    ASSERT(strlen(hostUrl) + strlen(deviceId) + strlen(signature) + 40 < sasSz);
        
    strcpy(sasToken, "SharedAccessSignature sr=");
    strcat(sasToken, hostUrl);
    strcat(sasToken, "%2Fdevices%2F");
    strcat(sasToken, deviceId);
    strcat(sasToken, "&");
    strcat(sasToken, signature);
}


void lqc_setDeviceConfigFromSASToken(const char* sasToken, lqcDeviceConfig_t * deviceConfig)
{
    ASSERT(strncmp(sasToken, "SharedAccessSignature ", 22) == 0);
    ASSERT(sasToken[22] == 's' && sasToken[24] == '=' && strlen(sasToken) < 180);

    char *start = (char *)sasToken + 25;
    char *end = strstr(start, "%2Fdevices");
    if (end != NULL)
    {
        memcpy(deviceConfig->hostUrl, start, end - start);

        start = end + 13;
        end = strstr(start, "&sig=");
        if (end != NULL)
        {
            memcpy(deviceConfig->deviceId, start, end - start);

            start = end + 1;
            end = start + strlen(start);
            memcpy(deviceConfig->signature, start, end - start);

            memcpy(deviceConfig->magicFlag, LQC_PROVISIONING_MAGICFLAG, strlen(LQC_PROVISIONING_MAGICFLAG));
            memcpy(deviceConfig->packageId, LQC_DEVICECONFIG_PACKAGEID, strlen(LQC_DEVICECONFIG_PACKAGEID));
            deviceConfig->hostPort = 8883;
        }
    }
}

