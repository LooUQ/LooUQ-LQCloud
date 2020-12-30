/******************************************************************************
 * Device Profile\Settings Header
 *****************************************************************************/

#ifndef __DVCSETTINS_H__
#define __DVCSETTINS_H__

#include <lqCloud.h>


/* Example SAS tokens
HostName=iothub-dev-pelogical.azure-devices.net;DeviceId=e8fdd7df-2ca2-4b64-95bc-031c6b199299;SharedAccessSignature=SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2Fe8fdd7df-2ca2-4b64-95bc-031c6b199299&sig=ZGr8hjqBMcFHmMDc%2Bgjo3gFAaoiRazEnLTMBtsDWNpI%3D&se=1607176494
HostName=iothub-dev-pelogical.azure-devices.net;DeviceId=e8fdd7df-2ca2-4b64-95bc-031c6b199299;SharedAccessSignature=SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2Fe8fdd7df-2ca2-4b64-95bc-031c6b199299&sig=mRksB7BM%2Bmya6P3fkoeHFVnSL%2FHSNrTIZAL8AEZ6gJA%3D&se=1610056256
HostName=iothub-dev-pelogical.azure-devices.net;DeviceId=e8fdd7df-2ca2-4b64-95bc-031c6b199299;SharedAccessSignature=SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2Fe8fdd7df-2ca2-4b64-95bc-031c6b199299&sig=3LOLoFZJcfM6a5EbH7Qhzg0LLu8PMorxjjSibTwvdgs%3D&se=4487176329
*/

// devl: "iothub-dev-pelogical.azure-devices.net"
// prod: "iothub-a-prod-loouq.azure-devices.net"

// #define DEVICESHORTNAME "CloudTstD"
// #define LQCLOUD_URL "iothub-dev-pelogical.azure-devices.net"
// #define LQCLOUD_SASTOKEN ""
// #define LQCLOUD_APPLKEY "notDefined"


#define DEVICESHORTNAME "CloudTstP"
#define LQCLOUD_URL "iothub-a-prod-loouq.azure-devices.net"
#define LQCLOUD_SASTOKEN "SharedAccessSignature sr=iothub-a-prod-loouq.azure-devices.net%2Fdevices%2F867198053226845&sig=%2F58Qbuld6EX5PCJkidWVuKeq75vrjZGKbmmsa00uhQ8%3D&se=1611768392"
#define LQCLOUD_APPLKEY "notDefined"


#endif  /* !__DVCSETTINS_H__ */
