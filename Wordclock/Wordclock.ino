
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
int MaxBrightness = 150;

Adafruit_NeoPixel strip(NumPixels, PixelPin, NEO_GRB + NEO_KHZ800);

//WiFiSettings werden nach folgendem Schema definiert:
const char *ssid    = "CTDO-LEGACY";
const char *password = "ctdo2342";

//Setup für NTPclient (zum abfragen der Uhrzeit)
int utcOffsetInSeconds = 3600;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Definition des Grids/ der Worte auf der Uhr
int es_ist[] = {0,1,3,4,5};

int zusatz[][6]={{23, 22, 21, 20, 19, 18}, //gerade
                 {17, 16, 15, 14, 13, 12}, //gleich
                 {30, 31, 32, 33, -1, -1}}; //etwa

int minute[][19] = {{24, 25, 26, 27, 28, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //punkt
                    {48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, -1, -1, -1, -1}, //fünf minuten nach
                    {39, 38, 37, 36, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, -1, -1, -1, -1}, //zehn minute nach
                    {71, 70, 69, 68, 67, 66, 65, 63, 62, 61, 60, -1, -1, -1, -1, -1, -1, -1, -1}, //viertel nach
                    {47, 46, 45, 44, 43, 42, 41, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, -1}, //zwanzig minuten nach
                    {48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, 78, 79, 80, 81, -1}, //fünf minute vor halb
                    {78, 79, 80, 81, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //halb
                    {48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 63, 62, 61, 60, 78, 79, 80, 81}, //fünf minuten nach halb
                    {47, 46, 45, 44, 43, 42, 41, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, -1, -1}, //zwanzig minuten vor
                    {71, 70, 69, 68, 67, 66, 65, 73, 74, 75, -1, -1, -1, -1, -1, -1, -1, -1, -1}, //viertel vor
                    {39, 38, 37, 36, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, -1, -1, -1, -1, -1}, //zehn minuten vor
                    {48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 73, 74, 75, -1, -1, -1, -1, -1}}; //fünf minuten vor
                     
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
int h=0;
int m=0;
int h_new, m_new, Stunde, Minute;

int CurrentCase[]={0,0,0};

byte WheelPos = random(0,255);
String colorStr = "rainbow";
uint32_t color = strip.Color(255,150,0);
uint32_t colorSet;

int i;

bool testmodus = 0; //Testmodus, um mehrere LED zu prüfen, oder ähnliches (1=an, 0=aus)
bool OnOff = 1;     //zum ein und ausschalten später mit Homie

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
  strip.setBrightness(MaxBrightness);
  strip.show();

  //timeClient-Setup
  timeClient.begin();
  timeClient.update();
}


void loop() {
  
  //Wenn testmodus eingeschaltet, dann zeige einfach 40 Pixel in weiß an
  if (testmodus){
    for (int i=10; i<=40; i++){
      strip.setPixelColor(i, 255, 255, 255);
    }
    strip.show();
  } 
  
  //Wenn Uhr an, dann Update anzeige
  else if (OnOff){
    UpdateTime();
  }

}


void UpdateTime(){
    //Abfragen der aktuellen Zeit:
    timeClient.update();
    int h_new = timeClient.getHours();
    int m_new = timeClient.getMinutes();
  
    //Ändern der Anzeige nur notwendig, wenn neue Zeit anders als alte
    if(h_new != Stunde || m_new != Minute) {

      //Überschreiben der alten Werte
      h = h_new;
      m = m_new;
      Stunde = h_new;
      Minute = m_new;

      //Umwandeln in 12-Stunden Format
      if (h > 12){
        h=h-12;
      }
      if (h == 0){
        h = 12;
      }
      
      //Ermitteln des aktuellen Cases
      getCurrentCase(h,m);

      //Umstellen der Anzeige entsprechend dem Case
      strip.clear();

      for (i=0; i<5; i++){
        setPixels(colorStr,es_ist[i]);  
      }
      for (i=0; i<6; i++){   
        setPixels(colorStr,zusatz[CurrentCase[0]][i]);
        setPixels(colorStr,stunde[CurrentCase[2]][i]);
      }
      for (i=0; i<19; i++){
        setPixels(colorStr,minute[CurrentCase[1]][i]);
      }
      
      //anzeigen
      strip.show();   
      Serial.println("new time"); 
    }
}

void getCurrentCase(int h, int m){

  //CurrentCase[1] Minute Bestimmen (teilen durch 5 und runden auf ganze Zahl bildet den Eintrag in minuten
    CurrentCase[1] = m/5;
    if (CurrentCase[1] >= 12){
      CurrentCase[1] = 0;
    }  

  //CurrentCase[2] Stunde bestimmen in Abhängigkeit von Minute
    CurrentCase[2] = h-1;
    //ab Minute 28 bis Minute 60 
    if (m >= 28 && m <= 60){
      CurrentCase[2] = h;
    }

  //CurrentCase[0] Zusatz bestimmen
    //Zustand-Zusatz 0 (gerade): m%5 == 0
    if (m%5 == 0){
      CurrentCase[0] = 0;
    }
    //Zustand-Zusatz 1 (gleich): m%5 => 3
    else if (m%5 >= 3){
      CurrentCase[0] = 1;
    }
    //Zustand-Zusatz 2 (etwa): m%5 <= 2 && !=0
    else if (m%5 <= 2){
      CurrentCase[0] = 2;
    }
    
}

void setPixels(String colorStr, int PixelPos){

  //Set next Pixel
  if (PixelPos >= 0){
    //Get new color
    if (colorStr == "rainbow"){
      colorSet = Wheel(WheelPos);
      WheelPos++;
    } else {
      colorSet = color;
    }
  
    strip.setPixelColor(PixelPos,colorSet);
  } 
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
