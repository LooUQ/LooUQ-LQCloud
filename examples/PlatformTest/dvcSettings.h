/******************************************************************************
 * Device Profile\Settings Header
 *****************************************************************************/

#ifndef __DVCSETTINS_H__
#define __DVCSETTINS_H__

/* Example Connection Strings >>> SAS tokens
// Obtain from LQCloud.Manage|Devices
HostName=iothub-dev-pelogical.azure-devices.net;DeviceId=e8fdd7df-2ca2-4b64-95bc-031c6b199299;SharedAccessSignature=SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2Fe8fdd7df-2ca2-4b64-95bc-031c6b199299&sig=3LOLoFZJcfM6a5EbH7Qhzg0LLu8PMorxjjSibTwvdgs%3D&se=4487176329
                                                                                                                    ^-- SAS token part -------------------------------------------------------------------------------------------------------------------------------------------------------------^
LQCloud IoTHub: "iothub-a-prod-loouq.azure-devices.net"
Note: Leave APPLKEY as an empty string if not using
*/

// Change the shortname and SAS token to your values to get started. Note:
// Note: DEVICESHORTNAME is currently a local build only value. LQCloud auto-provisioning will supply from LQCloud.Manage device settings in early 2021 using your device 
// network hardware (IMEI, MACaddress, etc.) and (organizational) APPLKEY as your device identifiers.

//myDevice
#define DEVICESHORTNAME "myDevice"
#define LQCLOUD_URL "iothub-a-prod-loouq.azure-devices.net"
#define LQCLOUD_SASTOKEN "SharedAccessSignature sr=iothub-a-prod-loouq.azure-devices.net%2Fdevices%2F866208053226845&sig=%2F58Qbuld6EX5PCJkidWVuKeq75vrjZGKbmmsa00uiM2%3D&se=1611768392"
#define LQCLOUD_APPLKEY ""

#endif  /* !__DVCSETTINS_H__ */
