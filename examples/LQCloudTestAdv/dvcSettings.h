/******************************************************************************
 * Device Profile\Settings Header
 * 
 * LooUQ's LQCloud requires a small amount of configuration per device, this 
 * includes information to authenticate the device to the LQCloud individually.
 * Each device attaching to LQCloud has unique identity and authentication 
 * (security) values.
 * 
 * This configuration info can be hard-coded into the application or retrieved
 * from a provisioning service. Hard-coding present significant challenges 
 * beyone a couple devices. LooUQ recommends a provisioning service to allow
 * for the device to retrieve its configuration. This process is secured
 * using information you add to your device application build, which is 
 * common for all your devices. 
 * 
 * LooUQ provides an open-source Azure Cloud project creating a basic 
 * provisioning service. Alternatively, you can subscribe to LQCloud and use
 * the provisioning service provided without requiring you to setup one of 
 * your own.
 *****************************************************************************/

#ifndef __DVCSETTINS_H__
#define __DVCSETTINS_H__


/* Example Connection Strings >>> SAS tokens
// Obtain from LQCloud.Manage|Devices
HostName=iothub-dev-pelogical.azure-devices.net;DeviceId=e8fdd7df-2ca2-4b64-95bc-031c6b199299;SharedAccessSignature=SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2Fe8fdd7df-2ca2-4b64-95bc-031c6b199299&sig=3LOLoFZJcfM6a5EbH7Qhzg0LLu8PMorxjjSibTwvdgs%3D&se=4487176329
                                                                                                                    ^                                                                                                                ^-- signature part (sasSig)
                                                                                                                    ^-- SAS token part -------------------------------------------------------------------------------------------------------------------------------------------------------------^
LQCloud IoTHub: "iothub-a-prod-loouq.azure-devices.net"
Note: If you are not using the application key functionality in your application, set APPLKEY to an empty string ""
*/

/*
 * Set your device label and SAS information to get started. 
 * Notes:
 *  - DVC_LABEL is a local (aka device) value only.
 *  - You can use network hardware (IMEI, MACaddress, etc.) for your device identifiers.
 *  - You can use SASTOKEN or URL + DVC_ID + SASSIG. SASTOKEN encodes all of the later list.
 *      - There are lqc_composeSasToken() and lqc_decomposeSasToken() functions in the azureSasToken.c file to go between token and components.
 * 
*/
// Examples from LQCdev
#define DVC_ID "867198053224766"
#define DVC_LABEL "LQCdev"
#define LQCLOUD_URL "iothub-dev-pelogical.azure-devices.net"
#define LQCLOUD_SASTOKEN "SharedAccessSignature sr=iothub-dev-pelogical.azure-devices.net%2Fdevices%2F867198053224766&sig=8F6EWCSm4Kc1CILQA6FrrmITjIVQvVC8a8DOapo9W1M%3D&se=2871120500"
#define LQCLOUD_SASSIG "sig=8F6EWCSN6Kc1CILQA6FrrmITjIVQvVC8a8DOapo9W1M%3D&se=2871120500"
#define LQCLOUD_APPLKEY ""

#endif  /* !__DVCSETTINS_H__ */
