#include <Arduino.h>
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 256
 
#include "TinyGsmClient.h" //https://github.com/vshymanskyy/TinyGSM
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

// load DigiCert Global Root CA ca_cert
const char* ca_cert= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=" \
"-----END CERTIFICATE-----\n";

void callback(char* topic, byte* payload, unsigned int length);
void mqtt_send_lamp_status();
void toggle_lamp_sts();
void button_action();
void startTimer();
void stopTimer();
void setup_sw1();
void setup_sw2();
void setup_sw3();
void setup_sw4();
void setup_sw5();
void setup_sw6();

static uint8_t gpio_relay1 = 14;
static uint8_t gpio_relay2 = 27;
static uint8_t gpio_relay3 = 32;
static uint8_t gpio_relay4 = 33;
static uint8_t gpio_relay5 = 26;
static uint8_t gpio_relay6 = 25;

const char* ssid = "DarkNet";
const char* password = "xwelcomexxx";
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* mqttUser = "emqx";
const char* mqttPassword = "public";

const char* mqttTopic_config = "homeassistant/switch/irrigation/config";

const char* mqttTopic_set1 = "homeassistant/switch/light1/set";
const char* mqttTopic_set2 = "homeassistant/switch/light2/set";
const char* mqttTopic_set3 = "homeassistant/switch/light3/set";
const char* mqttTopic_set4 = "homeassistant/switch/light4/set";
const char* mqttTopic_set5 = "homeassistant/switch/light5/set";
const char* mqttTopic_set6 = "homeassistant/switch/light6/set";

const char* mqttTopic_status1 = "homeassistant/switch/light1/state";
const char* mqttTopic_status2 = "homeassistant/switch/light2/state";
const char* mqttTopic_status3 = "homeassistant/switch/light3/state";
const char* mqttTopic_status4 = "homeassistant/switch/light4/state";
const char* mqttTopic_status5 = "homeassistant/switch/light5/state";
const char* mqttTopic_status6 = "homeassistant/switch/light6/state";


String payload; // Declare payload as a global variable
String result;
String setSwitchName(const String& switchType, const String& switchName) {
  result = "{\"name\":\"" + switchName + "\",\"command_topic\":\"homeassistant/switch/" + switchType + "/set\",\"state_topic\":\"homeassistant/switch/" + switchType + "/state\",\"unique_id\":\"" + switchType + "01ad\",\"device\":{\"identifiers\":[\"" + switchType + "01ad\"],\"name\":\"" + switchName + "\"}}";
  return result;
}

#define LED 2
#define RELE 19
#define BUTTON 35
#define RELE_ON HIGH
#define RELE_OFF LOW
#define LED_ON HIGH
#define LED_OFF LOW
#define sim800 Serial2


//WiFiClientSecure espClient;
//PubSubClient client(espClient);

TinyGsm modem(sim800);
TinyGsmClient espClient(modem);
PubSubClient client(espClient);

// Your GPRS credentials, if any
const char apn[] = "mobitel3g"; // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
const char gprsUser[] = "";
const char gprsPass[] = "";



//--------- WIFI -------------------------------------------

void wifi_connect() {
  Serial.print("Starting connecting WiFi.");
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//------------------ MQTT ----------------------------------
void mqtt_setup() {
 // set root ca cert
  //espClient.setCACert(ca_cert);
  client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    Serial.println("Connecting to MQTT…");
    while (!client.connected()) {        
        String clientId = "emqx_cloud50a308";
      //clientId += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", clientId.c_str());
        if (client.connect(clientId.c_str(), mqttUser, mqttPassword )) {
            Serial.println("connected");
        } else {
            Serial.print("failed with state  ");
            Serial.println(client.state());
            delay(2000);
        }
    }

    mqtt_send_lamp_status();
    client.subscribe(mqttTopic_set1);
    client.subscribe(mqttTopic_set2);
    client.subscribe(mqttTopic_set3);
    client.subscribe(mqttTopic_set4);
    client.subscribe(mqttTopic_set5);
    client.subscribe(mqttTopic_set6);
}

void mqtt_send_lamp_status() {   
  digitalWrite(gpio_relay1, HIGH);
  digitalWrite(gpio_relay2, HIGH);
  digitalWrite(gpio_relay3, HIGH);
  digitalWrite(gpio_relay4, HIGH);
  digitalWrite(gpio_relay5, HIGH);
  digitalWrite(gpio_relay6, HIGH);

  client.publish(mqttTopic_status1, "OFF");
  client.publish(mqttTopic_status2, "OFF");
  client.publish(mqttTopic_status3, "OFF");
  client.publish(mqttTopic_status4, "OFF");
  client.publish(mqttTopic_status5, "OFF");
  client.publish(mqttTopic_status6, "OFF");

  /*int val = digitalRead(RELE);
  Serial.printf("Sending LAMP status: ");
  if(val == RELE_OFF) {
    Serial.println("OFF");
    digitalWrite(LED, LED_OFF);
    client.publish(mqttTopic_status, "OFF");
  } else {
    Serial.println("ON");
    digitalWrite(LED, LED_ON);
    client.publish(mqttTopic_status, "ON");
  } */
}

void callback(char* topic, byte* payload, unsigned int length) {

    Serial.print("Message arrived in topic: ");
    Serial.println(topic);

    String byteRead = "";
    Serial.print("Message: ");
    for (int i = 0; i < length; i++) {
        byteRead += (char)payload[i];
    }    
    Serial.println(byteRead);

  if (strncmp(mqttTopic_set1,topic,33)==0)
	{
    if (byteRead == "OFF"){
        Serial.println("LAMP2 OFF");
        digitalWrite(gpio_relay1, HIGH);
         client.publish(mqttTopic_status1, "OFF",true);
    }

    if (byteRead == "ON"){
        Serial.println("LAMP1 ON");
        digitalWrite(gpio_relay1, LOW);
         client.publish(mqttTopic_status1, "ON",true);
    }
  }
  if (strncmp(mqttTopic_set2,topic,33)==0)
	{
    if (byteRead == "OFF"){
        Serial.println("LAMP2 OFF");
        digitalWrite(gpio_relay2, HIGH);
         client.publish(mqttTopic_status2, "OFF",true);
    }

    if (byteRead == "ON"){
        Serial.println("LAMP2 ON");
        digitalWrite(gpio_relay2, LOW);
         client.publish(mqttTopic_status2, "ON",true);
    }
  }
  if (strncmp(mqttTopic_set3,topic,33)==0)
	{
    if (byteRead == "OFF"){
        Serial.println("LAMP3 OFF");
        digitalWrite(gpio_relay3, HIGH);
         client.publish(mqttTopic_status3, "OFF",true);
    }

    if (byteRead == "ON"){
        Serial.println("LAMP3 ON");
        digitalWrite(gpio_relay3, LOW);
         client.publish(mqttTopic_status3, "ON",true);
    }
  }
  if (strncmp(mqttTopic_set4,topic,33)==0)
	{
    if (byteRead == "OFF"){
        Serial.println("LAMP4 OFF");
        digitalWrite(gpio_relay4, HIGH);
         client.publish(mqttTopic_status4, "OFF",true);
    }

    if (byteRead == "ON"){
        Serial.println("LAMP4 ON");
        digitalWrite(gpio_relay4, LOW);
         client.publish(mqttTopic_status4, "ON",true);
    }
  }
  if (strncmp(mqttTopic_set5,topic,33)==0)
	{
    if (byteRead == "OFF"){
        Serial.println("LAMP5 OFF");
        digitalWrite(gpio_relay5, HIGH);
         client.publish(mqttTopic_status5, "OFF",true);
    }

    if (byteRead == "ON"){
        Serial.println("LAMP5 ON");
        digitalWrite(gpio_relay5, LOW);
         client.publish(mqttTopic_status5, "ON",true);
    }
  }
  if (strncmp(mqttTopic_set6,topic,33)==0)
	{
    if (byteRead == "OFF"){
        Serial.println("LAMP6 OFF");
        digitalWrite(gpio_relay6, HIGH);
         client.publish(mqttTopic_status6, "OFF",true);
    }

    if (byteRead == "ON"){
        Serial.println("LAMP6 ON");
        digitalWrite(gpio_relay6, LOW);
         client.publish(mqttTopic_status6, "ON",true);
    }
  }

    

    

    Serial.println();
    Serial.println("---------");

}

//----------- IOs -------------------------------------------
void setup_ios() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(gpio_relay1, OUTPUT);
  pinMode(gpio_relay2, OUTPUT);
  pinMode(gpio_relay3, OUTPUT);
  pinMode(gpio_relay4, OUTPUT);
  pinMode(gpio_relay5, OUTPUT);
  pinMode(gpio_relay6, OUTPUT);

  digitalWrite(RELE, RELE_OFF);
}

//---------------- INTERRUPTS ----------------------------
struct Button {
    const uint8_t PIN;
    uint32_t numberKeyPresses;
    bool pressed;
};

Button button = {BUTTON, 0, false};

void IRAM_ATTR isr() {
    static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 200ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 200){
   button.pressed = true;
 }
 last_interrupt_time = interrupt_time;
}

void config_interrupts() {
  attachInterrupt(button.PIN, isr, FALLING);
}

//----------- GENERAL -------------------------------------
void toggle_lamp_sts() {
  int val = digitalRead(RELE);
  digitalWrite(RELE, !val);
  digitalWrite(LED, val);
}

void button_action() {
  if (button.pressed) {
    toggle_lamp_sts();      
    mqtt_send_lamp_status();
    button.pressed = false;
  }
}

//--------- ARDUINO --------------------------------------
void setup() {  
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser,gprsPass))
  {
    Serial.println(" fail");
    delay(1000);
    modem.restart();
    return;
  }
  Serial.println(" OK"); 
  setup_ios();  
 // wifi_connect();
  mqtt_setup();  
  setup_sw1();
  setup_sw2();
  setup_sw3();
  setup_sw4();
  setup_sw5();
  setup_sw6();
  //startTimer();
  //config_interrupts();
}

void loop() {
    if (!client.connected()) {
    //reconnect();
  }
    client.loop();
    button_action();
}


void setup_sw1(){
  // Change these values to modify the JSON entity name and type
  const char* entityName = "light1"; 
  const char* entityType = "switch";

  StaticJsonDocument<256> doc;
  doc["name"] = entityName;
  doc["command_topic"] = "homeassistant/switch/light1/set";
  doc["state_topic"] = "homeassistant/switch/light1/state";
  doc["unique_id"] = "irr01ad1";
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"].add("garden01ad1");
  device["name"] = "Lobby";

  char buffer[256];
  serializeJson(doc, buffer);
  const char* mqttTopic_config = "homeassistant/switch/light1/config";
  client.publish(mqttTopic_config, buffer,true);
  delay(50); // Adjust the delay based on your requirements
}

void setup_sw2(){
  // Change these values to modify the JSON entity name and type
  const char* entityName = "light2"; 
  const char* entityType = "switch";

  StaticJsonDocument<256> doc;
  doc["name"] = entityName;
  doc["command_topic"] = "homeassistant/switch/light2/set";
  doc["state_topic"] = "homeassistant/switch/light2/state";
  doc["unique_id"] = "irr01ad2";
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"].add("garden01ad2");
  device["name"] = "Lobby";

  char buffer[256];
  serializeJson(doc, buffer);
  const char* mqttTopic_config = "homeassistant/switch/light2/config";
  client.publish(mqttTopic_config, buffer,true);
  delay(50); // Adjust the delay based on your requirements
}

void setup_sw3(){
  // Change these values to modify the JSON entity name and type
  const char* entityName = "light3"; 
  const char* entityType = "switch";

  StaticJsonDocument<256> doc;
  doc["name"] = entityName;
  doc["command_topic"] = "homeassistant/switch/light3/set";
  doc["state_topic"] = "homeassistant/switch/light3/state";
  doc["unique_id"] = "irr01ad3";
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"].add("garden01ad3");
  device["name"] = "Lobby";

  char buffer[256];
  serializeJson(doc, buffer);
  const char* mqttTopic_config = "homeassistant/switch/light3/config";
  client.publish(mqttTopic_config, buffer,true);
  delay(50); // Adjust the delay based on your requirements
}

void setup_sw4(){
  // Change these values to modify the JSON entity name and type
  const char* entityName = "light4"; 
  const char* entityType = "switch";

  StaticJsonDocument<256> doc;
  doc["name"] = entityName;
  doc["command_topic"] = "homeassistant/switch/light4/set";
  doc["state_topic"] = "homeassistant/switch/light4/state";
  doc["unique_id"] = "irr01ad4";
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"].add("garden01ad4");
  device["name"] = "Lobby";

  char buffer[256];
  serializeJson(doc, buffer);
  const char* mqttTopic_config = "homeassistant/switch/light4/config";
  client.publish(mqttTopic_config, buffer,true);
  delay(50); // Adjust the delay based on your requirements
}

void setup_sw5(){
  // Change these values to modify the JSON entity name and type
  const char* entityName = "light5"; 
  const char* entityType = "switch";

  StaticJsonDocument<256> doc;
  doc["name"] = entityName;
  doc["command_topic"] = "homeassistant/switch/light5/set";
  doc["state_topic"] = "homeassistant/switch/light5/state";
  doc["unique_id"] = "irr01ad5";
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"].add("garden01ad5");
  device["name"] = "Lobby";

  char buffer[256];
  serializeJson(doc, buffer);
  const char* mqttTopic_config = "homeassistant/switch/light5/config";
  client.publish(mqttTopic_config, buffer,true);
  delay(50); // Adjust the delay based on your requirements
}

void setup_sw6(){
  // Change these values to modify the JSON entity name and type
  const char* entityName = "light6"; 
  const char* entityType = "switch";

  StaticJsonDocument<256> doc;
  doc["name"] = entityName;
  doc["command_topic"] = "homeassistant/switch/light6/set";
  doc["state_topic"] = "homeassistant/switch/light6/state";
  doc["unique_id"] = "irr01ad6";
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"].add("garden01ad6");
  device["name"] = "Lobby";

  char buffer[256];
  serializeJson(doc, buffer);
  const char* mqttTopic_config = "homeassistant/switch/light6/config";
  client.publish(mqttTopic_config, buffer,true);
  delay(50); // Adjust the delay based on your requirements
}