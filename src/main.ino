#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "config.h"

// Set web server and wifi client
WiFiServer server(80);
WiFiClient wifiClient;
WiFiManager wifiManager;
PubSubClient client(wifiClient);

// Definition of public variables
int longPressCounter = 0;
int lightFlashCounter = 0;
int whiteFlashStatus = 0;
int reconnectCounter = CHECK_CONNETCTION_ITERATIONS;
int rolloutCounter = 0;
int waterButtonState, lowSpeedState, highSpeedState;
bool isLowSpeedPressed = false;
bool isHighPressed = false;
bool isWaterPresed = false;
bool isConnected = false;

void setup() {
  // Setup pins
  pinMode(lowSpeedRelay, OUTPUT);
  pinMode(waterButtonRelay, OUTPUT);
  pinMode(highSpeedRelay, OUTPUT);
  pinMode(linkLed, OUTPUT);

  pinMode(waterButtonSensor, INPUT);
  pinMode(lowSpeedSensor, INPUT);
  pinMode(highSpeedSensor, INPUT);

  delay(10);

  digitalWrite(linkLed, HIGH);
  digitalWrite(lowSpeedRelay, LOW);
  digitalWrite(waterButtonRelay, LOW);
  digitalWrite(highSpeedRelay, LOW);

  // Setup serial
  if (serialEnabled)
  {
    Serial.begin(serialBuadrate);
  }

  Serial.println("[ INFO ] Init WiFi");

  // If you want to erase all the stored information
  if (resetWifiEnabled) {
      wifiManager.resetSettings();
  }

  // Add token field to the portal
  WiFiManagerParameter device_token_field("token", "Device credential", TOKEN, 20);

  wifiManager.addParameter(&device_token_field);
  wifiManager.setConfigPortalTimeout(configPortalTimeout);
  wifiManager.setDebugOutput(false);
  // wifiManager.setSaveParamsCallback(saveParamsCallback);

  //automatically connect using saved credentials if they exist
  //If connection fails it starts an access point with the specified name
  if (autoGenerateSSID) {
    // Use this for auto generated name ESP + ChipID
    wifiManager.autoConnect();
  } else {
    wifiManager.autoConnect(DEVICE_NAME);
  }

  // if you get here you have connected to the WiFi
  Serial.println("[ INFO ] Connected to WiFi: ");

  // Read token from EEPROM
  // if(!statictoken) {
  //     EEPROM.get(0, TOKEN);
  // }

  // Connect to the server
  server.begin();
  client.setServer(dmiotServer, mqttPort);
  client.setCallback(on_message);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("[ INFO ] Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveParamsCallback () {
  // Save token into EEPROM
  // if(!statictoken){
  //   strcpy(TOKEN, device_token_field.getValue());
  //   EEPROM.put(0, TOKEN);
  //   EEPROM.commit();
  // }
}

void startConfigPortal() {
  digitalWrite(waterButtonRelay, HIGH);
  digitalWrite(highSpeedRelay, LOW);
  digitalWrite(lowSpeedRelay, LOW);
  delay(3000);
  wifiManager.erase();
  ESP.restart();
}

void loop() {
  // ============================ Left button =================================
  waterButtonState = digitalRead(waterButtonSensor);

  if (waterButtonState == HIGH)
    isWaterPresed = false;

  if (waterButtonState == LOW && !isWaterPresed) {
    isWaterPresed = true;
    if (gpioState[0] == false)
    {
      digitalWrite(waterButtonRelay, HIGH);

      // Update GPIOs state
      gpioState[0] = true;
    }
    else {
      digitalWrite(waterButtonRelay, LOW);

      // Update GPIOs state
      gpioState[0] = false;
    }
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
  }

  // ============================ Middle button =================================
  lowSpeedState = digitalRead(lowSpeedSensor);

  if (lowSpeedState == HIGH)
    isLowSpeedPressed = false;
  else {
    // This part of code is for detecting long press
    if ( lowSpeedState == LOW ) {
      longPressCounter++;
    }
    else {
      longPressCounter = 0;
    }

    if (longPressCounter == LONG_PRESS_ITERATIONS) {
      Serial.println("[ INFO ] Long press detected, starting Config Portal");

      startConfigPortal();

      longPressCounter = 0;
    }
  }

  if (lowSpeedState == LOW && !isLowSpeedPressed) {
    isLowSpeedPressed = true;
    if (gpioState[1] == false)
    {
      digitalWrite(highSpeedRelay, LOW);
      delay(10);
      digitalWrite(lowSpeedRelay, HIGH);

      // Update GPIOs state
      gpioState[1] = true;
      gpioState[2] = false;
    }
    else {
      digitalWrite(lowSpeedRelay, LOW);

      // Update GPIOs state
      gpioState[1] = false;
    }
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
  }

  // ============================ Right button =================================
  highSpeedState = digitalRead(highSpeedSensor);

  if (highSpeedState == HIGH)
    isHighPressed = false;

  if (highSpeedState == LOW && !isHighPressed) {
    isHighPressed = true;
    if (gpioState[2] == false)
    {
      digitalWrite(lowSpeedRelay, LOW);
      delay(10);
      digitalWrite(highSpeedRelay, HIGH);

      // Update GPIOs state
      gpioState[1] = false;
      gpioState[2] = true;
    }
    else {
      digitalWrite(highSpeedRelay, LOW);

      // Update GPIOs state
      gpioState[2] = false;
    }
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
  }

  // Check for connection
  if (reconnectCounter >= CHECK_CONNETCTION_ITERATIONS) {
    if ( !client.connected() )
    {
      reconnect();
    }

    reconnectCounter = 0;
  }

  client.loop();
  reconnectCounter++;

  delay(1);
}

// The callback for when a PUBLISH message is received from the server.
void on_message(const char* topic, byte* payload, unsigned int length) {

  Serial.println("On message");

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("[ INFO ] Topic: ");
  Serial.println(topic);
  Serial.print("[ INFO ] Message: ");
  Serial.println(json);

  // Decode JSON request
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success())
  {
    Serial.println("[ EROR ] parseObject() failed");
    return;
  }

  // Check request method
  String methodName = String((const char*)data["method"]);

  if (methodName.equals("getGpioStatus")) {

    // Reply with GPIO status
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
    client.publish(responseTopic.c_str(), get_gpio_status().c_str());
  } else if (methodName.equals("setGpioStatus")) {
    // Update GPIO status and reply
    set_gpio_status(data["params"]["pin"], data["params"]["enabled"]);

    String responseTopic = String(topic);

    responseTopic.replace("request", "response");
    client.publish(responseTopic.c_str(), get_gpio_status().c_str());
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
  }
}

// Get GPIO RPC
String get_gpio_status() {

  // Prepare gpios JSON payload string
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();

  data[String(1)] = gpioState[0] ? true : false;
  data[String(2)] = gpioState[1] ? true : false;
  data[String(3)] = gpioState[2] ? true : false;

  char payload[256];
  data.printTo(payload, sizeof(payload));
  String strPayload = String(payload);
  Serial.print("Get gpio status: ");
  Serial.println(strPayload);
  return strPayload;
}

// Set GPIO RPC
void set_gpio_status(int pole, boolean enabled) {
  if (pole == 1) {
    if (enabled)
    {
      digitalWrite(waterButtonRelay, HIGH);
    }
    else
    {
      digitalWrite(waterButtonRelay, LOW);
    }

    // Update GPIOs state
    gpioState[0] = enabled;
  }

  if (pole == 2) {
    if (enabled)
    {
      digitalWrite(highSpeedRelay, LOW);
      delay(10);
      digitalWrite(lowSpeedRelay, HIGH);

      // Update GPIOs state
      gpioState[1] = enabled;
      gpioState[2] = !enabled;
    }
    else
    {
      digitalWrite(lowSpeedRelay, LOW);

      // Update GPIOs state
      gpioState[1] = enabled;
    }
  }

  if (pole == 3) {
    if (enabled)
    {
      digitalWrite(lowSpeedRelay, LOW);
      delay(10);
      digitalWrite(highSpeedRelay, HIGH);

      // Update GPIOs state
      gpioState[1] = !enabled;
      gpioState[2] = enabled;
    }
    else
    {
      digitalWrite(highSpeedRelay, LOW);

      // Update GPIOs state
      gpioState[2] = enabled;
    }
  }
}

// Reconnect to the server function
void reconnect() {
  // Connecting to the server
  Serial.println("[ INFO ] Connecting to DMIoT server ...");

  // Attempt to connect (clientId, username, password)
  if ( client.connect(DEVICE_NAME, TOKEN, NULL) ) {
    Serial.println( "[ INFO ] Connected to the server" );

    digitalWrite(linkLed, LOW);
    isConnected = true;

    // Subscribing to receive RPC requests
    client.subscribe("v1/devices/me/rpc/request/+");

    // Sending current GPIO status
    Serial.println("[ INFO ] Sending current GPIO status ...");
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
  } else {
    isConnected = false;
    Serial.print( "[FAILED] [ rc = " );
    Serial.print( client.state() );
    Serial.println( " : retrying in 5 seconds]" );
  }
}
