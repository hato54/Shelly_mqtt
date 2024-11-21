

/*  Program name: Shelly1 Manual
 *  This program is used for Shelly1 device to control 
 *  one input (switch) and one output (relay).
 *  The manual function is overriding the Home Assistant 
 *  settings so if the Home assistant is down You could
 *  switch the light on/off manually
 *  2021-02-12 Håkan Torén
 */

#define DEBUG   // This will use pullup on input and print messages


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define wifi_ssid "Ekeborg"
#define wifi_password "Mittekeborg54"
#define mqtt_server "192.168.1.36"
#define mqtt_user "ha"
#define mqtt_password "ha"

#define state_topic1 "test/shelly1/state"
#define inTopic1 "test/shelly1/command"
#define state_topic2 "test/shelly1/input"

#define buttonPin 5
#define relayPin 4
#define HOSTNAME Shellytest       // WiFi name

int buttonState = LOW;
int lastButtonState = LOW;
int relayState = LOW;

unsigned long lastDebounceTime = 0;
unsigned int debounceDelay = 100;

WiFiClient espClient;
PubSubClient client(espClient);
 

void setup_wifi(){
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(wifi_ssid);
  WiFi.hostname("HOSTNAME");
  WiFi.begin(wifi_ssid, wifi_password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi connected \n");
  Serial.print("IP adress: ");
  Serial.println(WiFi.localIP());
}

void reconnect(){
  // Loop until we are connected
  while(!client.connected()){
    Serial.print("Atempting MQTT connection ...");
    // Create random client ID
    String clientId = "MQTT_Relay-";
    clientId += String(random(0xffff),HEX);
    // Attempt to connect
    if(client.connect(clientId.c_str(), mqtt_user, mqtt_password)){
      Serial.println("connected");
      // Once connected, publish an announcement..
      client.publish(state_topic1, "OFF");
      if(digitalRead(buttonPin)){
        client.publish(state_topic2, "ON");  
      }else{
        client.publish(state_topic2, "OFF");  
      }
      // ... and resubscribe
      client.subscribe(inTopic1);
    }else{
      Serial.print("failed, rc = ");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    
  }
}

void callback(char* topic1,byte* payload, unsigned int length){
#ifdef  DEBUG
  Serial.print("Message arrived on topic: ");
  Serial.print(topic1);
  Serial.print(". Message: ");
#endif
  String messageTemp;
  
  for(unsigned int i=0;i<length;i++){
    messageTemp += (char)payload[i];
  }
#ifdef  DEBUG
  Serial.print("messageTemp=");Serial.println(messageTemp);
#endif
  if(String(topic1) == inTopic1){ 
    if(messageTemp == "ON"){
#ifdef  DEBUG
      Serial.print("Changing _output to ON\n");
#endif
      client.publish(state_topic1, "ON");
      relayState = HIGH;
      delay(200);
      
    }else if(messageTemp == "OFF"){
#ifdef  DEBUG
      Serial.print("Changing output to OFF\n");
#endif
      client.publish(state_topic1, "OFF");
      relayState = LOW;
      delay(200);
    }
    digitalWrite(relayPin, relayState);
  }
}

void setup() {
#ifdef  DEBUG
// Test on WMOS Mini
  pinMode(buttonPin,INPUT_PULLUP);
#else
  pinMode(buttonPin,INPUT);
#endif
  Serial.begin(115200);
  pinMode(relayPin,OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void loop(){
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  int reading = digitalRead(buttonPin);
  // check if you just pressed the button
  if(reading != lastButtonState){ 
    //reset debbouncing timer
    lastDebounceTime = millis();
  }
  if((millis() - lastDebounceTime) > debounceDelay){
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if(reading != buttonState){
      buttonState = reading;
      if(buttonState){
        client.publish(state_topic2, "ON");
#ifdef  DEBUG
        Serial.println((String)state_topic2 + " => ON");
#endif     
      }else{
        client.publish(state_topic2, "OFF");
#ifdef  DEBUG
        Serial.println((String)state_topic2 + " => OFF");
#endif        
      }
      relayState = !relayState;
      if(relayState){
        client.publish(state_topic1, "ON");
#ifdef  DEBUG
        Serial.print("Changing output to ON\n");
#endif
      }else{
        client.publish(state_topic1, "OFF");
#ifdef  DEBUG
        Serial.print("Changing output to OFF\n");   
#endif
      }
      digitalWrite(relayPin, relayState);
     
    }
  }
  lastButtonState = reading;
}




