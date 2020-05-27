#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define GATE_RELAY 5
#define GATE_CLOSED_SENSOR 4

//--------WIFI-----------------
const char* ssid = "";
const char* password = "";
const IPAddress ip(192,168,2,201);
const IPAddress dns(192,168,2,21);
const IPAddress gateway(192,168,2,1);
const IPAddress subnet(255,255,255,0);
//--------WIFI--------------------

//---------IBMCLOUD-----------
#define ORG ""
#define DEVICE_TYPE ""
#define DEVICE_ID ""
#define TOKEN ""

char server[] = ORG".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;
//---------IBMCLOUD------------

//---------Topics---------------
const char publishTopic[] = "iot-2/evt/status/fmt/json";

  //----------Commands----------
    const char gateSwitchTopic[] = "iot-2/cmd/gateswitch/fmt/text";
  //----------Commands----------

  //----------Events----------
    const char statusTopic[] = "iot-2/evt/status/fmt/json";
  //----------Events----------
//---------Topics---------------

void callback(char* topic, byte* payload, unsigned int payloadLength);

WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

int publishInterval = 60000; // 60 seconds
long lastPublishMillis;
int gate_status;  // 0: Closed 1: Open

void setup() {
  Serial.begin(115200); Serial.println();
//----------Pin setup------------------
  pinMode(GATE_RELAY, OUTPUT);
  digitalWrite(GATE_RELAY, HIGH);

  pinMode(GATE_CLOSED_SENSOR, INPUT);
  gate_status = digitalRead(GATE_CLOSED_SENSOR);
//----------Pin setup------------------
  wifiConnect();
  mqttConnect();
  initTopics();
}

void loop() {
  if (millis() - lastPublishMillis > publishInterval) {
    lastPublishMillis = millis();
    publishData();
  }
  if(gate_status != digitalRead(GATE_CLOSED_SENSOR)) {
    gate_status = digitalRead(GATE_CLOSED_SENSOR);
    publishData();
  }
  if(!client.loop()) {
    mqttConnect();
    initTopics();
  }
}

void wifiConnect() {
 Serial.print("Connecting to "); Serial.print(ssid);
 WiFi.mode(WIFI_STA);
 WiFi.config(ip, gateway, subnet, dns);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
 Serial.print("nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void mqttConnect() {
 if (!!!client.connected()) {
   Serial.print("Reconnecting MQTT client to "); Serial.println(server);
   while (!client.connect(clientId, authMethod, token)) {
     Serial.print("Wait...");
     delay(500);
   }
   Serial.println();
 }
}
void subscribeTo(const char* topic) {
 Serial.print("subscribe to "); Serial.print(topic);
 if (client.subscribe(topic)) {
   Serial.println(" OK");
 } else {
   Serial.println(" FAILED");
 }
}

void initTopics() {
  subscribeTo(gateSwitchTopic);
}
void publishData() {
  String payload = "{\"d\":{\"gate_status\":";
  payload += gate_status?"\"Open\"":"\"Closed\"";
  payload += "}}";
  
  Serial.print("Sending payload: "); Serial.println(payload);

  if(client.publish(publishTopic, (char*) payload.c_str())) {
    Serial.println("Publish OK");
  } else {
    Serial.println("Publish FAILED");
  }
}
void callback(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.println("Callback invoked for topic: "); Serial.print(topic);
  if (strcmp(gateSwitchTopic, topic) == 0) {
    gateSwitch();
  }
}

void gateSwitch() {
  digitalWrite(GATE_RELAY,LOW);
  delay(100);
  digitalWrite(GATE_RELAY, HIGH);
}
