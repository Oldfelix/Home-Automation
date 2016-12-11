#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

/* WiFi Settings */
const char* ssid     = "wings";
const char* password = "B!rd2016";

/* ESP9266 Outputs */
const int relayPin = 5;  // Active high
const int _pin = 16;

/* MQTT Settings */
const char* mqttTopic = "/house/external/garage/GD1";   // MQTT topic
IPAddress broker(10,10,128,234);          // Address of the MQTT broker
#define CLIENT_ID "client-sonoff1"         // Client ID to send to the broker

class Ultrasonic
{
  public:
    Ultrasonic(int pin);
        void DistanceMeasure(void);
    long microsecondsToCentimeters(void);
    long microsecondsToInches(void);
  private:
    int _pin;//pin number of Arduino that is connected with SIG pin of Ultrasonic Ranger.
        long duration;// the Pulse time received;
};

/**
 * MQTT callback to process messages
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Examine only the first character of the message
  if(payload[0] == 49)              // Message "1" in ASCII (turn outputs ON)
  {
//    digitalWrite(ledPin, LOW);      // LED is active-low, so this turns it on
    digitalWrite(relayPin, HIGH);
    
  } else if(payload[0] == 48)       // Message "0" in ASCII (turn outputs OFF)
  {
//    digitalWrite(ledPin, HIGH);     // LED is active-low, so this turns it off
    digitalWrite(relayPin, LOW);
    
  } else {
    Serial.println("Unknown value");
  }
  
}
WiFiClient wificlient;
PubSubClient client(wificlient);

/**
 * Attempt connection to MQTT broker and subscribe to command topic
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
Ultrasonic::Ultrasonic(int pin)
{
  _pin = pin;
}
/*Begin the detection and get the pulse back signal*/
void Ultrasonic::DistanceMeasure(void)
{
    pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(_pin, HIGH);
  delayMicroseconds(5);
  digitalWrite(_pin,LOW);
  pinMode(_pin,INPUT);
  duration = pulseIn(_pin,HIGH);
}
/*The measured distance from the range 0 to 400 Centimeters*/
long Ultrasonic::microsecondsToCentimeters(void)
{
  return duration/29/2; 
}
/*The measured distance from the range 0 to 157 Inches*/
long Ultrasonic::microsecondsToInches(void)
{
  return duration/74/2; 
}
Ultrasonic ultrasonic(16);

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("WiFi begun");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
}

  Serial.println("Proceeding");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR   ) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR  ) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR    ) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  /* Set up the outputs. LED is active-low */
//  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
//  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, LOW);

  /* Prepare MQTT client */
  client.setServer(broker, 1883);
  client.setCallback(callback);
}

void loop()
{
  long RangeInInches;
  long RangeInCentimeters;
  ultrasonic.DistanceMeasure();// get the current signal time;
    RangeInInches = ultrasonic.microsecondsToInches();//convert the time to inches;
  RangeInCentimeters = ultrasonic.microsecondsToCentimeters();//convert the time to centimeters
  Serial.println("The distance to obstacles in front is: ");
  Serial.print(RangeInInches);//0~157 inches
  Serial.println(" inch");
  Serial.print(RangeInCentimeters);//0~400cm
  Serial.println(" cm");
  delay(100);
   ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
  }
  
  if (client.connected())
  {
    client.loop();
}
}

