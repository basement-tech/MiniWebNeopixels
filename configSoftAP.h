#ifndef __CONFIGSOFTAP_H__
  
#define AP_SSID   "NeopixelMagic"  // SoftAP SSID
#define AP_PASSWD "123456789"     // SoftAP Password ... must be long-ish for ssid to be advertised

#define AP_LOCAL_IP  192, 168, 4, 1        // Custom IP Address
#define AP_GATEWAY   192, 168, 4, 1         // Gateway
#define AP_SUBNET    255, 255, 255, 0       // Subnet Mask

#define AP_JS_NAME   "config.html"          // file containing configuration page (mostly) javascript

void configSoftAP(void);

#define __CONFIGSOFTAP_H__
#endif