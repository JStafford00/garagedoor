#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define LEDBLUE 2 //sets the LED

const char* ssid = "ece631Lab";
const char* password = "esp32IOT!";
const char* mqtt_server = "192.168.1.126";
WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE  (100)
char msg[MSG_BUFFER_SIZE];
unsigned long timer; //timer controlling the blinking of the LED
String garage = "Closed"; //The original state of the garage is closed
const char* code; //the input code
String correct_code = "000000"; //the correct pin input that is accepted
const char* uid; //the keycard number
String correct_uid1 = "897a9bb3"; //Card 1 that can be accepted
String correct_uid2 = "c94ca5b3"; //Card 2 that can be accepted
bool LEDState; //state of the LED to flash it when opening/closing
int count = 0; //count to open/close garage for 10 seconds
float distance; //distance for Ultrasonic Sensor. DECIDE IF FEET or INCHES
bool read_msg = false; //variable stating if there was a message read in to make sure that the code does not constantly run something we don't want it to (i.e. the error checking code)
bool read_uid = false;
bool read_code = false;
bool cread = false;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(1024);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
     
  for (int i = 0; i < length; i++) {
    msg[i] = ((char)payload[i]);
  }
  deserializeJson(doc,msg);

  JsonObject obj = doc.as<JsonObject>();

  //Takes the UID, Code, and Distance from the MQTT topics and sets their respective payloads
  if(obj.containsKey("Password") && cread == false)
  {
    code = obj["Password"];
    read_code = true;
    cread = true;
  }
  else if(obj.containsKey("UID") && cread == false)
  {
    uid = obj["UID"];
    read_uid = true; 
    cread = true;
  }
  if(obj.containsKey("Distance"))
  {
    distance = (float)obj["Distance"];
  }
  if(obj.containsKey("New Password"))
  {
    correct_code = (int)obj["New Password"];
    client.publish("/ece631/Final/Garage", "Password Successfully Changed");
  }
  read_msg = true;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("/ece631/Final/Input", "Hello World");
      // ... and resubscribe
      client.subscribe("/ece631/Final/Input");
    }else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LEDBLUE, OUTPUT);
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 
  timer = millis();
  digitalWrite(LEDBLUE, LOW);
  LEDState = false;
}

void loop() {
  // put your main code here, to run repeatedly:
  DynamicJsonDocument doc(1024); 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  
  if((((String)uid == correct_uid1 || (String)uid == correct_uid2) || (String)code == correct_code) && ((float)distance < 2.0 || (float)distance > 5.0))
  {
    //Publish saying if garage is opening/closing
    if(garage == "Closed")
    {
      //Sends message saying the garage is opening
      doc["Garage"] = "Opening";
      serializeJson(doc, Serial);
      serializeJson(doc, msg);
      client.publish("/ece631/Final/Garage", msg);
      cread = false;

      while(count < 5)
      {
        //Blinks LED every second, the time it takes to open garage.
        if ((millis() - timer + 0xFFFF) % 0xFFFF >= 1000) 
        {
          timer = millis();
          LEDState = LEDState ^ HIGH;
          digitalWrite(LEDBLUE, LEDState);
          count++;
        }
      }

      //Sends message saying the garage is now Open
      doc["Garage"] = "Open";
      serializeJson(doc, Serial);
      serializeJson(doc, msg);
      client.publish("/ece631/Final/Garage", msg);
      garage = "Open";
      digitalWrite(LEDBLUE, HIGH);
      count = 0;
      
    }
    else if(garage == "Open")
    {
      //Sends message saying the garage is closing
      doc["Garage"] = "Closing";
      serializeJson(doc, Serial);
      serializeJson(doc, msg);
      client.publish("/ece631/Final/Garage", msg);
      cread = false;

      while(count < 5)
      {
        //Blinks LED every second, the time it takes to close garage.
        if ((millis() - timer + 0xFFFF) % 0xFFFF >= 1000) 
        {
          timer = millis();
          LEDState = LEDState ^ HIGH;
          digitalWrite(LEDBLUE, LEDState);
          count++;
        }
      }

      //Sends message saying the garage is now closed
      doc["Garage"] = "Closed";
      serializeJson(doc, Serial);
      serializeJson(doc, msg);
      client.publish("/ece631/Final/Garage", msg);
      garage = "Closed";
      digitalWrite(LEDBLUE, LOW);
      count = 0;
    }
    
   
  }
  else //One or more of the conditions is not met
  {
    if(read_msg == true)
    {   
      if((String)code != "" && (String)code != correct_code)
      {
        doc["Password"] = "Incorrect Password";
      }
      if((String)uid != "" && (String)uid != correct_uid1 && (String)uid != correct_uid2)
      {
        doc["UID"] = "Incorrect UID";
      }
      if((float)distance >= 2.0 && (float)distance <= 5.0)
      {
        doc["Distance"] = "Car is not clear of the garage door"; 
      }
      serializeJson(doc, Serial);
      serializeJson(doc, msg);
      if(read_uid == true || read_code == true)
      {
        client.publish("/ece631/Final/Garage", msg);
      }
      cread = false;
    }
  }
  read_msg = false;
  uid = "";
  code = "";
  read_uid = false;
  read_code = false;
}
