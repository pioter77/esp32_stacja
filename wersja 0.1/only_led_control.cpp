#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
 
 #define LED1 27 //led1
 bool led1_state=0;
//const char* ssid = "Husarz";
//const char* password =  "t3nrmaNawz2u";
 const char* ssid = "Klonowa_3/1";
const char* password =  "baborow123";
void setup() {
  pinMode(LED1,OUTPUT);
  digitalWrite(LED1,0);
  Serial.begin(115200);
  delay(4000);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
 
}
 
void loop() {
  int sens1=0,sens2=0,sens3=0;//values to get form parsed json object
  StaticJsonBuffer<200> jsonBuffer;//to jest wchuj ważne żeby przy każdym obiegu byl deklarowny
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
    String payload;
    http.begin("http://espstation.000webhostapp.com/control_data.json"); //Specify the URL
    int httpCode = http.GET();                                        //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        payload = http.getString();//payload is a string that stores the response
        Serial.println(httpCode);
        Serial.println(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
      http.end(); //Free the resources
      delay(30000);
      return;
    }
 
    http.end(); //Free the resources

    //part responsible for parsing json
    JsonArray& arr1 = jsonBuffer.parseArray(payload);

    if(!arr1.success()) {
      Serial.println("parse failed");
      delay(30000);
      return;
    }
    sens1=arr1[0];
    sens2=arr1[1];
    sens3=arr1[2];
    Serial.println("pole sensora 1[2]:");
    Serial.println(sens1);
    Serial.println("pole sensora 2[3]:");
    Serial.println(sens2);
    Serial.println("pole sensora 3[4]:");
    Serial.println(sens3);

    if(led1_state!=(bool)sens1){
      led1_state=(bool)sens1;
      digitalWrite(LED1,led1_state);
    }
      
  }
 
  delay(20000);
 
}
