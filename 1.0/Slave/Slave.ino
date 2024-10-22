#include <Arduino_FreeRTOS.h>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <EEPROM.h>
#include <FastLED.h>
#include <Wire.h>
#include <SoftwareSerial.h>



uint8_t selectedEffect = 2;
uint8_t selectedBright = 128;
uint8_t a = 0;
uint8_t c = 0;
uint8_t red, green, blue;
String inputString = "";
String sr, sg, sb;
String tempStrData = "";
String ws = "a0", ms = "a1";
bool stringComplete = false;


long lastMsg = "";
long lastMsg2 = 5000; 
long lastRain = "";

void setup() {

// UART & USART setup --------------------------------------

  Serial.begin(9600);
  mySerial.begin(9600);

// Pin Mode setup ------------------------------------------

  pinMode(5, OUTPUT);

  digitalWrite(BUTTON, HIGH);  
  digitalWrite(KILLBUTTON, HIGH); 
  digitalWrite(8 , 1);

  attachInterrupt(digitalPinToInterrupt(KILLBUTTON), changeEffect3, FALLING); 
  attachInterrupt(digitalPinToInterrupt(BUTTON), changeEffect2, FALLING);


  FastLED.addLeds<WS2811,PIN,GRB>(leds,NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(selectedBright);  


// EEPROM setup --------------------------------------------

  EEPROM.get(0, selectedEffect);
  EEPROM.get(1, selectedBright);
  EEPROM.get(5, c);

  if(selectedEffect == 1){
    EEPROM.get(2, red);
    EEPROM.get(3, green);
    EEPROM.get(4, blue);
  } 



  Wire.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  inputString.reserve(1000);                 // Allocate memory big enough for the inputstring

  xTaskCreate(
    TaskDigitalRead
    ,  "DigitalRead"                         // A name just for humans
    ,  128                                   // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL                                  //Parameters for the task
    ,  2                                     // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );                               //Task Handle

  xTaskCreate(
    TaskDigitalRead
    ,  "DigitalRead"                         // A name just for humans
    ,  128                                   // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL                                  //Parameters for the task
    ,  1                                    // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );                               //Task Handle

}


void changeEffect() {

    EEPROM.put(0, selectedEffect);
    EEPROM.put(1, selectedBright);
    if(selectedEffect == 1){
      EEPROM.put(2, red);
      EEPROM.put(3, green);
      EEPROM.put(4, blue);
    }
    delay(500);
    asm volatile ("jmp 0");     
} 

void changeEffect2() {
  
    selectedEffect++;
    if (selectedEffect > 5) { 
      selectedEffect=2;
    }
    EEPROM.put(0, selectedEffect);
    asm volatile ("jmp 0");   
}

void changeEffect3() {
    selectedEffect = 0;
    EEPROM.put(0, selectedEffect);
    asm volatile ("jmp 0");   
}

defineTaskLoop(ledmain){

  switch(selectedEffect) {
                
    case 0 : { setAll(0,0,0);
                break;
             }
    case 1 : {
                setAll(red,green,blue);
                break;
              }                     
    case 2 : {
                colorWipe(0x00,0xff,0x00, 50);
                colorWipe(0x00,0x00,0x00, 50);
                break;
              }

    case 3 : {
                rainbowCycle(20);
                break;
              }
        
    case 4 : {
                PinkToBlueCycle(20);
                break;
              }
        
    case 5 : {
                AllrainbowCycle(60);
                break;
              }
    case 6 : {
                TwinkleRandom(20, 100, false);
                break;
              } 
    case 7 : {
                RunningLights(0xff,0xff,0x00, 50);
                break;
              }
    case 8 : {
                meteorRain(0xff,0xff,0xff,10, 64, true, 30);
                break;
              } 
    case 9 : {
                theaterChaseRainbow(50);
                break;
              }
    case 10 : {
                Fire(55,120,15,true);
                break;
              }
    case 11 : {
                ChrimasCycle(20);
                break;
              } 
    case 12 : {
                ChrimasSpirit(1);
                break;
              }
    case 13 : {
                Fire(55,120,15,false);
                break;
              }                                                                               
  } 
}   

void loop() {

  mySCoop.sleep(50); 
  String lednum,ledbri;

  uint8_t lednum_byte, ledbri_byte;

  if (stringComplete)                             // 当发现缓存中有数据时，将数据送至字符数组a中{"lednum":3}
  {
     //Serial.println(inputString);
     if(inputString.indexOf("n")!= -1)  
        {
         lednum = inputString.substring(1);
         uint8_t lednum_byte = lednum.toInt();
         selectedEffect = lednum_byte;
         inputString = "";
         stringComplete = false;
         changeEffect();
         }
      else if(inputString.indexOf("b")!= -1)  
        {
         ledbri = inputString.substring(1);
         uint8_t ledbri_byte = ledbri.toInt();
         selectedBright = ledbri_byte;
         inputString = "";
         stringComplete = false;
         changeEffect();
         }
      else if(inputString.indexOf("x")!= -1)      // {"hex":255y245z224} {"hex":250a253b255} {"hex":0xFAy0xFDz0xFF}
        {
         sr = inputString.substring(1,4);
         sg = inputString.substring(4,7);
         sb = inputString.substring(7);
         uint8_t ir = sr.toInt(); uint8_t ig = sg.toInt(); uint8_t ib = sb.toInt();
         red = ir; green = ig; blue= ib;
         //Serial.println(red);Serial.println(green);Serial.println(blue);
         selectedEffect = 1;
         inputString = "";
         stringComplete = false;
         changeEffect();
         }
     else if(inputString.indexOf("m")!= -1)  
        {
         uint8_t acmodei = inputString.substring(1).toInt();
         switch (acmodei) {
             case 0 :{
                    for (uint8_t k = 0; k < 5; k++)
                    {
                      mySerial.print(IR1[k]);}
                      delay(100); 
                      break;}
             case 1 :{
                      break;}
             case 2 :{
                      break;}                   
         }
         inputString = "";
         stringComplete = false;
         }   
      else if (inputString[0] == command0[0] && inputString[1] == command0[1] && inputString[2] == command0[2]) 
        { 
         selectedEffect = 0;
         inputString = "";
         stringComplete = false;
         changeEffect();
    }
      else if (inputString[0] == command1[0] && inputString[1] == command1[1] && inputString[2] == command1[2]) 
        {
         selectedEffect = 3;
         inputString = "";
         stringComplete = false;
         changeEffect();
    }
       else{inputString = "";stringComplete = false;}                         
  }

 long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    String gas = "t"+String(analogRead(A0));
    delay(100);
        for (uint8_t k = 0; k < 4; k++)
        {
          Serial.print(head2[k]);
        }
        Serial.print(gas);
        delay(100);
    }
  if (now - lastRain > 2500) {
    lastRain = now;
    Serial.println(analogRead(A2));
    //if (analogRead(A1) >500 || analogRead(A2) <500) digitalWrite(5,1); else digitalWrite(5,0);
    if (analogRead(A1) >500){
        digitalWrite(5,1);
        delay(100);
        for (uint8_t k = 0; k < 4; k++)
        {
          Serial.print(head2[k]);
        }
        Serial.print(ws);
        delay(100);
      }
   else if (analogRead(A2) <500){
        digitalWrite(5,1);
        delay(100);
        for (uint8_t k = 0; k < 4; k++)
        {
          Serial.print(head2[k]);
        }
        Serial.print(ms);
        delay(100);
      }
    else digitalWrite(5,0);
    }  
  if (now - lastMsg2 > 10000) {
    lastMsg2 = now;
    uint16_t lux = lightMeter.readLightLevel();
    String luxs = "l"+String(lux);
    delay(100);
        for (uint8_t k = 0; k < 4; k++)
        {
          Serial.print(head2[k]);
        }
        Serial.print(luxs);
        delay(100);
    } 
}


void serialEvent() {
  char a;
  bool inputBool = false;
  tempStrData = "";
  while (Serial.available() > 0) 
  { a = char(Serial.read());
      if (a == 0xFFFFFFAA || a == 0xFFFFFFBB) 
          inputBool = true;
      if (inputBool == true) 
      {
          tempStrData += a; 
      }
      delay(2);
    }
    inputString="";
    if (tempStrData.length() > 3 )  
  {     
    for (uint8_t ii = 3; ii < tempStrData.length(); ii++) 
    {
        inputString += tempStrData[ii];delay(2);
    }
    stringComplete = true;
  }
}












