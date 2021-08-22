#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <CircularBuffer.h>

/* Define pins */
#define trigger (23)
#define echo (22)
#define r1 (32)
#define r2 (33)
#define r3 (25)
#define r4 (26)
#define c1 (27)
#define c2 (14)
#define c3 (12)

/* Set up wifi constants */
const char* ssid = "ece631Lab";
const char* password = "esp32IOT!";
const char* mqtt_server = "192.168.1.126";

//const char* ssid = "ATTsEpqNuI";
//const char* password = "sps?awqe4udy";
//const char* mqtt_server = "192.168.1.126";

/* Initiate wifi clients */
WiFiClient espClient;
PubSubClient client(espClient);
DynamicJsonDocument doc(1024);
DynamicJsonDocument pass(1024);
DynamicJsonDocument newPass(1024);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

int PWM_FREQUENCY = 16; // Defines the frequency
int PWM_CHANNEL = 0;    // Selects the channel number
int PWM_RESOLUTION = 8; // Defines the resolution of the signal
int dutyCycle = 1;      // Defines the width of the signal
volatile byte state = LOW;
volatile byte prevState = LOW;
float distance = 0;
float totalDistance = 0;
void ISR() { state = !state;}

/* Moving Average Filter Initiate */
const int sampleSize = 11;
CircularBuffer<float, sampleSize> Buffer;

float start;
float finish;


unsigned long printStart = millis();
unsigned long printFinish;
unsigned long buttonStart = millis();
unsigned long buttonFinish;
unsigned long buffStart = millis();
unsigned long buffFinish = 0;
unsigned long inputStart = millis();
unsigned long inputFinish = 0;

int keyIndex = 0;

bool buttonPressed = false;

String passcode;
String newPasscode;
bool isNewPass = false;

void setup_wifi() 
{

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

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("/ece631/Lab4Python/NFC/UID");
    } 
    
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

char keypad()
{
  char keyVal;
  bool valCheck = false;
  
  // Row 1
  digitalWrite(r1, HIGH);
  if (digitalRead(c1) == HIGH)
  {
    keyVal = '1';
    valCheck = true;
  }
  else if (digitalRead(c2) == HIGH)
  {
    keyVal = '2';
    valCheck = true;
  }
  else if (digitalRead(c3) == HIGH)
  {
    keyVal = '3';
    valCheck = true;
  }

//  Serial.print("r1-> ");
//  Serial.print(digitalRead(c1));
//  Serial.print(digitalRead(c2));
//  Serial.println(digitalRead(c3));
  digitalWrite(r1, LOW);
  
  // Row 2
  digitalWrite(r2, HIGH);
  if (digitalRead(c1) == HIGH)
  {
    keyVal = '4';
    valCheck = true;
  }
  else if (digitalRead(c2) == HIGH)
  {
    keyVal = '5';
    valCheck = true;
  }
  else if (digitalRead(c3) == HIGH)
  {
    keyVal = '6';
    valCheck = true;
  }
//  Serial.print("r2-> ");
//  Serial.print(digitalRead(c1));
//  Serial.print(digitalRead(c2));
//  Serial.println(digitalRead(c3));
  digitalWrite(r2, LOW);

  // Row 3
  digitalWrite(r3, HIGH);
  if (digitalRead(c1) == HIGH)
  {
    keyVal = '7';
    valCheck = true;
  }
  else if (digitalRead(c2) == HIGH)
  {
    keyVal = '8';
    valCheck = true;
  }
  else if (digitalRead(c3) == HIGH)
  {
    keyVal = '9';
    valCheck = true;
  }
//  Serial.print("r3-> ");
//  Serial.print(digitalRead(c1));
//  Serial.print(digitalRead(c2));
//  Serial.println(digitalRead(c3));
  digitalWrite(r3, LOW);

  // Row 4
  digitalWrite(r4, HIGH);
  if (digitalRead(c1) == HIGH)
  {
    keyVal = '*';
    valCheck = true;
  }
  else if (digitalRead(c2) == HIGH)
  {
    keyVal = '0';
    valCheck = true;
  }
  else if (digitalRead(c3) == HIGH)
  {
    keyVal = '#';
    valCheck = true;
  }
//  Serial.print("r4-> ");
//  Serial.print(digitalRead(c1));
//  Serial.print(digitalRead(c2));
//  Serial.println(digitalRead(c3));
  digitalWrite(r4, LOW);

  if (!valCheck)
    keyVal = ' ';

  return keyVal;
}

void setup() 
{
  Serial.begin(9600);
  delay(100);
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT_PULLUP);
  pinMode(r1, OUTPUT);
  pinMode(r2, OUTPUT);
  pinMode(r3, OUTPUT);
  pinMode(r4, OUTPUT);
  pinMode(c1, INPUT);
  pinMode(c2, INPUT);
  pinMode(c3, INPUT);
  digitalWrite(r1, LOW);
  digitalWrite(r2, LOW);
  digitalWrite(r3, LOW);
  digitalWrite(r4, LOW);
  attachInterrupt(digitalPinToInterrupt(echo), ISR, CHANGE);
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(trigger, PWM_CHANNEL );
  ledcWrite(PWM_CHANNEL, dutyCycle);

  Serial.setTimeout(100);
  Serial.println("ECE631 Final Lab");

  for (int i = 0; i < sampleSize; i++)
    Buffer.unshift(0);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() 
{

  if (!client.connected())
  {
    reconnect();
  }
  
  client.loop();

  printFinish = millis();

  digitalWrite(trigger, state);

  if (state == HIGH && prevState == LOW)
  {
    buffStart = millis();
    prevState = HIGH;
    
  }
    

  else if (state == LOW && prevState == HIGH)
  {
    buffFinish = millis() - buffStart;
    prevState = LOW;

    distance = (buffFinish / 1000.0) * (13503.9 / 2.0)/(float)sampleSize;  // distance in inches
    Buffer.unshift(distance);

    totalDistance = 0;

    for (int i = 0; i < sampleSize; i++)
      totalDistance = Buffer[i] + totalDistance;
    
  }

  unsigned long printTime = printFinish - printStart;

  if (printTime >= 1000)
  {
    Serial.print("Distance: ");
    Serial.print(totalDistance);
    Serial.println(" in");

    doc["Distance"] = totalDistance;
    doc["units"] = "inches";

    String payload;
    serializeJson(doc, payload);

    client.publish("/ece631/Final/Input", payload.c_str());
    
    printStart = millis();
  }

  digitalWrite(r1, HIGH);
  digitalWrite(r2, HIGH);
  digitalWrite(r3, HIGH);
  digitalWrite(r4, HIGH);

  buttonFinish = millis();
   
  if ((digitalRead(c1) == HIGH || digitalRead(c2) == HIGH || digitalRead(c3) == HIGH) && buttonPressed == false)
  {

    digitalWrite(r1, LOW);
    digitalWrite(r2, LOW);
    digitalWrite(r3, LOW);
    digitalWrite(r4, LOW);
    
    buttonPressed = true;
    char keyVal = keypad();

    if (keyVal != ' ')
    {
      Serial.print("KeyVal = ");
      Serial.println(keyVal);
    }

    if (keyVal == ' ')
      buttonPressed = false;

    else if (keyVal == '*')
    {
      passcode = "";
      newPasscode = "";
      keyIndex = 0;
      isNewPass = false;

      Serial.println("-----Reset To Default Input-----");
    }
    
    else if (keyVal == '#')
    {
      isNewPass = true;
      keyIndex = 0;
      passcode = "";
      newPasscode = "";
      Serial.println("-----Reset To New Password Input-----");
    }
      
    else if (isNewPass)
    {
      newPasscode += keyVal;
      keyIndex++;
    }
    
    else if (keyIndex < 6)
    {
      passcode += keyVal;
      keyIndex++;
    }

    else if(keyIndex >= 6)
      keyIndex = 6;
  }

  else if (digitalRead(c1) == LOW && digitalRead(c2) == LOW && digitalRead(c3) == LOW && buttonPressed == true)
  {
    buttonStart = millis();
    buttonPressed = false;
  }

  inputFinish = millis();
  
  if (keyIndex >= 6 && inputFinish >= inputStart + 5000)
  {
    keyIndex = 0;

    if (!isNewPass)
    {
      pass["Password"] = passcode;
  
      String payload;
      serializeJson(pass, payload);
      client.publish("/ece631/Final/Input", payload.c_str());
      Serial.print("Password: ");
      Serial.println(passcode);
  
      passcode = "";
    }

    else
    {
      newPass["New Password"] = newPasscode;

      String payload;
      serializeJson(newPass, payload);
      client.publish("/ece631/Final/Input", payload.c_str());
      Serial.print("New Password: ");
      Serial.println(newPasscode);

      newPasscode = "";
      isNewPass = false;
    }

    inputStart = millis();
  }

  else if (keyIndex > 0 && keyIndex <= 6 && inputFinish <= inputStart + 5000)
  {
    keyIndex = 0;
    passcode = "";
    newPasscode = "";

    Serial.println("Invaid Input. Garage door opening or closing.");
  }
}
