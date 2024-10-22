void setPixel(uint8_t Pixel, uint8_t red, uint8_t green, uint8_t blue) {
   leds[Pixel] = CRGB(red, green, blue);
}

void setAll(uint8_t red, uint8_t green, uint8_t blue) {
  for(uint8_t i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CRGB(red, green, blue);
  }
  showStrip();
  }

void showStrip() {
   FastLED.show();
}


void colorWipe(uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay) {
  for(uint16_t i = 0; i < NUM_LEDS; i++) {
      setPixel(i, red, green, blue);
      showStrip();
      delay(SpeedDelay);
  }
}

void rainbowCycle(int SpeedDelay) {
  uint8_t *c;
  uint16_t i, j;

  uint8_t* wheel(uint8_t WheelPos) {
    static uint8_t color[3];
    
    if (WheelPos < 85) {
    color[0] = WheelPos * 3;
    color[1] = 255 - WheelPos * 3;
    color[2] = 0;
    } else if (WheelPos < 170) {
    WheelPos -= 85;
    color[0] = 255 - WheelPos * 3;
    color[1] = 0;
    color[2] = WheelPos * 3;
    } else {
    WheelPos -= 170;
    color[0] = 0;
    color[1] = WheelPos * 3;
    color[2] = 255 - WheelPos * 3;
    }
    return color;
  }

  for (j = 0; j < 256; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < NUM_LEDS; i++) {
      c = wheel((i * 256 / NUM_LEDS) + j);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    showStrip();
    delay(SpeedDelay);
  }
}




void PinkToBlueCycle(int SpeedDelay) {
  uint8_t *c;
  uint16_t i, j;

  uint8_t* wheel(uint8_t WheelPos2) {
    static uint8_t color[3];
    
    if (WheelPos2 < 128) {
    color[0] = WheelPos2 * 2;
    color[1] = 255 - WheelPos2 * 2;
    color[2] = 255;
    } else {
    WheelPos2 -= 128;
    color[0] = 255 - WheelPos2 * 2;
    color[1] = WheelPos2 * 2;
    color[2] = 255;
    }
  
    return color;  
  }

  for (j = 0; j < 256; j++) { 
    for (i = 0; i < NUM_LEDS; i++) {
      c = wheel((i * 256 / NUM_LEDS) + j);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    showStrip();
    delay(SpeedDelay);
  }
}




void ChrimasCycle(int SpeedDelay) {
  uint8_t *c;
  uint16_t i, j;

  uint8_t* wheel(uint8_t WheelPos) {
    static uint8_t color[3];
    if (WheelPos < 85) {
      color[0] = 255;
      color[1] = 255 - WheelPos3 * 3;
      color[2] = 255 - WheelPos3 * 3;
    } 
    else if (WheelPos < 170) {
    WheelPos3 -= 85;
      color[0] = 255 - WheelPos3 * 3;
      color[1] = WheelPos3 * 3;
      color[2] = 0;
    } 
    else {
      WheelPos -= 170;
      color[0] = WheelPos3 * 3;
      color[1] = 255;
      color[2] = WheelPos3 * 3;
    }
    return color;
  } 

  for (j = 0; j < 256; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < NUM_LEDS; i++) {
      c = wheel((i * 256 / NUM_LEDS) + j);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    showStrip();
    delay(SpeedDelay);
  }  
}





void AllrainbowCycle(int SpeedDelay) {
  uint8_t* c;
  uint16_t i, j;

  uint8_t* Wheel3(uint8_t WheelPos) {
    static uint8_t color[3];
    if (WheelPos < 85) {
      color[0] = WheelPos * 3;
      color[1] = 255 - WheelPos * 3;
      color[2] = 0;
    } 
    else if (WheelPos < 170) {
      WheelPos -= 85;
      color[0] = 255 - WheelPos * 3;
      color[1] = 0;
      color[2] = WheelPos * 3;
    } 
    else {
      WheelPos -= 170;
      color[0] = 0;
      color[1] = WheelPos * 3;
      color[2] = 255 - WheelPos * 3;
    }
    return color;
  }  

  for (j = 0; j < 256; j++) {                             // 5 cycles of all colors on wheel
    for (i = 0; i < NUM_LEDS; i++) {
      c = Wheel3(j);
      setPixel(i, *c, *(c+1), *(c+2));
    }
    showStrip();
    delay(SpeedDelay);
  }  
}


void ChrimasSpirit(uint8_t Count) {
  uint8_t Count3 = 3 * Count;
  uint8_t i=0;
  uint8_t j = Count;
  uint8_t k = 2 * Count;
  for (uint8_t ii = 0; ii < Count; ii++) {
    for (i; i<NUM_LEDS; i += Count3){
      setPixel(i, 255, 255, 255);
    }
    i=ii+1;
  }
    
  for (uint8_t jj = 0; jj < Count; jj++) {
    for (j; j < NUM_LEDS; j += Count3){
      setPixel(j, 255, 0, 0);
    }
    j = Count + jj + 1;
  }
    
  for (uint8_t kk = 0; kk < Count; kk++) {
    for (k; k < NUM_LEDS; k += Count3){
      setPixel(k, 0, 255, 0);
    }
    k = 2 * Count + kk + 1;
  }
  showStrip();
}
  



void TwinkleRandom(uint8_t Count, uint8_t SpeedDelay, boolean OnlyOne) {

  setAll(0, 0, 0);
 
  for (uint8_t i = 0; i < Count; i++) {
     setPixel(random(NUM_LEDS), random(0, 255), random(0, 255), random(0, 255));
     showStrip();
     delay(SpeedDelay);
     if (OnlyOne) {
       setAll(0, 0, 0);
     }
   }
 
  delay(SpeedDelay);
}




void RunningLights(uint8_t red, uint8_t green, uint8_t blue, uint8_t WaveDelay) {
  uint8_t Position=0;
  
  for(uint8_t i = 0; i < NUM_LEDS * 2; i++)
  {
      Position++; // = 0; //Position + Rate;
      for(uint8_t i = 0; i < NUM_LEDS; i++) {
        // sine wave, 3 offset waves make a rainbow!
        //float level = sin(i+Position) * 127 + 128;
        //setPixel(i,level,0,0);
        //float level = sin(i+Position) * 127 + 128;
        setPixel(i,((sin(i + Position) * 127 + 128) / 255) * red,
                   ((sin(i + Position) * 127 + 128) / 255) * green,
                   ((sin(i + Position) * 127 + 128) / 255) * blue);
      }
      
      showStrip();
      delay(WaveDelay);
  }
}




void meteorRain(uint8_t red, uint8_t green, uint8_t blue, uint8_t meteorSize, uint8_t meteorTrailDecay, bool meteorRandomDecay, uint8_t SpeedDelay) {  

  void fadeToBlack(uint8_t ledNo, uint8_t fadeValue) {
  #ifdef ADAFRUIT_NEOPIXEL_H
      // NeoPixel
      uuint8_t32_t oldColor;
      uuint8_t8_t r, g, b;
      uint8_t value;
    
      oldColor = strip.getPixelColor(ledNo);
      r = (oldColor & 0x00ff0000UL) >> 16;
      g = (oldColor & 0x0000ff00UL) >> 8;
      b = (oldColor & 0x000000ffUL);

      r=(r<=10)? 0 : (uint8_t) r-(r*fadeValue/256);
      g=(g<=10)? 0 : (uint8_t) g-(g*fadeValue/256);
      b=(b<=10)? 0 : (uint8_t) b-(b*fadeValue/256);
    
      strip.setPixelColor(ledNo, r,g,b);
  #endif
  #ifndef ADAFRUIT_NEOPIXEL_H
    // FastLED
    leds[ledNo].fadeToBlackBy( fadeValue );
  #endif  
  }

  setAll(0,0,0);
 
  for(uint8_t i = 0; i < NUM_LEDS+NUM_LEDS; i++) {   
    // fade brightness all LEDs one step
    for(uint8_t j=0; j<NUM_LEDS; j++) {
      if((!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
    // draw meteor
    for(uint8_t j = 0; j < meteorSize; j++) {
      if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
        setPixel(i-j, red, green, blue);
      }
    }
   
    showStrip();
    delay(SpeedDelay);
  }
}


void theaterChaseRainbow(uint8_t SpeedDelay) {
  uint8_t *c;
 
  for (uint8_t j=0; j < 256; j++) {                           // cycle all 256 colors in the wheel
    for (uint8_t q=0; q < 3; q++) {
        for (uint8_t i=0; i < NUM_LEDS; i=i+3) {
          c = Wheel( (i+j) % 255);
          setPixel(i+q, *c, *(c+1), *(c+2));              // turn every third pixel on
        }
        showStrip();
       
        delay(SpeedDelay);
       
        for (uint8_t i=0; i < NUM_LEDS; i=i+3) {
          setPixel(i+q, 0,0,0);                           // turn every third pixel off
        }
    }
  }
}


void Fire(uint8_t Cooling, uint8_t Sparking, uint8_t SpeedDelay, bool FireorIce) {
  static uint8_t heat[NUM_LEDS];
  uint8_t cooldown;
 
  void setPixelHeatColor (uint8_t Pixel, uint8_t temperature) {
    uint8_t t192 = round((temperature/255.0)*191);
    uint8_t heatramp = t192 & 0x3F; 
    heatramp <<= 2; 
    if( t192 > 0x80) {                    
      setPixel(Pixel, 255, 255, heatramp);
    } else if( t192 > 0x40 ) {             
      setPixel(Pixel, 255, heatramp, 0);
    } else {                               
      setPixel(Pixel, heatramp, 0, 0);
    }
  }

  void setPixelFrozenColor (uint8_t Pixel, uint8_t temperature) {
    uint8_t t192 = round((temperature/255.0)*191);
    uint8_t heatramp = t192 & 0x3F; 
    heatramp <<= 2; 
    if( t192 > 0x80) {                     
      setPixel(Pixel, heatramp, 255, 255);
    } else if( t192 > 0x40 ) {             
      setPixel(Pixel, 0 , heatramp, 255);
    } else {                               
      setPixel(Pixel, 0 , 0, heatramp);
    }
  }

  for( uint8_t i = 0; i < NUM_LEDS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUM_LEDS) + 2);
   
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
 
  for( uint8_t k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
   
  if( random(255) < Sparking ) {
    uint8_t y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  for( uint8_t j = 0; j < NUM_LEDS; j++) {
    if(FireorIce == true){setPixelHeatColor(j, heat[j] );}
    else {setPixelFrozenColor(j, heat[j] );}
  }

  showStrip();
  delay(SpeedDelay);
}
