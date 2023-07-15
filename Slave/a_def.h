#define NUM_LEDS 60 
#define PIN 6 
#define BUTTON 2
#define KILLBUTTON 3

CRGB leds[NUM_LEDS];

BH1750 lightMeter(0x23);

SoftwareSerial mySerial(10, 11);



// ------------------------ Bluetooth Mesh Command Sheet-----------------

const char head1[4]    PROGMEM = {0xAA, 0xFB, 0x00, 0x11};
const char head2[4]    PROGMEM = {0xAA, 0xFB, 0x00, 0x22};

// ------------------------ IR Data Sheet-----------------

const char IR1[5]      PROGMEM = {0xA1,0xF1,0x22,0xDD,0xF5};
const char command1[3] PROGMEM = {0xE7, 0xF3, 0x01};
const char command0[3] PROGMEM = {0xE7, 0xF3, 0x00};