/*
 * MechWarriorsWebNeopixels
 *
 * brief: define and control the playing of a sequence on a string of neopixels
 * using buttons on a web-enabled application.
 * 
 * this application is based on the webServer example below and snippets from
 * the teststrand neopixel example, provided by adafruit.
 *
 * sequence definition and playout is custom added code, as is the
 * html/js/c to put up the button webpage and handle button input.
 *
 * note that files must be uploaded to the littleFS embedded file system
 * to enable full functionality.
 * 
 * libraries used:
 * - ArduinoJson by Benoit Blanchon v7.3.0 see documentation:
 *   https://arduinojson.org/v7/tutorial/deserialization/
 * - Adafruit NeoPixel by Adafruit v1.12.3
 * - ArduinoOTA by Arduino, Juraj Andrasy v1.1.0
 * - Arduino_DebugUtils by Arduino v1.4.0
 * - webserver came as an example with esp8266 board package
 *
 * Default Pin Assignments(see app_pins.h):
 * Pin      Function                #define 
 * 0        BOOT REQ                reserved
 * 2       OB Blue LED              reserved
 * 4        i2C SDA                 future - not yet implemented
 * 5        i2c SCL                 future - not yet implemented
 * 12        configure
 * 13        UNUSED
 * 14        UNUSED
 * 15       neo data                NEO_PIN 
 * 16        UNUSED
 * 
 *
 * Some notes regarding how the webserver works 
 * (see the README.md for some things regarding the webserver example):
 * 
 * The webserver application provides the web server itself, and some built in 
 * functionality:
 * - webserver
 * - LittleFS for storing files on a flash-disk (persists through firmware flashing)
 *    (https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html)
 *   NOTE: the file system is located in the flash memory.
 *         the board type specification says how much of the flash is reserved for the fs
 *         seems to be reserved whether it's being used or not
 *         littleFS is said to do wear balancing (just as the name implies)
 * - built in functions for uploading files (maybe for deleting)
 *   (note: files are overwritten by files of the same name: useful for development)
 * If not specific URL is sent to the sebserver the implementation looks for a file
 * named index.htm.  That's the mechanism that I used for the button page (i.e. uploaded
 * html/js for the page in a file called index.htm)
 *
 * There are options for setting the segment sizes reserved for program, littlefs, etc.
 * They are under the tools->Flash Size menu in the Arduino IDE.  The options available
 * are described in (which apparently has to be modified using the tool boards.txt.py):
 * C:\Users\djzma\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.1.2\boards.txt
 *
 * Some major discoveries:
 *
 * DEBUG_DEBUG() and similar have an upper limit of the size of the
 * string that they can display ... something like 2-3K bytes.
 *
 * How to get a value from an htlm button to a c language callback ?
 * major learning: existence of server.arg("plain")

 * Regarding data flow from the button (html) -> js -> c callback:
 * html puts up buttons with an onclick: directive; that calls the 
 * javascript to make a POST to the server.  The button has an value= that
 * identifies which specific button was pressed, while calling the same js callback
 * for all buttons.  i.e. the identification of the button is sent via the 'this' argument
 * to the callback in the button html line.
 * 
 * javascript can see the value of the button as o.button, where o is the passed in argument.
 * it converts it to a json object, attaching the tag "sequence".  This json string becomes the 
 * body of the fetch/POST request to the server.
 *
 * the server makes the body of the POST available to ...
 *
 * index.html .........................................MechWarriorsWebNeopixels.ino
 * html/button .............callCfunction() ...........handleButton() 
 * html button "value" -> js o.value -> POST body -> server.arg("plain")
 *
 *
 * How to parse/deserialize a simple json string?
 * major learning: overloading and function of jsonDoc[]
 * see this in handleButton():
 *  const char *seq;
 *  JsonDocument jsonDoc;
 *  DeserializationError err;
 *  ...
 *  err = deserializeJson(jsonDoc, buf);
 *  if(err)  {
 *    TRACE("Deserialization of button failed: %s\n", err.f_str());
 *  }
 *  else  {
 *    TRACE("json parsing successful, extracting value\n");
 *    seq = jsonDoc["sequence"];
 *
 * jsonDoc[] is overloaded with a finite list of return types.
 * that's why const char *seq was necessary.
 *
 * 
 * How to open and read a file using littleFS ?
 * major learning: once the file is open, the read functions are provided
 *                 by the "file descriptor"-like class functions
 * see this in neo_load_sequence():
 *  TRACE("Loading filename %s ...\n", label);
 *  if((fd = LittleFS.open(label, "r")) != 0)  {
 *    while(fd.available())  {
 *      *pbuf++ = fd.read();
 *      ...
 * 
 * How to deserialize a json string with two dimensional array ?
 * done, see code.  Was made easier by properly formatting the json.
 *
 * don't know yet ... working it out in neo_load_sequence()
 *
 * Last couple of neopixels were flickering during playback?
 * - fixed: two potential contributing factors: I was clearing
 *   the strand of neopixels before writing each new value ... probably
 *   the root cause.  I also updated the neopixel library.
 * -seems eliminating the clear all pixels before writing the new
 *  value seemed to eliminate it.  Although might have just been that
 *  the update got faster ?
 *
 *
 * TODO (x = done):
 * o figure out why the ntp/time stuff doesn't work
 * x slowp: add r, g, b to bonus string to set flicker up
 * x do I need to add something to get the esp8266 to remember the last successful WiFi connection ?
 *   - apparently not
 * o allow use of sequences based on user files as default
 * x change fallback wifi credentials on initial failure ?
 *   - set secrets.h to guest (done)
 * x figure out how the DELETE works and implement it
 * x single shot sequences using strategy attribute
 * x more built in sequences
 * x more different strategies for interpretting json sequence files and playout them out
 * o chase, x pong, x rainbow, x slowfade (color, intensity range, speed)
 * x how to set debug levels and clean up use of TRACE()
 * o how can I publicise the ip address when using DHCP
 *   and use http://hostname or http://hostname.local
 * x add eeprom parm for enable/disable DHCP
 * x how many connections can be made to the server at a time?
 *   default is apparently 5
 * x Is keep-alive being used ?  After sitting, sometimes takes 10 seconds to respond
 *   see comments near webserver instantiation in this file
 * x download and install esp8266 littleFS plugin and reduce size of filesystem
 *   -> can be set through Tools->Flash Size selection (with limitations)
 * x should a timer be added to drive frequency of neopixel strand updates (2 mS )
 * o look at how the number of USER sequences can be dynamically done (malloc-ish)
 *   or write to a single "USER" space in the sequence array and use another means
 *   to determine if the sequence has changed on button press.
 * x enable OTA firmware update.  Here are some key points:
 *    Key steps:
 *    Prepare your code:
 *    Include the "ArduinoOTA" library in your Arduino sketch. 
 *    Define your WiFi network credentials (SSID and password). 
 *    Set up the ArduinoOTA.begin() function within your setup() function to initialize the OTA process. 
 *    Add the ArduinoOTA.handle() function in your loop() function to check for incoming updates and handle the update process. 
 *    Upload initial firmware:
 *    Compile and upload your sketch with the OTA functionality to your ESP8266 using the Arduino IDE. 
 *    Access the update interface:
 *    Once connected to your network, find the IP address of your ESP8266. 
 *    Open a web browser and navigate to the IP address to access the OTA update interface (if your code is set up to provide one). 
 *    Send the new firmware:
 *    Select the new firmware file you want to upload. 
 *    Initiate the update process through the web interface. 
 *
 *    Uploading Firmware OTA
 *      Make sure your ESP is connected to the same network.
 *      Open Arduino IDE → Tools → Port.
 *      Select the ESP's network address (e.g., esp8266-XXXX.local).
 *      Make sure firewall settings are appropriate
 *      Click Upload → The new firmware will be uploaded via OTA!
 *      (if prompted for credentials and none are configured, put any characters in the field)
 * 
 *
 * (c) Daniel J. Zimmerman  Jan 2025
 *
 */

// @file WebServer.ino
// @brief Example implementation using the ESP8266 WebServer.
//
// See also README.md for instructions and hints.
//
// Changelog:
// 21.07.2021 creation, first version

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Arduino_DebugUtils.h>

#include "secrets.h"  // add WLAN Credentials in here.

#include <FS.h>        // File System for Web Server Files
#include <LittleFS.h>  // This file system is used.
#include <ArduinoOTA.h>  // Over-the-air updates
#include <ESP8266TimerInterrupt.h>  // neopixel timer

#include "bt_eepromlib.h"
#include "neo_data.h"  // for neopixels
#include "app_pins.h"
#include "configSoftAP.h"

// mark parameters not used in example
#define UNUSED __attribute__((unused))

// TRACE output simplified, can be deactivated here ... replaced with Arduino debug library
//#define TRACE(...) Serial.printf(__VA_ARGS__)

// name of the server. You reach it using http://webserver
#define HOSTNAME "warhammer"

// local time zone definition (US Central Daylight Time)
#define TIMEZONE "CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00"

// get access to the eeprom based configuration structure
net_config *pmon_config = get_mon_config_ptr();

#ifdef CONFIG_SERVER
/*
 * part of figuring out why, after sitting a while, the first button press
 * takes 5-10 seconds to work.  Although, clicking in the address line
 * of the browser seems to speed it up ???
 * (the request to the server is always, eventually honored ... hmmm ?)
 * 
 * Update: seems to have been helped by calling the neopixel update
 * on a timer so that the webserver update call gets more time.
 *
 * was trying to follow this ... not sure it's for this implementation:
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html#_CPPv414httpd_config_t
 * 
 * Configure the HTTP server ... this code doesn't compile; for reference only
 * default max_open_sockets is apparently 4
 * keep-alive: Defaults to true when the client's HTTP version is 1.1 or above, otherwise it defaults to false.
 */
HTTPD_DEFAULT_CONFIG();

httpd_config_t config = { 
  .max_open_sockets = 5,  //default
  .server_port = 80;
  .keep_alive_enable = true;
  .keep_alive_idle = 5;  //default
  .keep_alive_interval = 5; // default
  .keep_alive_count = 3; // default
};
// Start the server
ESP8266WebServer server(config);

#else
/*
 * need a WebServer for http access on port 80.
 */
ESP8266WebServer server(80);

#endif

// The text of builtin files are in this header file
#include "builtinfiles.h"

// ===== Simple functions used to answer simple GET requests =====

// This function is called when the WebServer was requested without giving a filename.
// This will redirect to the file index.htm when it is existing otherwise to the built-in $upload.htm page
//
// this is used to display the main page after having uploaded a index.htm page with buttons
void handleRedirect() {
  DEBUG_INFO("Redirect...\n");
  String url = "/index.htm";

  if (!LittleFS.exists(url)) { url = "/$update.htm"; }

  server.sendHeader("Location", url, true);
  server.send(302);  // send "found redirection" return code
}  // handleRedirect()


// This function is called when the WebServer was requested to list all existing files in the filesystem.
// a JSON array with file information is returned.
void handleListFiles() {
  Dir dir = LittleFS.openDir("/");
  String result;

  result += "[\n";
  while (dir.next()) {
    if (result.length() > 4) { result += ","; }
    result += "  {";
    result += " \"name\": \"" + dir.fileName() + "\", ";
    result += " \"size\": " + String(dir.fileSize()) + ", ";
    result += " \"time\": " + String(dir.fileTime());
    result += " }\n";
    // jc.addProperty("size", dir.fileSize());
  }  // while
  result += "]";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleListFiles()


// This function is called when the sysInfo service was requested.
void handleSysInfo() {
  String result;

  FSInfo fs_info;
  LittleFS.info(fs_info);

  result += "{\n";
  result += "  \"flashSize\": " + String(ESP.getFlashChipSize()) + ",\n";
  result += "  \"freeHeap\": " + String(ESP.getFreeHeap()) + ",\n";
  result += "  \"fsTotalBytes\": " + String(fs_info.totalBytes) + ",\n";
  result += "  \"fsUsedBytes\": " + String(fs_info.usedBytes) + ",\n";
  result += "  \"Chip ID\": " + String(ESP.getChipId()) + ",\n";
  result += "  \"CPU Frequency\": " + String(ESP.getCpuFreqMHz()) + "MHz" + ",\n";
  result += "  \"firmware version\": " + String(EEPROM_VALID) + ",\n";
  result += "}";

  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleSysInfo()

// This function is called when the netInfo service was requested.
// ... added this to see if I could extend the built in functions ... worked (and useful)
void handleNetInfo() {
  String result;
  long rssi = 0;

  result += "{ NETWORK STATUS, \n";
  result += "  {";
    if(WiFi.status() == WL_CONNECTED) result += " \"status \": connected,\n";
    else result += " \"status \": disconnected,\n";  // not sure how this will ever be sent
    result += "    \"ip_address\":" + WiFi.localIP().toString() + ",\n";
    result += "    \"WiFi RSSI\":" + String(WiFi.RSSI()) + ",\n";
    result += "  }\n";
  result += "}\n";

  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}  // handleNetInfo()

//
// handle button presses from the index.htm file
// - all buttons on the default page call this same function based
// - on being registered with a server.on(api/button) below
// - the javascript in index.htm constructs and sends a json string ("sequence" : "value"),
//   as the body of the POST,
//   to identify the identity of the specific button being pressed.
// - non-sequence, special purpose "value"'s are intercepted and processed
//   by this function, otherwise the value is sent to neo_set_sequence()
//   to do what the function name says
// 
void handleButton()  {
  int8_t neoerr = NEO_SUCCESS;
  char buf[128];
  char *file = NULL;
  const char *seq, *filename;
  JsonDocument jsonDoc;
  DeserializationError err;

  if(server.method() == HTTP_POST)  {
    /*
     * get the value of the button pressed
     */
    String body = server.arg("plain");
    DEBUG_DEBUG("Button pressed: ");
    body.toCharArray(buf, sizeof(buf));
    DEBUG_DEBUG("return buffer <%s>\n", buf);

    /*
     * parse the json to get to just the sequence label
     * NOTE: this code is very sensitive to types of variables
     * used in extracting values after parsing.  
     * e.g. const char *seq; was specifically required to get the
     * jsonDoc["sequence"] to build, apparently due to the overloading
     * that's built into this function (i.e. adding the const specifier).
     */

    err = deserializeJson(jsonDoc, buf);
    if(err)  {
      DEBUG_ERROR("ERROR: Deserialization of button failed: %s\n", err.f_str());
      neoerr = NEO_DESERR;
    }
    else  {
      DEBUG_DEBUG("json parsing successful, extracting value\n");
      seq = jsonDoc["sequence"];
      /*
       * process the button that was pressed based on the seq string
       */
      if(seq != NULL)  {
        DEBUG_INFO("Setting sequence to %s\n", seq);

        /*
         * was it the stop button
         */
        if(strcmp(seq, "STOP") == 0)
          neo_cycle_stop();

        /*
         * if not STOP, see if it was a USER defined sequence
         * if so, load the file and set the sequence and strategy
         */
        else if((neo_is_user(seq)) == NEO_SUCCESS)  {
          if((neoerr = neo_load_sequence(jsonDoc["file"])) != NEO_SUCCESS)
            DEBUG_ERROR("ERROR: Error loading sequence file after proper detection\n");
        }

        /*
         * if not STOP or USER-x, then attempt to set the sequence,
         * assuming that it's a pre-defined button.
         * strategies are hardcoded for built in sequences.
         */
        else  {
          if((neoerr = neo_set_sequence(seq, "")) != NEO_SUCCESS)
            DEBUG_ERROR("ERROR: Error setting sequence after proper detection\n");
        }
      }
      else  {
        neoerr = NEO_NOPLACE;
        DEBUG_ERROR("ERROR: \"sequence\" not found in json data\n");
      }
    }
    if(neoerr != NEO_SUCCESS)
      server.send(404, "text/plain", "handleButton(): Couldn't process button press");
    else  {
      // Set the response header with "Connection: keep-alive" 
      server.sendHeader("Connection", "keep-alive");
      server.send(201, "text/plain", "handleButton(): success");
    }
  }
  else
    server.send(405, "text/plain", "handleButton(): Method Not Allowed");
}

// ===== Request Handler class used to answer more complex requests =====

// The FileServerHandler is registered to the web server to support DELETE and UPLOAD of files into the filesystem.
class FileServerHandler : public RequestHandler {
public:
  // @brief Construct a new File Server Handler object
  // @param fs The file system to be used.
  // @param path Path to the root folder in the file system that is used for serving static data down and upload.
  // @param cache_header Cache Header to be used in replies.
  FileServerHandler() {
    DEBUG_INFO("FileServerHandler is registered\n");
  }


  // @brief check incoming request. Can handle POST for uploads and DELETE.
  // @param requestMethod method of the http request line.
  // @param requestUri request ressource from the http request line.
  // @return true when method can be handled.
  bool canHandle(HTTPMethod requestMethod, const String UNUSED &_uri) override {
    return ((requestMethod == HTTP_POST) || (requestMethod == HTTP_DELETE));
  }  // canHandle()


  bool canUpload(const String &uri) override {
    // only allow upload on root fs level.
    return (uri == "/");
  }  // canUpload()


  bool handle(ESP8266WebServer &server, HTTPMethod requestMethod, const String &requestUri) override {
    // ensure that filename starts with '/'
    String fName = requestUri;
    if (!fName.startsWith("/")) { fName = "/" + fName; }

    if (requestMethod == HTTP_POST) {
      // all done in upload. no other forms.

    } else if (requestMethod == HTTP_DELETE) {
      if (LittleFS.exists(fName)) { 
        LittleFS.remove(fName);
        DEBUG_INFO("handle: %s deleted successfully\n", fName.c_str());
      }
    }  // if

    server.send(200);  // all done.
    return (true);
  }  // handle()


  // uploading process
  void upload(ESP8266WebServer UNUSED &server, const String UNUSED &_requestUri, HTTPUpload &upload) override {
    // ensure that filename starts with '/'
    String fName = upload.filename;
    if (!fName.startsWith("/")) { fName = "/" + fName; }

    if (upload.status == UPLOAD_FILE_START) {
      // Open the file
      if (LittleFS.exists(fName)) { LittleFS.remove(fName); }  // if
      _fsUploadFile = LittleFS.open(fName, "w");

    } else if (upload.status == UPLOAD_FILE_WRITE) {
      // Write received bytes
      if (_fsUploadFile) { _fsUploadFile.write(upload.buf, upload.currentSize); }

    } else if (upload.status == UPLOAD_FILE_END) {
      // Close the file
      if (_fsUploadFile) { _fsUploadFile.close(); }
    }  // if
  }    // upload()

protected:
  File _fsUploadFile;
};

// ^^^^^^^^^^^^^^^^^^^ END OF SERVER SETUP AND CALLBACKS ^^^^^^^^^^^^^^^^^^^^^^^^

/*
 * set up the global parameters and timer callback for neopixel
 * service function
 */
// Select a Timer Clock (note: this is acted upon by some #defines in the class definition file)
#define USING_TIM_DIV1                false           // for shortest and most accurate timer  (80MHz)
#define USING_TIM_DIV16               true            // for medium time and medium accurate timer (5 MHz)
#define USING_TIM_DIV256              false           // for longest timer but least accurate.  (312.5 KHz)

ESP8266Timer ITimer;   // Init ESP8266 timer 1
volatile bool neo_timer_active = false;
void IRAM_ATTR neoTimerHandler(void) {
  neo_timer_active = true;
}


/*
 *  *****************  SETUP  ******************
 */

/*
 * little helper to wait for the wifi connection
 */
wl_status_t l_wifi_wait(void)  {
  int8_t tries = -1;

  DEBUG_INFO("Connecting to WiFi...\n");
  if((tries = atoi(pmon_config->wifitries)) < 0)
    tries = 10;  // default to 10 ... seems to take about 6 on my local network
  while ((WiFi.status() != WL_CONNECTED) && (tries > 0)) {
    DEBUG_INFO("%d ", tries);
    delay(500);
    tries--;
  }
  DEBUG_INFO("\n");

  if(WiFi.status() == WL_CONNECTED)
    DEBUG_INFO("connected at %s\n", WiFi.localIP().toString().c_str());
  else
    DEBUG_ERROR("ERROR: Error connecting WiFi\n");
  
  return(WiFi.status());
}


void setup(void) {
  delay(3000);  // wait for serial monitor to start completely.

  // Use Serial port for some trace information from the example
  Serial.begin(115200);
  //Serial.setDebugOutput(false);  // TODO: what is this ???

  /*
   * set the debug message level:
   * DBG_NONE - no debug output is shown
   * DBG_ERROR - critical errors
   * DBG_WARNING - non-critical errors
   * DBG_INFO - information
   * DBG_DEBUG - more information
   * DBG_VERBOSE - most information
   * NOTE: these map to integers -1 to 4 ... a little hacky, but
   *       use the epprom value in the same way
   */
  Debug.setDebugLevel(DBG_VERBOSE);  // bootstrap here; set when eeprom is connected
  Debug.newlineOff();
  int debug_level = DBG_VERBOSE;  // for eeprom read


  /*
   * initialize the EEPROM for basic bootstrapping of application
   * (e.g. wifi credentials)
   *
   * NOTE: don't use the Arduino debug library for this part
   */
  eeprom_begin();  // instantiate the eeprom class

  Serial.println();
  Serial.println(EEPROM_INTRO_MSG);
  Serial.println();
  Serial.println("Press any key to change settings");

  int8_t tries = 6;
  bool out = false;
  char inChar = '\0';
  while((out == false) && (tries > 0))  {
    Serial.print(tries);Serial.print(" . ");
  // check for incoming serial data:
    if (Serial.available() > 0) {
      // read incoming serial data:
      inChar = Serial.read();
      out = true;
    }
    delay(500);
    tries--;
  }
  Serial.println();

  /*
   * prompt the user for input if the out bool was set above
   * (if not, this function just loads the current contents of the eeprom)
   */
  eeprom_user_input(out);


  /*
   * mount and/or reformat the littleFS
   * wait to do this until after eeprom work so that
   * we know whether to format the fs.
   */
  FSInfo fs_info;

  if(strcmp(pmon_config->reformat, "true") == 0)  {
    DEBUG_INFO("Formatting the filesystem...\n");
    if (!LittleFS.format())
      DEBUG_ERROR("ERROR: Could not format the filesystem...\n");
    else
      DEBUG_INFO("Format successful\n");
  }
  delay(500);
  DEBUG_INFO("Mounting the filesystem...\n");
  if (!LittleFS.begin())
    DEBUG_ERROR("ERROR: Could not mount the filesystem...\n");
  else  {
    LittleFS.info(fs_info);
    DEBUG_INFO("Mount successful\n");
    DEBUG_INFO("fsTotalBytes: %d\n", fs_info.totalBytes);
    DEBUG_INFO("fsUsedBytes: %d\n", fs_info.usedBytes);
  }
  delay(500);

  /*
   * if the config physical button was pressed on power-up,
   * instantiate and start the local soft AP to facilitate configuration
   * from the captive page.  The esp is reset when this function exits.
   */
  pinMode(PIN_CONFIG, INPUT_PULLUP);
  delay(1000);  // seems to be required to set it get to true before reading
  Serial.println("Press config physical button to start configuration SoftAP");
  if(digitalRead(PIN_CONFIG) == 0)
    configSoftAP();

  /*
   * once all of the eeprom setup is done,
   * set the debug level (see above for other comments)
   */
  debug_level = atoi(pmon_config->debug_level);
  if(debug_level < -1) debug_level = DBG_NONE;
  if(debug_level > 4)  debug_level = DBG_VERBOSE;
  Debug.setDebugLevel(debug_level);
  DEBUG_INFO("Debug level set to %d\n", Debug.getDebugLevel());

  DEBUG_INFO("Starting WebServer ...\n");



  /*
   * setup wifi to use a fixed IP address
   * attempt to convert the ip address from the eeprom, and
   * if that fails load a default address
   */
  uint8_t ip[4]; // ip address as octets

  /*
   * if eeprom config indicates that DHCP is disabled,
   * set the fixed ip address
   */
  DEBUG_INFO("DHCP Enable = <%s>\n", pmon_config->dhcp_enable);
  if(strcmp(pmon_config->dhcp_enable, "false") == 0)  {
    DEBUG_INFO("... setting fixed address\n");
    if(eeprom_convert_ip(pmon_config->ipaddr, ip) != 0)  {
      DEBUG_ERROR("ERROR: Failed to convert eeprom IP address value ... loading default\n");
      ip[0] = 192; ip[1] = 168; ip[2] = 1; ip[3] = 37;
    }
    const byte gateway[] = {192, 168, 1, 1}; // Gateway address
    const byte subnet[] = {255, 255, 255, 0}; // Subnet mask
    WiFi.config(ip, gateway, subnet); // Set static IP ... DZ added this
  }

  /*
   * start WiFI ... DHCP by default, unless above is executed
   */
  WiFi.mode(WIFI_STA);

  /*
   * allow to address the device by the given name e.g. http://webserver
   * saw a note in the reference manual that this had to happen before the WIFI.begin()
   * doesn't seem to matter, but I caved to conventional wisdom.
   */
  WiFi.setHostname(HOSTNAME);

  /*
   * attempt these three cases to connect to WiFi:
   *
   * case 1:
   *   if the wifi credentials were set in eeprom, attempt to use them.
   * case 2:
   *   attempt to connect to using the last good wifi credentials
   *   that the esp stores in non-volatile memory
   * case 3:
   *   use the credentials set in secrets.h as the fallback
   */
  wl_status_t wifi_status = WL_DISCONNECTED;
  if((strlen(pmon_config->wlan_ssid) > 0) && (strlen(pmon_config->wlan_pass) > 0))  {
    DEBUG_INFO("Wifi.begin() is using eeprom values, ssid = %s\n", pmon_config->wlan_ssid);
    WiFi.begin(pmon_config->wlan_ssid, pmon_config->wlan_pass);
    wifi_status = l_wifi_wait();
  }

  if (wifi_status != WL_CONNECTED) {
    DEBUG_WARNING("WARNING: Wifi.begin() is using the last known wifi (stored in nonvolatile memory)\n");
    WiFi.begin();
    wifi_status = l_wifi_wait();
  }

  if(wifi_status != WL_CONNECTED) {
    DEBUG_WARNING("WARNING: Wifi.begin() is using fallback values from secrets.h, ssid = %s\n", ssid);
    WiFi.begin(ssid, passPhrase);
    l_wifi_wait();
  }


  /*
  * OTA callbacks (from ArduinoOTA example)
  */
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
// end of OTA callbacks
  ArduinoOTA.begin();


  // Ask for the current time using NTP request builtin into ESP firmware.
  DEBUG_INFO("Setup ntp...\n");
  if(strlen(pmon_config->tz_offset_gmt) > 0)
    configTime(pmon_config->tz_offset_gmt, "pool.ntp.org");
  else
    configTime(TIMEZONE, "pool.ntp.org");

  DEBUG_INFO("Register service handlers...\n");

  // serve a built-in htm page
  server.on("/$upload.htm", []() {
    server.send(200, "text/html", FPSTR(uploadContent));
  });

  /*
   * LittleFS based file delete code
   * 
   * if the url starts is http://xxx.xxx.xxx.xxx/$delete and it's a
   * HTTP_GET method, then send the html/js and it will be executed
   * by the browser given the Content-Type specified as the second
   * argument to the server.send() (seems that HTTP_GET is the default
   * if one types something in the address line of a browser)
   * (see extensive comments in builtinfiles.h)
   */
  server.on("/$delete", HTTP_GET, []() {
      server.send(200, "text/html", FPSTR(deleteContent));
  });

  // register a redirect handler when only domain name is given.
  server.on("/", HTTP_GET, handleRedirect);

  //
  // register some REST services
  // NOTE: the server attempts to find a match with the URL's registered here,
  // before falling back to the server.addHandler(new FileServerHandler()) below
  // (e.g. "/api/button" intercepts POST requests for buttons ... POSTS for
  // file operations are passed along)
  //
  server.on("/$list", HTTP_GET, handleListFiles);
  server.on("/$sysinfo", HTTP_GET, handleSysInfo);
  server.on("/$netinfo", HTTP_GET, handleNetInfo);
  server.on("/api/button", HTTP_POST, handleButton);

  // UPLOAD and DELETE of files in the file system using a request handler.
  server.addHandler(new FileServerHandler());

  // enable CORS header in webserver results
  server.enableCORS(true);

  // enable ETAG header in webserver results from serveStatic handler
  server.enableETag(true);

  // serve all static files
  server.serveStatic("/", LittleFS, "/");

  // handle cases when file is not found
  server.onNotFound([]() {
    // standard not found in browser.
    server.send(404, "text/html", FPSTR(notFoundContent));
  });

  server.begin();
  DEBUG_INFO("hostname=%s\n", WiFi.getHostname());

  // initialize neopixel strip
  DEBUG_INFO("Initialize neopixel strip with %d pixels...\n", atoi(pmon_config->neocount));
  if(atoi(pmon_config->neocount) > 0)
    neo_init(atoi(pmon_config->neocount), NEO_PIN, NEO_TYPE);
  else
    neo_init(NEO_NUMPIXELS, NEO_PIN, NEO_TYPE);

  DEBUG_INFO("Setting gamma correction to %s\n", pmon_config->neogamma);
  if(strcmp(pmon_config->neogamma, "true") == 0)
    neo_set_gamma_color(true);
  else
    neo_set_gamma_color(false);

  /*
   * give a visual indicator of WiFi connection status
   */
  if(WiFi.status() == WL_CONNECTED)
    neo_n_blinks(0, 128, 0, 3, 500);  // three green blinks ... NOTE: blocking function
  else
    neo_n_blinks(128, 0, 0, 3, 500);  // three red blinks ... NOTE: blocking function

  /*
   * start the default sequence from eeprom setting
   */
  if((strcmp(pmon_config->neodefault, "none") != 0) && (strlen(pmon_config->neodefault) > 0))  {
    if(neo_set_sequence(pmon_config->neodefault, "") != NEO_SUCCESS)
      DEBUG_ERROR("ERROR: Error setting default sequence %s\n", pmon_config->neodefault);
  }

  /*
   * set up the timer for the neopixel service routime
   */
#if !defined(ESP8266)
  #error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif
  if (ITimer.attachInterruptInterval(NEO_UPDATE_INTERVAL, neoTimerHandler))
    DEBUG_DEBUG("Setup: neopixel timer setup successful\n");
  else
        DEBUG_ERROR("ERROR: Setup: error: neopixel timer setup failed\n");

  /*
   * set up a pin for debugging
   */
  if(DEBUG_PIN >= 0)  {
    pinMode(DEBUG_PIN, OUTPUT);
    digitalWrite(DEBUG_PIN, 0);
  }

}  // setup


// run the server...
void loop(void) {


  server.handleClient(); // webserver requests
  ArduinoOTA.handle();   // over-the-air firmware updates

  /*
   * checking whether updates to the neopixel array
   * are needed are on a timer that sets  neo_timer_active
   *
   * With the debug pin configured as shown, the following timings
   * were observed:
   * no update: ~10uS pulse width every 2 mS
   * (apparent) update running rainbow : ~1.75 mS pulse width every other call to neo_cycle_next()
   * (apparent) update running slowp: ~1.5 mS pulse width every other call to neo_cycle_next()
   *
   */
  if(neo_timer_active)  {
#if DEBUG_PIN >= 0
    digitalWrite(DEBUG_PIN, true);
#endif
    neo_cycle_next();      // neopixel updates
    neo_timer_active = false;
#if DEBUG_PIN >= 0
    digitalWrite(DEBUG_PIN, false);
#endif
  }
}  // loop()

// end.
