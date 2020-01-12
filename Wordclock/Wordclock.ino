

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define PixelPin D2
int NumPixels = 144;

Adafruit_NeoPixel strip(NumPixels, PixelPin, NEO_GRB + NEO_KHZ800);


#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char *ssid     = "*";
const char *password = "*";

int utcOffsetInSeconds = 3600;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

int es_ist[] = {0,1,3,4,5};

int zusatz[][6]={{23, 22, 21, 20, 19, 18}, //gerade
                 {17, 16, 15, 14, 13, 12}, //gleich
                 {30, 31, 32, 33, -1, -1}, //etwa
                 {-1, -1, -1, -1, -1, -1}}; //kein zusatz

int minute[][19] = {{48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, -1, -1, -1, -1}, //fünf minuten nach
                    {39, 38, 37, 36, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, -1, -1, -1, -1}, //zehn minute nach
                    {71, 70, 69, 68, 67, 66, 65, 63, 62, 61, 60, -1, -1, -1, -1, -1, -1, -1, -1}, //viertel nach
                    {47, 46, 45, 44, 43, 42, 41, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, -1}, //zwanzig minuten nach
                    {48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, 78, 79, 80, 81, -1}, //fünf minute vor halb
                    {78, 79, 80, 81, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //halb
                    {48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, 78, 79, 80, 81}, //fünf minuten nach halb
                    {47, 46, 45, 44, 43, 42, 41, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, -1, -1}, //zwanzig minuten vor
                    {71, 70, 69, 68, 67, 66, 65, 73, 74, 75, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //viertel vor
                    {39, 38, 37, 36, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, -1, -1, -1, -1, -1}, //zehn minuten vor
                    {48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, -1, -1, -1, -1, -1}, //fünf minuten vor
                    {24, 25, 25, 27, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}; //punkt
                     
int stunde[][6] = {{ 93,  92,  91,  90,  -1,  -1}, //eins
                   { 95,  94,  93,  92,  -1,  -1}, //zwei
                   { 88,  87,  86,  85,  -1,  -1}, //drei
                   { 96,  97,  98,  99,  -1,  -1}, //vier
                   {102, 103, 104, 105,  -1,  -1}, //fünf
                   {118, 117, 116, 115, 114,  -1}, //sechs
                   {114, 113, 112, 111, 110, 109}, //sieben
                   {120, 121, 122, 123,  -1,  -1}, //acht
                   {128, 129, 130, 131,  -1,  -1}, //neun
                   {125, 126, 127, 128,  -1,  -1}, //zehn
                   {141, 140, 139,  -1,  -1,  -1}, //elf
                   {136, 135, 134, 133, 132,  -1}}; //zwölf

String time_string = "12:37"; // solte ergeben: es ist etwa fünf nach halb eins --> es_ist + zusatz[2][:] + minuten[6][:] + stunden[0][:]

int Stunde, Minute_full, Minute, Minute_second, Zusatz, h, m, h_new, m_new;
byte WheelPos;

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting..");
  }
  
  strip.begin();
  strip.setBrightness(10);
  strip.show();

  timeClient.begin();
  timeClient.update();

  h = 0;
  m = 0;

}


void loop() {
  timeClient.update();
  
  int h_new = timeClient.getHours();
  int m_new = timeClient.getMinutes();


  if(h_new != h || m_new != m) {

    Serial.print(h);
    Serial.println(m);

    h = h_new;
    m = m_new;
    Stunde = h;
    Minute_full = m;
    Minute_second = m % 10;
    
    Minute = convertMinute(Minute_full);
  
    Zusatz = determineZusatz(Minute_second, Minute_full);
  
    if (Stunde == 0){
      Stunde = 12;
    } else if (Stunde >12){
      Stunde = Stunde - 12;
    }

    if (Minute >= 4) {
      Stunde ++;
    }
  
    //notwendige Pixel-Arrays kombinieren
    // es_ist + zusatz + minute + stunde
    int counter = 0;
    WheelPos = random(0,255); 
    strip.clear();
    
    for (int i=0; i<= 4; i++){
      if (es_ist[i] > -1){
        strip.setPixelColor(es_ist[i], Wheel(WheelPos));
        Serial.print(es_ist[i]);
        Serial.print(',');
        counter ++;
        WheelPos ++;
        if (WheelPos >255){
          WheelPos = 0;
        }
      }
    }
  
    for (int i=0; i<= 5; i++){
      if (zusatz[Zusatz][i] > -1) {
        strip.setPixelColor(zusatz[Zusatz][i], Wheel(WheelPos));
        Serial.print(zusatz[Zusatz][i]);
        Serial.print(',');
        counter ++;
        WheelPos ++;
        if (WheelPos >255){
          WheelPos = 0;
        }
      }
    }
  
    for (int i=0; i<=18; i++){
      if (minute[Minute][i] > -1){
        strip.setPixelColor(minute[Minute][i], Wheel(WheelPos));
        Serial.print(minute[Minute][i]);
        Serial.print(',');
        WheelPos ++;
        if (WheelPos >255){
          WheelPos = 0;
        }
        counter ++;
      }    
    }
  
    for (int i=0; i<=5; i++){
      if (stunde[(Stunde-1)][i] > -1){
        strip.setPixelColor(stunde[(Stunde-1)][i], Wheel(WheelPos));
        Serial.print(stunde[(Stunde-1)][i]);
        Serial.println(';');
        counter ++;
        WheelPos ++;
        if (WheelPos >255){
          WheelPos = 0;
        }
      }
    }
  
    strip.show();
  }
}

int convertMinute (int Minute_full){

  //Minute first gibt die "minute" vor
  if (Minute_full >= 58 || Minute_full <= 2){
    Minute = 11;
  } else if (Minute_full >=53){
    Minute = 10;    
  } else if (Minute_full >=48){
    Minute = 9;
  } else if (Minute_full >= 43){
    Minute = 8;
  } else if (Minute_full >= 38){
    Minute = 7;
  } else if (Minute_full >= 33){
    Minute = 6;
  } else if (Minute_full >= 28){
    Minute = 5;
  } else if (Minute_full >= 23){
    Minute = 4;
  } else if (Minute_full >= 18){
    Minute = 3;
  } else if (Minute_full >= 13){
    Minute = 2;
  } else if (Minute_full >= 8){
    Minute = 1;
  } else if (Minute_full >= 3){
    Minute = 0;
  }

  return Minute;
}

int determineZusatz (int Minute_second, int Minute_full){
    //Minute_second gibt vor, ob etwa, gleich, gerade, punkt
  if (Minute_second == 1 || Minute_second == 2 || Minute_second == 6 || Minute_second == 7){
     Zusatz = 2; //etwa
  } else if (Minute_second == 3 || Minute_second == 4 || Minute_second == 8 || Minute_second == 9){
    Zusatz = 1; //gleich
  } else if (Minute_second == 0 || Minute_second == 5){
    if (Minute_full == 0){
      Zusatz = 3; //punkt = kein Zusatz
    } else {
      Zusatz = 0; //gerade
    }
  }

  return Zusatz;
}

uint32_t Wheel(byte WheelPos){
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }  else if (WheelPos < 170)  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }  else  {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
