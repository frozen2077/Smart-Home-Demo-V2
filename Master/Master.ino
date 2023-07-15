#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <EEPROM.h>
#include <PubSubClient.h>                     // Library can only publish QoS 0 messages, but can subscribe to QoS 0 or QoS 1.
#include <SoftwareSerial.h>
#include <time.h>
#include <Wire.h>
#include <WiFiUdp.h>


#include  "a_def_hard.h"                     // Hardware definition file
#include  "a_def_soft.h"                     // Software definition file

#include  "images.h"                         // Image files in array

bool relay_en;
bool motor_en;
bool detect;


String TimeShoww;
String timers;

bool str_complete = false;
bool acmode = true;

int hazardgas;
int lux;

byte buff[2];



float alarm_t;
float alarm_h;
float temp;
float humi;
float humi_soil;
bool  flag_alarm_t = false;
bool  flag_alarm_h = false;
uint16_t addr_alarm_t;
uint16_t addr_alarm_h;


void mqttTask(void* parameter) {
  for (;;) {    
    mqttMonitor();
    processMqttQueue();
    mqtt.loop();
  }
}

void timeTask(void* parameter) {
  for (;;) {    
    ntpMonitor();
    timerCtl();    
  }
}

void netTask(void* parameter) {
  for (;;) {    
    wifiMonitor();
    ArduinoOTA.handle();  
  }
}

void serialTask(void* parameter) {
  for (;;) {    
    serialEventBt(); 
  } 
}

void dataTask(void* parameter) {
  for (;;) {    
    alarmValueMonitor();
    personDetectMonitor();
    luxReader(); 
    pumpCtl();
    displayOled();
    PubMQTT(PUB_TOPIC_DTA, dataFeedBack().c_str()); 
  }
}


void setup() {

// Pin Mode setup ------------------------------------------

  pinMode(0, INPUT);                                        // Pin 0  - Temperature & Humidity sensor
  pinMode(12, INPUT);                                       // Pin 12 - Motion detection
  pinMode(14, OUTPUT);                                      // Pin 14 - Power relay switch
  pinMode(15, OUTPUT);                                      // Pin 15 - Buzzer control
  pinMode(16, OUTPUT);                                      // Pin 16 - H-bridge Motor control

  digitalWrite(14, 0);
  digitalWrite(15, 0);  
  digitalWrite(16, 0);

  analogWriteFreq(10000);

// UART & USART setup --------------------------------------

  Serial.begin(115200);
  Serial1.begin(115200);

// DHT22 Sensor setup --------------------------------------

  dht.begin();

// I2C setup -----------------------------------------------

  Wire.begin();

// EEPROM setup --------------------------------------------

  EEPROM.begin(100);
  alarm_t = String(EEPROM.read(0));
  alarm_h = String(EEPROM.read(1));

// OLED Display setup -------------------------------------- 

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 

// WiFi connection setup -----------------------------------  

  if (wifi_ssid.length() > 0) {
    DEBUG(F("\n Wifi configured to "));
    DEBUGLN(wifi_ssid);
    if (STATIC_EN) {
      if (DEFAULT_IP[0] + DEFAULT_IP[1] + DEFAULT_IP[2] + DEFAULT_IP[3] > 0 ) {
        WiFi.config(IPAddress(DEFAULT_IP[0], DEFAULT_IP[1], DEFAULT_IP[2], DEFAULT_IP[3]),         // Static IP    192.168.6.100
                    IPAddress(DEFAULT_GW[0], DEFAULT_GW[1], DEFAULT_GW[2], DEFAULT_GW[3]),         // Gateway      192.168.6.1
                    IPAddress(DEFAULT_NM[0], DEFAULT_NM[1], DEFAULT_NM[2], DEFAULT_NM[3]),         // Subnet Mask  255.255.255.0
                    IPAddress(DEFAULT_D1[0], DEFAULT_D1[1], DEFAULT_D1[2], DEFAULT_D1[3]),         // D1           192.168.6.1
                    IPAddress(DEFAULT_D2[0], DEFAULT_D2[1], DEFAULT_D2[2], DEFAULT_D2[3])          // D2           1.1.1.1  
                    )                                                                                              
        DEBUGLN("Using static ip -> %d.%d.%d.%d", DEFAULT_IP[0], DEFAULT_IP[1], DEFAULT_IP[2], DEFAULT_IP[3]);               
      } 
      else DEBUGLN(F("Staic IP empty, using DHCP")); 
    }  
  else DEBUGLN(F("Using DHCP"));   
  delay(10);                                        // 10ms delay to prevent crash
  WiFi.mode(WIFI_STA);

// MQTT connection setup -----------------------------------

  uint32_t t = millis();
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);
  mqtt.setSocketTimeout(1);
  mqttConnectionMonitor();
  if (millis() - t > MQTT_CONNECT_TIMEOUT) {
    nextMqttConnectTime = millis() + MQTT_RECONNECT_PERIOD;
    DEBUGLN(F("\n MQTT server connection failed.'"));
    DEBUGLN(mqtt.state());
    DEBUGLN("Attempt again in 1 minutes");       
  }

// ArduinoOTA setup -----------------------------------
  
  //ArduinoOTA.setPort(8266);                                                           // Port defaults to 8266
  //ArduinoOTA.setHostname("myesp8266");                                                // Hostname defaults to esp8266-[ChipID]
  //ArduinoOTA.setPassword((const char*)"12345678");                                    // No authentication by default
  ArduinoOTA.onStart([]() {
    DEBUGLN("Start");
  });
  ArduinoOTA.onEnd([]() {
    DEBUGLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUGLN("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUGLN("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)         DEBUGLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   DEBUGLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUGLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUGLN("Receive Failed");
    else if (error == OTA_END_ERROR)     DEBUGLN("End Failed");
  });
  ArduinoOTA.begin();

// NTP setup -----------------------------------

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  ntpMonitor();

//----------------------------------------------

  TaskHandle_t BtMeshHandle;
  TaskHandle_t MqttHandle;
  TaskHandle_t TimeHandle;
  TaskHandle_t DataHandle;
  TaskHandle_t NetHandle;        

  xTaskCreatePinnedToCore(
    serialTask,
    "BtMeshHandle",                    // A name just for humans
    128,                               // This stack size can be checked & adjusted by reading the Stack Highwater
    NULL,                              // Parameters for the task
    3,                                 // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    &BtMeshHandle,                     // Task handle
    0                                  // CPU core assigned to the task
  );                                  

  xTaskCreatePinnedToCore(
    mqttTask,
    "MqttHandle",                    
    128,                              
    NULL,                             
    2,                                
    &MqttHandle,
    0
  );        

  xTaskCreatePinnedToCore(
    timeTask,
    "TimeHandle",                    
    128,                              
    NULL,                             
    1,                                
    &TimeHandle,
    0
  );     

  xTaskCreatePinnedToCore(
    dataTask,
    "DataHandle",                    
    128,                              
    NULL,                             
    1,                                
    &DataHandle,
    0
  );     

   xTaskCreatePinnedToCore(
    netTask,
    "NetHandle",                    
    128,                              
    NULL,                             
    3,                                
    &NetHandle,
    0
  );                                    

}



void ntpMonitor() {

  if (WiFi.status() != WL_CONNECTED) return;

  if (millis() - time_ntp_last > SYNC_TIME_PERIOD * 1000 || time_ntp_last == 0) {
    if (ntp_status == true || millis() - time_ntp_last > 300000) {
      bool ok = getLocalTime(&timeinfo);
      if (ok) {
        ntp_status = true;
        ntp_cnt = 0;
        time_ntp_last = millis();
        time_hour  = timeinfo.tm_hour;
        time_min   = timeinfo.tm_min;
        time_sec   = timeinfo.tm_sec;
        time_day   = timeinfo.tm_mday;
        time_month = timeinfo.tm_mon + 1;
        time_year  = timeinfo.tm_year + 1900;  
        time_number = atol(&timeinfo, "%Y%m%d%H%M%S");      
        DEBUGLN(&timeinfo, "%A, %B %d %Y %H:%M:%S");       
      }
      else {
        ntp_cnt++;
        DEBUGLN(F("NTP connection timeout!"));
        if (ntp_cnt >= 5 ) {
          ntp_status = false;  
          ntp_cnt = 0;   
          time_ntp_last = millis();     
          DEBUGLN (F("Failed to connect to NTP server."));  

          DynamicJsonDocument doc(256);
          string out;
          doc["act"] = F("TIME");
          doc["server_name"] = ntpServer;
          doc["result"] = F("FAILED");
          serializeJson (doc, out);      
          PubMQTT(PUB_TOPIC_ERR, out);          
        }  
      } 
    }
  }   
  DEBUGLN();             
}



void wifiMonitor(uint32_t waitTime) {

 if (WiFi.status() != WL_CONNECTED) {
    
    wifi_connected = false;

    WiFi.begin(wifi_ssid, wifi_pass);

    uint32_t  start_wifi_check = millis();
    uint32_t  last_wifi_check = 0 ;
    int16_t   cnt = 0 ;
    while (!wifi_connected) {
      delay(1);
      if (millis() - last_wifi_check > 250) {
        last_wifi_check = millis();
        wifi_connected = WiFi.status() == WL_CONNECTED;
        if (wifi_connected) {
          // Connection established
          wifi_connected = true;
          DEBUGLN();
          DEBUG(F("WiFi connected. IP address: "));
          DEBUGLN(wifi.localIP());
          break;
        }
        if (cnt % 50 == 0) {
          DEBUGLN();
        }
        DEBUG(".");
        cnt++;
      }
      if (millis() - start_wifi_check > waitTime) {
        // Network connection timed out
        DEBUGLN(F("Failed to connect to WiFi network."));
        break ;
      }
      delay(1);
    }
    DEBUGLN();
 }
}

void callback(char* topic, byte* payload, uint32_t length) {
  // check if we received data from the topic we need
  bool ok = false;
  uint8_t cmp;
  uint8_t sub_topics_len = sizeof(sub_topics)/sizeof(sub_topics[0]);
  DEBUG ("MQTT << topic= '" + String(topic) + "'");
  for (uint8_t i = 0; i < sub_topics_len; i++) {
    if (strcmp(topic, mqtt_topic_sub(sub_topics[i]).c_str()) == 0) {
      ok = true; cmp = i + 1;
      break;
    }  
  }
  if (ok) {
    memset(incomeMqttBuffer, 0, BUF_MQTT_SIZE);
    memcpy(incomeMqttBuffer, payload, length);
    DEBUG ("; msg='"));
    DEBUG (incomeMqttBuffer);
    DEBUG ("'");  
    String cmd = String(incomeMqttBuffer); 
    display_msg = cmd;
    if (cmd.length() > 0 && queueLength < QSIZE_IN) {
      queueLength++;
      cmpQueue[queueWriteIdx++] = cmp;      
      cmdQueue[queueWriteIdx++] = cmd;
      if (queueWriteIdx >= QSIZE_IN) queueWriteIdx = 0;
    }   
  }
  else {
    notifyUnknownCmd("Topic: " + topic + " | Msg:" + payload);
  }
  DEBUGLN ();
}


void processMqttQueue() {

  if (queueLength > 0) {    

    // Topic and content of the sent message
    uinit8_t topic_id = cmpQueue[queueReadIdx];
    String command = cmdQueue[queueReadIdx];

    if (topic_id && command.length() > 0) {
      switch (topic_id) {

        // 1 SUB_TOPIC_LED - Receive led control command from the app and send it to Slave
        case 1:
          uint8_t len = sizeof(head1)/sizeof(head1[0]);
          for (uint8_t i = 0; i < len; i++) {
            Serial1.print(head1[i]);
            delay(1);                                                // Delay 1ms to prevent from sending failure
          }
          Serial1.print(command);

          DEBUGLN("Sent LED message to Slave: %s", command);
          break;

        // 2 SUB_TOPIC_RLY - Power relay control, relay_en = 1 (switch on), relay_en = 0 (switch off)
        case 2:
          StaticJsonBuffer<50> jsonBuffer;
          JsonObject& ret = jsonBuffer.parseObject(command);
          relay_en = ret["relay_en"].toInt();
          digitalWrite(14, relay_en); 
          ret.clear();

          DEBUGLN("Switch power to %d", relay_en); 
          break;

         // 3 SUB_TOPIC_ALM - Target alarm value settings, alarm_t (Max temperature), alarm_h (Max humidity),  
        case 3:
          float tmp_t, tmp_h;

          StaticJsonBuffer<100> jsonBuffer;
          JsonObject& ret = jsonBuffer.parseObject(command); 
          tmp_t = ret["alarm_t"].toFloat();
          tmp_h = ret["alarm_h"].toFloat();

          if (tmp_t) {
            EEPROM.write(addr_alarm_t, tmp_t);
            EEPROM.commit();
            alarm_t = tmp_t;
            DEBUGLN("Set max temperature to %f Celsius", alarm_t); 
          }
          else DEBUGLN(F("Max temperature valuse is empty."));

          if (tmp_h) {
            EEPROM.write(addr_alarm_h, tmp_h);
            EEPROM.commit();
            alarm_h = tmp_h;
            DEBUGLN("Set max humidity to %f \%", alarm_h);             
          }
          else DEBUGLN(F("Max humidity valuse is empty."));

          ret.clear();
          break;

        // 4 SUB_TOPIC_TME - Set timer for executing certain commmand
        case 4:
          String tmp = ""; 
          char buffer[32];                                           // at least big enough for the whole string
          memset(buffer, 0, 32);

          StaticJsonBuffer<100> jsonBuffer;
          JsonObject& ret = jsonBuffer.parseObject(command); 
          tmp = ret["timer"];
          if (tmp) {
            EEPROM.write(addr_timer, tmp);
            EEPROM.commit();
            tmp.toCharArray(buffer, sizeof(buffer));
            timer = atol(buffer);                                    // Converts String to long integer  
            DEBUGLN("Set timer to %d", timer); 
          }
          else DEBUGLN(F("Timer valuse is empty."));                                     
          ret.clear();
          break;

        // 5 SUB_TOPIC_NLT - Receive night light control command from the app and send it to Slave
        case 5:
          uint8_t tmp, mod;
          uint8_t tmp2, mod2;     

          StaticJsonBuffer<50> jsonBuffer;
          JsonObject& ret = jsonBuffer.parseObject(command);
          tmp  = ret["night_light"].substring(0,1).toInt();
          tmp2 = ret["night_light"].substring(1).toInt(); 
          if (tmp) mod = tmp; else DEBUGLN(F("Night light command is empty"));
          if (tmp2) mod2 = tmp2; else DEBUGLN(F("Night light command is empty"));

          switch (mod) {
            case 1:
              if (mod2) for (int k = 0; k < 7; k++) Serial1.print(no10[k]);
              else      for (int k = 0; k < 7; k++) Serial1.print(no11[k]);                           
              break;
            case 2:
              if (mod2) for (int k = 0; k < 7; k++) Serial1.print(no20[k]);
              else      for (int k = 0; k < 7; k++) Serial1.print(no21[k]);                           
              break;
            case 3:
              if (mod2) for (int k = 0; k < 7; k++) Serial1.print(no30[k]);
              else      for (int k = 0; k < 7; k++) Serial1.print(no31[k]);                           
              break;
            case 4:
              if (mod2) for (int k = 0; k < 7; k++) Serial1.print(no40[k]);
              else      for (int k = 0; k < 7; k++) Serial1.print(no41[k]);                           
              break;                            
          }

          DEBUGLN("Sent LED message to Slave: %s", command);
          break;       

        // 6 SUB_TOPIC_MOT - Receive Motor control command from the app (Water Pump)
        case 6:
          uint16_t tmp;
          StaticJsonBuffer<50> jsonBuffer;
          JsonObject& ret = jsonBuffer.parseObject(command);
          tmp = ret["motor_time"].toInt();
  
          if (tmp) {
            time_motor_set = tmp;        
          }
          else {
            time_motor_set = DEFAULT_MOTOR_TIME;                     // Default running time: 6 sec
          }

          motor_en = true;
          pumpCtl()

          ret.clear();
          DEBUGLN("Sent IR message to Slave: %s", msg);
          break;

        // 7 SUB_TOPIC_SYN - Synchronize device status data between master and app
        case 7:
          String sync;
          
          sync = stateFeedBack();
          PubMQTT(SUB_TOPIC_SYN, sync.c_str(), false);

          DEBUGLN(F("Device status data sync complete");  
          break;

        // 8 SUB_TOPIC_IRC - Receive IR control command from the app and send it to Slave
        case 8:
          uint8_t len = sizeof(head1)/sizeof(head1[0]);
          for (uint8_t i = 0; i < len; i++) {
            Serial1.print(head1[i]);
            delay(1);                                                // Delay 1ms to prevent from sending failure
          }
          Serial1.print(command);

          DEBUGLN("Sent IR message to Slave: %s", msg);
          break;
      }  
      cmpQueue[queueReadIdx] = "" ;
      cmdQueue[queueReadIdx] = "" ;
      queueReadIdx++;
      if (queueReadIdx >= QSIZE_IN) queueReadIdx = 0 ;
      queueLength--;
    }
}

void displayOled() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);   display.println("Temp:" + String(temp) + "C");
  display.setCursor(68, 0);  display.println("Hum:" + String(humi) + "%");
  display.setCursor(0, 12);  display.println("AlarmSet:" + String(alarm_t) + "%");
  display.setCursor(74, 12); display.println("Soil:" + String(humi_soil) + "%"); 
  display.setCursor(0, 24);  display.println(String(gaymsg));
  display.setCursor(0, 57);  display.println("Timer:" + String(timer));
  
  display.setTextSize(2);
  display.setCursor(10, 37); display.println(String(TimeShoww));
  if (a_f == 1) display.drawBitmap(93, 32, image_data, 32, 32, 1);
  if (t_f == 1) display.drawBitmap(93, 32, alarm_icon, 32, 32, 1);
  display.display(); 

  DEBUGLN(F("OLED update complete");
  }


void luxReader() {

  Wire.beginTransmission(BH1750address);
  Wire.write(0x10);
  Wire.endTransmission();

  uint8_t bh1750Read(uint8_t address) 
  {
    uint8_t ret;
    Wire.beginTransmission(address);
    Wire.requestFrom(address, 2);
    while(Wire.available()) 
    {
      buff[ret] = Wire.read();  
      ret++;
    }
    Wire.endTransmission();  
    return ret;
  }

  if (2 == BH1750_Read(BH1750address))
  {
    lux = ((buff[0] << 8) | buff[1]) / 1.2;
    DEBUG(lux, DEC);     
  }
}



String stateFeedBack() {
  String ret = "";
  StaticJsonBuffer<200> jsonBuffer; 
  JsonObject& doc = jsonBuffer.createObject();
  doc["alarm_t"] =  alarm_t;
  doc["alarm_h"] =  alarm_h;
  doc["relay"] =  relay_en;
  serializeJson(doc, ret);
  return ret;
}


bool personDetectMonitor() {
  detect = digitalRead(12);
    if (detect) {
      String msg = "personDetectMonitor: Person Detected"
      PubMQTT(PUB_TOPIC_NTI, msg.c_str());
      DEBUGLN(msg);                                                                     // Send notification to the app
      for (int k = 0; k < strlen(no21); k++) {                                          // Switch on night light
        Serial1.print(no21[k]);
        delay(1);
      }
      return true;
    }
    else {
      DEBUGLN(F("personDetectMonitor: Area Clear"));
      for (int k = 0; k < strlen(no20); k++) {                                           // Switch off night light
        Serial1.print(no20[k]);
        delay(1);      
      }
      return false;
    }
  return false;
}


void alarmValueMonitor() {

  temp = dht.readTemperature();
  humi = dht.readHumidity();
  humi_soil = 100 - analogRead(A0) / 6.5;

  if (isnan(temp)) {
    DEBUGLN(F("alarmValueMonitor: Temperature value is null"));  
  }
  else {
    if (temp >= alarm_t) {
      flag_alarm_t = true;
      if (BUZZER_EN) digitalWrite(15, 1);
      DEBUGLN("alarmValueMonitor: Temperature value %f exceeds %f", temp, alarm_t);
    }
    else {
      flag_alarm_t = false;
      if (BUZZER_EN && digitalRead(15)) digitalWrite(15, 0);     
      DEBUGLN("alarmValueMonitor: Temperature value %f belows %f", temp, alarm_t);      
    }
  }         

  if (isnan(humi)) {
    DEBUGLN(F("alarmValueMonitor: Humidity value is null"));
  }
  else {
    if (humi >= alarm_h) {
      flag_alarm_h = true;
      if (BUZZER_EN) digitalWrite(15, 1);
      DEBUGLN("alarmValueMonitor: Humidity value %f exceeds %f", humi, alarm_h);
    }
    else {
      flag_alarm_h = false;
      if (BUZZER_EN && digitalRead(15)) digitalWrite(15, 0);     
      DEBUGLN("alarmValueMonitor: Humidity value %f belows %f", humi, alarm_h);
    }
  }  

  if (isnan(humi_soil)) {
    DEBUGLN(F("alarmValueMonitor: Soil humidity value is null"));  
  }
  else {
    DEBUGLN("alarmValueMonitor: Soil humidity value is %f", humi);  
  }           
}


void pumpCtl() {
  if (motor_en) {
    digitalWrite(16, 1);  
    time_motor_last = millis();
    motor_en = false;
    DEBUGLN(F("Water pump started."))
  }

  if (!motor_en && (millis() - time_motor_last > time_motor_set) && time_motor_last != 0) {
    digitalWrite(16, 0);
    time_motor_last = 0;
    DEBUGLN(F("Water pump stopped."))    
  }
}

void timerCtl() {
  if (timer <= TimePro && timer != 0) {
    flag_timer = true;
    digitalWrite(14,0);                                                                   // Turn off power relay when timer is triggered.
    String msg = "Timer at " + String(timer) + " is triggered."
    PubMQTT(PUB_TOPIC_NTI, msg.c_str());
    timer = 0;
  }
}


String dataFeedBack() {
  DynamicJsonDocument doc(400);
  String ret;
  doc["temperature"]  =  t;
  doc["humidity"]     =  h;
  doc["waterlevel"]   =  soil_hum;
  doc["lux"]          =  lux;
  doc["ir"]           =  IR;
  doc["gas"]          =  hazardgas;
  serializeJson(doc, ret);
  return ret;
}

void webScrabber() {

  if (millis() - time_ntp_last > 60000) {
    time_ntp_last = millis();

    WiFiClient client;
    HTTPClient http;

    http.begin(client, "http://quan.suning.com/getSysTime.do");
    int httpCode = http.GET();
    String payload = http.getString();
    DEBUGLN(payload);

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& ret = jsonBuffer.parseObject(payload);
    String TimeShow = ret["sysTime2"];
    String TimeProS = ret["sysTime1"];

    TimeShoww = TimeShow.substring(11,16);
    String TimeProSS = TimeProS.substring(2,12);

    char longbuf[32];                                     // make this at least big enough for the whole string
    TimeProSS.toCharArray(longbuf, sizeof(longbuf));
  }

}

void loop() {}
