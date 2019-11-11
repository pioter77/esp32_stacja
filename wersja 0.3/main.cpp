#include <Arduino.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <DHT.h>
#include <Wire.h>
#include <Stepper.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/*USUN CHUJU TE DELAYE Z FUNKCJI JSONA I ZMIEN TO NA MILLISY
*/
#define LED1 27 //led1
#define LED2 26 //led2
#define BUTTON1 12 //led1butto
#define BUTTON2 14 //led2 butto
#define BUTTON3 13 //allows to send data to web page
#define DEL_BUTTON 300 //for debouncing
#define DEL_SENSOR_POST 600000 //time dalay  ot sned past sensor data to server
#define DEL_JSON_GET 120000//time delay between sensding get json requests
#define SERV_PIN 25 //servo control pwm pin
#define SERV_DEL_T 100 //czas na kolejny ruch serwa o 1 stopien
#define DHT_PIN 33 //dht data pin

#define ROT_MOTOR_STEPS 20 //number of engine steps
#define ROT_L293_P1 19//l293d pins used to control the stepper motor
#define ROT_L293_P2 18
#define ROT_L293_P3 4
#define ROT_L293_P4 15
#define ROT_ANGLE_VAL 5 //5/20 is 1/4*360=90 degrees
#define ROT_SPEED 5 //set rotation speed
//const char* ssid     = "ssid";
const char* ssid     = "ssid";
const char* password = "pass";
  
//const char* password = "pass";
const char* serverName = "http://mysite/post-data.php"; 
const char* serverName_json="http://mysite/control_data.json";

unsigned long time_b1_last_pressed=0;//for button debouncing
unsigned long time_b2_last_pressed=0;//for button debouncing
unsigned long time_last_post_send=0;//for post method for sensors
unsigned long time_last_json_get=0;//for json request

bool led1_state =0;
bool led2_state =0;
bool gate_state=0;//1 when open 0 when closed

bool first_run=1; //warunek 1 uruchomienia
//struktura do przechowaywnaia bledow
struct error_holder{
  bool temp_sens_err;
  bool mywifi_post_err;
  byte myjson_get_err;
}my_err_holder={0,0,0};

Stepper myStepper(ROT_MOTOR_STEPS, ROT_L293_P1, ROT_L293_P2, ROT_L293_P3, ROT_L293_P4);
Adafruit_AM2320 am2320 = Adafruit_AM2320();
DHT dht(DHT_PIN,DHT11);//dht object

String apiKeyValue = "tPmAT5Ab3j7F9";
String sensorName = "BMP280";
String sensorLocation = "Office";

void rot_fcn_Rotate(int rot_steps_no,bool rot_dir);
void wifi_initialize_block(void);
int mywifi_send_POST_sensor(float *dht_temp,float *dht_hum,float *am2320_temp,float *am2320_hum,const char* server_name);
bool sensors_readout(float *dht_temp,float *dht_hum,float *am2320_temp,float *am2320_hum);
void ledChange(int pin_no,bool state,bool *prev_state);
byte myjsonhandler(const char *serverjsonname);

void setup() {
  Serial.begin(115200);
 
  wifi_initialize_block();//contains all stuff to initialize wifi connection
 



  pinMode(LED1,OUTPUT);//led1
  digitalWrite(LED1,LOW);
  pinMode(LED2,OUTPUT);//led2
  digitalWrite(LED2,LOW);

  pinMode(BUTTON1,INPUT_PULLUP);
  pinMode(BUTTON2,INPUT_PULLUP);
  pinMode(BUTTON3,INPUT_PULLUP);
  
 
  
  am2320.begin();       //initialize am230 sensor
  dht.begin();          //initialize dht sensor
  
  myStepper.setSpeed(ROT_SPEED);
  //test:
 
}

void loop() {

  unsigned long time_actual=millis();
/*
  if(!digitalRead(BUTTON1) && time_actual-time_b1_last_pressed>DEL_BUTTON){//zmiana stanu led1
    led1_state=!led1_state;
    digitalWrite(LED1,led1_state);
    time_b1_last_pressed=millis();
  }

  if(!digitalRead(BUTTON2) && time_actual-time_b2_last_pressed>DEL_BUTTON){//zmiana stanu led2
    led2_state=!led2_state;
    digitalWrite(LED2,led2_state);
    time_b2_last_pressed=millis();
  }
  */
 if(time_actual-time_last_json_get>DEL_JSON_GET || first_run){
   my_err_holder.myjson_get_err=myjsonhandler(serverName_json);
   time_last_json_get=millis();
  }
 

  //blok do wifi:
  if(time_actual-time_last_post_send>DEL_SENSOR_POST  ||  first_run){
     float dht_t,dht_h,am2320_t,am2320_h;
    my_err_holder.temp_sens_err=sensors_readout(&dht_t,&dht_h,&am2320_t,&am2320_h);
     my_err_holder.mywifi_post_err=mywifi_send_POST_sensor(&dht_t,&dht_h,&am2320_t,&am2320_h,serverName);//bez ampersanta bo to jest wskażnik wiec przekazemy od razu adres
    time_last_post_send=millis();
    
  }
  first_run=0;
   
}
void gate_holder(bool direction,bool *gate_ac_state)
{/*//if 1 gate will rotate clockwise 90deg so it opens if 0 it will close 
    that happens when values are different in two variables if not nothing changes becouse there is no need to cycle the gate if no change state was ordered*/
  if(direction!=*gate_ac_state){
    rot_fcn_Rotate(ROT_ANGLE_VAL,direction);
    Serial.println("gate state:");
    Serial.print(direction);
    *gate_ac_state=direction;
  }
}

void rot_fcn_Rotate(int rot_steps_no,bool rot_dir)
{	//rotdir 1 for clockwise rotdir 0 for anticlockwise
	if(rot_dir)//clockwise
		myStepper.step(rot_steps_no);
	else
		myStepper.step(-rot_steps_no);
	
}

void wifi_initialize_block(void)
{
   WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}


//function that sends data  to site using post method
int mywifi_send_POST_sensor(float *dht_temp,float *dht_hum,float *am2320_temp,float *am2320_hum,const char *server_name)
{
   //Check WiFi connection status
      if(WiFi.status()== WL_CONNECTED){
        HTTPClient http;
        
        // Your Domain name with URL path or IP address with path
        http.begin(server_name);
        
        // Specify content-type header
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        // Prepare your HTTP POST request data
        
        String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName
                              + "&location=" + sensorLocation + "&value1=" + String(*am2320_temp)
                              + "&value2=" + String(*am2320_hum) + "&value3=" + String(*dht_temp) + "&value4=" + String(*dht_hum) + "";
                              
                           
        Serial.print("httpRequestData: ");
        Serial.println(httpRequestData);
        
        // You can comment the httpRequestData variable above
        // then, use the httpRequestData variable below (for testing purposes without the BME280 sensor)
        //String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&location=Office&value1=24.75&value2=49.54&value3=1005.14";

        // Send HTTP POST request
        int httpResponseCode = http.POST(httpRequestData);
        
        // If you need an HTTP request with a content type: text/plain
        //http.addHeader("Content-Type", "text/plain");
        //int httpResponseCode = http.POST("Hello, World!");
        
        // If you need an HTTP request with a content type: application/json, use the following:
        //http.addHeader("Content-Type", "application/json");
        //int httpResponseCode = http.POST("{\"value1\":\"19\",\"value2\":\"67\",\"value3\":\"78\"}");
            
        
        // Free resources
        http.end();
        Serial.println(httpResponseCode);
        return httpResponseCode;//it will return http request result code
      }
      else {
        Serial.println("WiFi Disconnected");
        return 937;//it will return code that suggest that wifi is not connected
      }
}

//function that will read all sensor values and check it for errors
bool sensors_readout(float *dht_temp,float *dht_hum,float *am2320_temp,float *am2320_hum)
  {
  *dht_temp=dht.readTemperature();
  *dht_hum=dht.readHumidity();
  *am2320_temp=am2320.readTemperature();
  *am2320_hum=am2320.readHumidity();

  if(isnan(*dht_temp) ||  isnan(*dht_hum)  ||  isnan(*am2320_temp)  ||  isnan(*am2320_hum))
    {
      Serial.println("one of the temp sensors has an error");
      return 1;
    }
  return 0;
}

void ledChange(int pin_no,bool state,bool *prev_state){
   if((*prev_state)!=state){
      *prev_state=state;
      digitalWrite(pin_no,state);
    }
}

byte myjsonhandler(const char *serverjsonname)
{
   int sens1=0,sens2=0,sens3=0;//values to get form parsed json object
  StaticJsonBuffer<200> jsonBuffer;//to jest wchuj ważne żeby przy każdym obiegu byl deklarowny
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
    String payload;
    http.begin(serverjsonname); //Specify the URL
    int httpCode = http.GET();                                        //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        payload = http.getString();//payload is a string that stores the response
        Serial.println(httpCode);
        Serial.println(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
      http.end(); //Free the resources
      
      return 3;
    }
 
    http.end(); //Free the resources

    //part responsible for parsing json
    JsonArray& arr1 = jsonBuffer.parseArray(payload);

    if(!arr1.success()) {
      Serial.println("parse failed");
      
      return 4;
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
    ledChange(LED1,(bool)sens1,&led1_state);
    ledChange(LED2,(bool)sens2,&led2_state);
    gate_holder(sens3,&gate_state);
  }
  return 0;
}
