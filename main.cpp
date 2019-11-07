#include <Arduino.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <DHT.h>

#define LED1 27 //led1
#define LED2 26 //led2
#define BUTTON1 12 //led1butto
#define BUTTON2 14 //led2 butto
#define DEL_TIME 5000 //czas odstepu miedzy odczytami
#define BUTT_DEBOU_T 300 //czas do debouncingu przyciskow
#define SERV_PIN 25 //servo control pwm pin
#define SERV_DEL_T 100 //czas na kolejny ruch serwa o 1 stopien
#define DHT_PIN 33
unsigned long stime=0;
unsigned long b1_time=0;//for button debouncing
unsigned long b2_time=0;//for button debouncing


bool led1_state =0;
bool led2_state =0;
int pos=0;
Adafruit_AM2320 am2320 = Adafruit_AM2320();
DHT dht(DHT_PIN,DHT11);//dht object


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // hang out until serial port opens
  }
  pinMode(LED1,OUTPUT);//led1
  digitalWrite(LED1,LOW);
  pinMode(LED2,OUTPUT);//led2
  digitalWrite(LED2,LOW);

  pinMode(BUTTON1,INPUT_PULLUP);
  pinMode(BUTTON2,INPUT_PULLUP);

  Serial.println(" Basic Test");
  
  am2320.begin();
  dht.begin();
  stime=millis();
  //test:
 
}

void loop() {
  unsigned long actime=millis();

  if(!digitalRead(BUTTON1) && actime-b1_time>BUTT_DEBOU_T){//zmiana stanu led1
    led1_state=!led1_state;
    digitalWrite(LED1,led1_state);
    b1_time=millis();
  }

  if(!digitalRead(BUTTON2) && actime-b2_time>BUTT_DEBOU_T){//zmiana stanu led2
    led2_state=!led2_state;
    digitalWrite(LED2,led2_state);
    b2_time=millis();
  }
  
  if(actime-stime>DEL_TIME){
    Serial.print("Temp: "); Serial.println(am2320.readTemperature());
    Serial.print("Hum: "); Serial.println(am2320.readHumidity());


     Serial.print("Humidity (%): "); //output value humidity 
 float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));

  
    stime=millis();//reset wartosci
  }
  
   
}
