#define    DEBUG_EN     1    // Set 1 to enable debug printing

// Use Variadic Macros to accept a variable number of arguments 
// When the macro is invoked, all the tokens in its argument list 
// after the last named argument, including any commas, become the variable argument.
#if DEBUG_EN
#define    DEBUG(...)        Serial.print(__VA_ARGS__)
#define    DEBUGLN(...)      Serial.println(__VA_ARGS__)
#else
#define    DEBUG(...)
#define    DEBUGLN(...)
#endif


// ------------------------ Network ---------------------

#define    WIFI_SSID            "@ZHANGSU_2.4G"                   
#define    WIFI_PASS            "12345678"                   

#if STATIC_EN

uint8_t    DEFAULT_IP[]         {192, 168, 6, 100}     
uint8_t    DEFAULT_GW[]         {192, 168, 6, 1} 
uint8_t    DEFAULT_NM[]         {255, 255, 255, 0}  
uint8_t    DEFAULT_D1[]         {192, 168, 6, 100}
uint8_t    DEFAULT_D2[]         {1, 1, 1, 1}  

#endif

bool        wifi_connected =    false;                             // true - connection to the wifi network is done  
const char* wifi_ssid =         WIFI_SSID;
const char* wifi_pass =         WIFI_PASS;

// ------------------------ SERVER  ---------------------


#define    NTP_SERVER         "pool.ntp.org"                   // Visit https://timetoolsltd.com/information/public-ntp-server/
#define    WEATHER_SOURCE     "api.openweathermap.org"         // Weather API
#define    WEATHER_API_KEY    "************"                   // API key, confidential


// --------------- MQTT topic definition Alpha -----------------

const char* sub_topic   = "565621061/ESP/#";
const char* sub_setled  = "565621061/ESP/setle";
const char* sub_alarm_t = "565621061/ESP/alarm";
const char* sub_timer   = "565621061/ESP/timer";
const char* sub_light   = "565621061/ESP/light";
const char* sub_motor   = "565621061/ESP/motor";
const char* sub_mled    = "565621061/ESP/mled";
const char* sub_alarm_h = "565621061/ESP/alarh";
const char* sub_sync    = "565621061/ESP/sync";
const char* sub_ac      = "565621061/ESP/accom"; 
  

const char* pub_topic   = "565621061/Android";
const char* pub_state_topic = "565621061/state";
const char* pub_ding_topic  = "565621061/ding";
const char* pub_wm_topic    = "565621061/wm";
const char* willTopic       = "565621061/ESP/status";
const char* willMessage     = "turn off";

// ----------------------- MQTT parameters --------------------

WiFiClient mqtt_client;                                 // Object for working with remote hosts - connection to the MQTT server
PubSubClient mqtt(mqtt_client);                         // MQTT server connection object

String mqtt_client_id =      String (random16(), HEX);  // Unique id to register the client with the MQTT server, does not allow duplicate!


#define   MQTT_SERVER        "192.168.6.248"            // MQTT server address
#define   MQTT_USER          ""                         // 
#define   MQTT_PASS          ""                         // 
#define   MQTT_PORT          1883                       // 
#define   MQTT_PREFIX        "565621061"                // user prefix            
#define   MQTT_WILL_QOS      0                          // 
#define   MQTT_WILL_RETAIN   1   

#define   SUB_PREFIX          "ESP"                     // Topic prefix for subscribed topics
#define   SUB_TOPIC_LED       "led"                     // Topic - receive led control command from the app
#define   SUB_TOPIC_RLY       "rly"                     // Topic - receive power relay control command from the app
#define   SUB_TOPIC_ALM       "alm"                     // Topic - receive target values for certain sensors (etc temperature|humidity)
#define   SUB_TOPIC_TME       "tme"                     // Topic - receive the timer value from the app
#define   SUB_TOPIC_NLT       "nlt"                     // Topic - receive messages from the app about turning the nignt light on / off
#define   SUB_TOPIC_MOT       "mot"                     // Topic - receive messages from the app about controlling the water pump motor
#define   SUB_TOPIC_SYN       "syn"                     // Topic - receive messages to force sync data between the app and master
#define   SUB_TOPIC_IRC       "irc"                     // Topic - receive IR control command from the app

const char* sub_topics[] = [SUB_TOPIC_LED, SUB_TOPIC_RLY, 
                            SUB_TOPIC_ALM, SUB_TOPIC_TME, 
                            SUB_TOPIC_NLT, SUB_TOPIC_MOT, 
                            SUB_TOPIC_SYN, SUB_TOPIC_IRC];

#define   PUB_TOPIC_DTA       "dta"                     // Topic - send the recorded sensor data to the app
#define   PUB_TOPIC_STE       "ste"                     // Topic - send certain MCU's state to the app
#define   PUB_TOPIC_NTI       "nti"                     // Topic - send floating notification to the app
#define   PUB_TOPIC_WIL       "wil"                     // Topic - send mqtt will message to the app (about turning the device on / off)
#define   PUB_TOPIC_ERR       "err"                     // Topic - send error message to the app

const char* pub_topics[] = [PUB_TOPIC_DTA, PUB_TOPIC_STE, 
                            PUB_TOPIC_NTI, PUB_TOPIC_WIL,
                            PUB_TOPIC_ERR];

#define   MQTT_CONNECT_TIMEOUT   1000                   // Timeout for trying to connect to MQTT server
#define   MQTT_RECONNECT_PERIOD  60000                  // The time between reconnect due to MQTT server timeout


char      mqtt_server[25] = "";                        // MQTT server name
char      mqtt_user[15]   = "";                        // Login from the server
char      mqtt_pass[15]   = "";                        // Password from the server
char      mqtt_prefix[31] = "";                        // Message topic prefix
uint16_t  mqtt_port = MQTT_PORT;                        // Port to connect to the MQTT server


// Allocate space for an array of commands coming from the MQTT server
// Callback to the command from the MQTT server occurs asynchronously, and if the previous
// the command has not been processed yet - a new command handler call occurs, which is not reentrant -
// this causes the application to crash. To avoid this, incoming commands will be queued
// and execute them in the main program loop
#define   QSIZE_IN  8                          // command queue size from MQTT
#define   QSIZE_OUT  96                        // MQTT outgoing message queue size
String    cmpQueue[QSIZE_IN];                  // Ring buffer queue of received commands from MQTT (topic)
String    cmdQueue[QSIZE_IN];                  // Ring buffer queue of received commands from MQTT (message)
String    tpcQueue[QSIZE_OUT];                 // Ring buffer queue for sending commands to MQTT (topic)
String    msgQueue[QSIZE_OUT];                 // Ring buffer queue for sending commands to MQTT (message)
bool      rtnQueue[QSIZE_OUT];                 // Ring buffer queue for sending commands to MQTT (retain)
uint8_t   queueWriteIdx = 0;                  // entry position in the queue for processing received commands
uint8_t   queueReadIdx = 0;                   // read position from the queue for processing received commands
uint8_t   queueLength = 0;                    // number of commands in the queue for processing received commands
uint8_t   outQueueWriteIdx = 0;               // entry position in the queue for sending MQTT messages
uint8_t   outQueueReadIdx = 0;                // read position from the queue for sending MQTT messages
uint8_t   outQueueLength = 0;                 // number of commands in the queue for sending MQTT messages


bool      mqtt_connected = false;            // Connecting to MQTT (not yet established)
bool      mqtt_connecting = false;           // Connecting to MQTT (not yet established)
bool      mqtt_topic_subscribed = false;     // Subscribed to the command topic completed
uint8_t   mqtt_conn_cnt = 0;                 // Count of connection attempts to format the output
uint32_t  mqtt_conn_last;                     // Time of the last attempt to connect to the MQTT server
uint32_t  uptime_send_last;                   // Time of the last uptime sent to the MQTT server initiated by the device
uint32_t  nextMqttConnectTime;                // If the MQTT server is unavailable, determines the time of the next connection attempt

#define   BUF_MQTT_SIZE   384                 // maximum allocated buffer size for incoming messages via MQTT channel
char      incomeMqttBuffer[BUF_MQTT_SIZE];    // Buffer for receiving command string from MQTT


// ------------------------ Misc timer ---------------------

#define   DEFAULT_MOTOR_TIME          6000                        // Default motor running time: 6 sec
uint16_t  time_motor_set;                                         // Motor running time(ms)
uint32_t  time_motor_last = 0;                                    // Time of the last motor start time

uint32_t  time_data_last = 0;
uint32_t  time_ntp_last  = 0; 
uint32_t  time_sensor_last  = 0; 
uint32_t  time_detect_last  = 0;

uint32_t  TimePro; 
uint32_t  timer = 0;
bool      flag_timer = false;


// **************** NTP Parameters *********************

uint16_t    SYNC_TIME_PERIOD = 60 ;               // Default sync period in seconds

const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;
struct tm   timeinfo;

uint8_t     ntp_cnt = 0 ;                       // Counter of attempts to receive data from the server
bool        ntp_status = true ;                 // Flag true - it's time to synchronize the clock with the NTP server

char        ntpServer[31] = NTP_SERVER ;        // NTP server to use

uint_8_t    time_hour  =  0;
uint_8_t    time_min   =  0;
uint_8_t    time_sec   =  0;
uint_8_t    time_day   =  0;
uint_8_t    time_month =  0;
uint_16_t   time_year  =  0; 
long        time_number = 0;


// ------------------------ Bluetooth Mesh Command Sheet-----------------

const char head1[4] PROGMEM = {0xAA, 0xFB, 0x00, 0x11};
const char no11[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF1, 0x01};
const char no10[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF1, 0x00};
const char no21[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF2, 0x01};
const char no20[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF2, 0x00};
const char no31[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF3, 0x01};
const char no30[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF3, 0x00};
const char no41[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF4, 0x01};
const char no40[7]  PROGMEM = {0xAA, 0xFC, 0x00, 0x33, 0xE7, 0xF4, 0x00};


// ------------------------ IR Data Sheet-----------------

const char ir_command_01[3] PROGMEM = {0xE7, 0xF1, 0x01};
const char ir_command_00[3] PROGMEM = {0xE7, 0xF1, 0x00};
const char ir_command_11[3] PROGMEM = {0xE7, 0xF2, 0x01};
const char ir_command_10[3] PROGMEM = {0xE7, 0xF2, 0x00};
