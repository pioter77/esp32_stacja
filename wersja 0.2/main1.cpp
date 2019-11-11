#include <Arduino.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <DHT.h>
#include <Wire.h>
#include <Stepper.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define LED1 27 //led1
#define LED2 26 //led2
#define BUTTON1 12 //led1butto
#define BUTTON2 14 //led2 butto
#define BUTTON3 13 //allows to send data to web page
#define DEL_BUTTON 300 //for debouncing
#define DEL_SENSOR_POST 600000 //time dalay  ot sned past sensor data to server
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
const char* ssid     = "ssid";
//const char* password = "pass";
//const char* serverName = "http://mysite/post-data.php";    
const char* password = "ssid";
const char* serverName = "http://mysite/post-data.php"; 


unsigned long time_b1_last_pressed=0;//for button debouncing
unsigned long time_b2_last_pressed=0;//for button debouncing
unsigned long time_last_post_send=0;//for button debouncing


bool led1_state =0;
bool led2_state =0;
bool first_run=1; //warunek 1 uruchomienia
//struktura do przechowaywnaia bledow
struct error_holder{
  bool temp_sens_err;
  bool mywifi_post_err;
};
error_holder my_err_holder={0,0};

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
  
 

  //blok do wifi:
  if(time_actual-time_last_post_send>DEL_SENSOR_POST  ||  first_run){
     float dht_t,dht_h,am2320_t,am2320_h;
    my_err_holder.temp_sens_err=sensors_readout(&dht_t,&dht_h,&am2320_t,&am2320_h);
     my_err_holder.mywifi_post_err=mywifi_send_POST_sensor(&dht_t,&dht_h,&am2320_t,&am2320_h,serverName);//bez ampersanta bo to jest wskażnik wiec przekazemy od razu adres
    time_last_post_send=millis();
    
  }
  first_run=0;
   
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
