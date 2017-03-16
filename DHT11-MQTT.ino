/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com
  Arduino IDE example: Examples > Arduino OTA > BasicOTA.ino
*********/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

//#include <SPI.h>

#include <DHT.h>
#include <EEPROM.h>
#include <PubSubClient.h>

// Replace with your network credentials
const char* ssid = "hoving";
const char* password = "groningen";
const char* mqtt_server = "m12.cloudmqtt.com";
const char* clientID = "Squirrel-1";
const char* outTopic = "home/fridge/temp";
const char* inTopic = "home/fridge/topic"; //- See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8746#sthash.JgCr0pF5.dpuf
WiFiClient espClient;
PubSubClient client(espClient); //- See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8746#sthash.JgCr0pF5.dpuf

int relay_pin = 2;
bool relayState = LOW;

#define DHTPIN 0          // What digital pin we're connected to
#define DHTTYPE DHT11     // DHT 11
DHT dht(DHTPIN, DHTTYPE,11);
float humidity, temp_c;  // Values read from sensor
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor
char msg[50];


void callback(char* topic, byte* payload, unsigned int length) {
  // Conver the incoming byte array to a string
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;
 
  Serial.print("Message arrived on topic: [");
  Serial.print(topic);
  Serial.print("], ");
  Serial.println(message);

  if(message == "temperature, c"){
    gettemperature();
    Serial.print("Sending temperature:");
    Serial.println(temp_c);
    dtostrf(temp_c , 2, 2, msg);
    client.publish("home/fridge/temp", msg);
  } else if (message == "humidity"){
    gettemperature();
    Serial.print("Sending humidity:");
    Serial.println(humidity);
    dtostrf(humidity , 2, 2, msg);
    client.publish("home/fridge/humidity", msg);
  }  else if (message == "on" || message == "off" || message=="toggle" ){
    if (message == "off")
     relayState=HIGH;
    if (message == "on") 
     relayState=LOW;
    if (message="toggle")
      relayState=!relayState;
      
    digitalWrite(relay_pin,relayState);
    EEPROM.write(0, relayState);    // Write state to EEPROM
    EEPROM.commit();
    if (relayState)
      client.publish("home/fridge/switch", "off");
    else
      client.publish("home/fridge/switch", "on");
  }

}

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_c = dht.readTemperature();     // Read temperature as Celcius
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_c)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID, "meindert","jatkled147")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, clientID);
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}


void setup() {
  EEPROM.begin(512);              // Begin eeprom to store on/off state - See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8746#sthash.XN07AFwE.dpuf
  pinMode(relay_pin, OUTPUT);     // Initialize the relay pin as an output - See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8746#sthash.XN07AFwE.dpuf
  relayState = EEPROM.read(0);
  digitalWrite(relay_pin,relayState); 
  
  Serial.begin(9600);
  delay(1000);
  Serial.println("--------------------------------------");
  Serial.println("Booting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Ready");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 10341);
  client.setCallback(callback); // See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8746#sthash.JgCr0pF5.dpuf
  
  dht.begin();


}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}




