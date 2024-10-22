// Device Configuration -----------------------------------------

#if defined(ESP8266)

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#endif

#if defined(ESP32)

#include <WiFi.h>
#include <WiFiClient.h>

#endif

// DHT22 Pin configuration --------------------------------------

#define DHTPIN 0
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// I2C address configuration --------------------------------------

uint8_t BH1750address = 0x23;


// OLED screen configuration --------------------------------------

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MISC configuration ---------------------------------------------

#define BUZZER_EN   1 