
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//Definition der Neopixel
#define PixelPin D2
int NumPixels = 144;

Adafruit_NeoPixel strip(NumPixels, PixelPin, NEO_GRB + NEO_KHZ800);

//WiFiSettings werden nach folgendem Schema definiert:
const char *ssid     = "***";
const char *password = "***";

//Setup für NTPclient (zum abfragen der Uhrzeit)
int utcOffsetInSeconds = 3600;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Testmodus, um mehrere LED zu prüfen, oder ähnliches (1=an, 0=aus)
bool testmodus = 0;

//Definition des Grids/ der Worte auf der Uhr
int es_ist[] = {0,1,3,4,5};

int zusatz[][6]={{23, 22, 21, 20, 19, 18}, //gerade
                 {17, 16, 15, 14, 13, 12}, //gleich
                 {30, 31, 32, 33, -1, -1}}; //etwa

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
                    {24, 25, 26, 27, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}; //punkt
                     
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

//Definition weiterer Variablen
int Stunde, Minute_full, Minute, Minute_second, Zusatz,h_new, m_new;
int h=0;
int m=0;
byte WheelPos;

void setup() {
  Serial.begin(115200);

  //Wifi-Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting..");
  }

  //NeoPixel-Setup
  strip.begin();
  strip.setBrightness(50);
  strip.show();

  //timeClient-Setup
  timeClient.begin();
  timeClient.update();
}


void loop() {
  if (testmodus){
    for (int i=10; i<=40; i++){
      strip.setPixelColor(i, 255, 255, 255);
    }
  } 
  else {

    //Abfragen der Zeit:
    timeClient.update();
    int h_new = timeClient.getHours();
    int m_new = timeClient.getMinutes();
  
    //Ändern der Anzeige nur notwendig, wenn neue Zeit anders als alte
    if(h_new != h || m_new != m) {

      //Überschreiben der alten Werte
      h = h_new;
      m = m_new;
      Stunde = h;
      Minute_full = m;
      Minute_second = m % 10;

      //Umwandeln der Minute in das entsprechende Element der minute-Liste (da auf 5Minuten gerundet wird, ist dies notwendig)
      Minute = convertMinute(Minute_full);

      //Zusatz bestimmen, in Abhängigkeit von der Minute wird gerade, gleich oder etwa hinzugefügt
      Zusatz = determineZusatz(Minute_second, Minute_full);
    
      //Stunde in 12h-Format umwandeln
      if (Stunde == 0){
        Stunde = 12;
      } else if (Stunde >12){
        Stunde = Stunde - 12;
      }

      //Stunde entsprechend der Minute anpassen
      if (Minute >= 4 && Minute < 11 || Minute == 11 && Zusatz == 1) {
        Stunde ++;
      }
    
      //notwendige Pixel-einschalten, Farbe wird dabei zufällig gewählt und dann anhand dem Color-Weel geändert
      // es_ist + zusatz + minute + stunde
      WheelPos = random(0,255); 
      strip.clear();
      
      for (int i=0; i<= 4; i++){
        if (es_ist[i] > -1){
          strip.setPixelColor(es_ist[i], Wheel(WheelPos));
          WheelPos ++;
          if (WheelPos >255){
            WheelPos = 0;
          }
        }
      }
    
      for (int i=0; i<= 5; i++){
        if (zusatz[Zusatz][i] > -1) {
          strip.setPixelColor(zusatz[Zusatz][i], Wheel(WheelPos));
          WheelPos ++;
          if (WheelPos >255){
            WheelPos = 0;
          }
        }
      }
    
      for (int i=0; i<=18; i++){
        if (minute[Minute][i] > -1){
          strip.setPixelColor(minute[Minute][i], Wheel(WheelPos));
           WheelPos ++;
          if (WheelPos >255){
            WheelPos = 0;
          }
        }    
      }
    
      for (int i=0; i<=5; i++){
        if (stunde[(Stunde-1)][i] > -1){
          WheelPos ++;
          if (WheelPos >255){
            WheelPos = 0;
          }
        }
      }
    
    }
  }
  strip.show();
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
