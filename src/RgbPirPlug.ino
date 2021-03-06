#include <Espiot.h>

#include <Adafruit_Sensor.h>
#include <DallasTemperature.h>
#include <OneWire.h>

Espiot espiot;

String sensorData = "";
int sensorValue;

// CONF
String ver = "1.0.5";
long lastTime = millis();

// DS18B20
#define ONE_WIRE_BUS 13
#define TEMPERATURE_PRECISION 12 // 8 9 10 12

float temp = NULL;

// PIR
#define PIRPIN 4

// RGB
#define REDPIN 16
#define GREENPIN 12
#define BLUEPIN 14

int red = 1023;
int green = 1023;
int blue = 1023;
int factor = 1;
boolean lightUp = false;

#define BUILTINLED 2

String MODE = "AUTO";
boolean sentMsg = false;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int devicesFound = 0;
DeviceAddress devices[10];

void setup() { //------------------------------------------------
  Serial.begin(115200);

  pinMode(PIRPIN, INPUT);
  pinMode(BUILTINLED, OUTPUT);

  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);

  analogWrite(REDPIN, 1023);
  analogWrite(GREENPIN, 1023);
  analogWrite(BLUEPIN, 1023);

  Serial.println("PirPlug ...v" + String(ver));
  delay(300);

  Serial.print("\nPWMRANGE: ");
  Serial.println(PWMRANGE);

  espiot.init(ver);
  // espiot.enableVccMeasure();
  espiot.SENSOR = "PIR,DS18B20";

  delay(300);
  sensors.begin();

  int statusCode;

  espiot.server.on("/info", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["PIRPIN"] = PIRPIN;
    root["REDPIN"] = REDPIN;
    root["BLUEPIN"] = BLUEPIN;
    root["GREENPIN"] = GREENPIN;

    root["ONE_WIRE_BUS"] = ONE_WIRE_BUS;
    root["TEMPERATURE_PRECISION"] = TEMPERATURE_PRECISION;

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/state", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;

    root["temp"] = temp;
    root["light"] = sensorValue;

    JsonObject &rgb = root.createNestedObject("rgb");
    rgb["red"] = red;
    rgb["green"] = green;
    rgb["blue"] = blue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/rgb", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;

    root["red"] = red;
    root["green"] = green;
    root["blue"] = blue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/rgb", HTTP_OPTIONS, []() {
    espiot.blink();
    espiot.server.sendHeader("Access-Control-Allow-Methods",
                             "POST,GET,OPTIONS");
    espiot.server.sendHeader("Access-Control-Allow-Headers",
                             "Origin, X-Requested-With, Content-Type, Accept");
    espiot.server.send(200, "text/plain", "");
  });

  espiot.server.on("/rgb", HTTP_PUT, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(espiot.server.arg("plain"));

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;

    red = root["red"];
    green = root["green"];
    blue = root["blue"];

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  sensors.requestTemperatures();
  getDeviceAddress();

} //--

// -----------------------------------------------------------------------------
// loop ------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void loop() {
  yield();
digitalWrite(BUILTINLED, HIGH);
  espiot.loop();

  sensorValue = analogRead(A0); // read analog input pin 0

  temp = sensors.getTempCByIndex(0);

  yield();

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["id"] = espiot.getDeviceId();
  root["name"] = espiot.deviceName;
  root["temp"] = temp;

  root["light"] = sensorValue;
  root["motion"] = NULL;

  yield();

  int inputState = digitalRead(PIRPIN);
  if (MODE == "AUTO") {
    if (inputState == HIGH) {
      Serial.println(F("Sensor high ... "));
      // Serial.println("espiot.lightThreshold=" +
      // String(espiot.lightThreshold));

      if (sensorValue < espiot.lightThreshold) {
        Serial.println(F("Light on ... "));
        lightUp = true;
      }

      lastTime = millis();
      if (!sentMsg) {
        String content;
        root["motion"] = true;
        root.printTo(content);
        //espiot.mqPublishSubTopic(content, "motion");
        sentMsg = true;
      }
    } else {
      root["motion"] = false;
    }
  }

  if (millis() > lastTime + espiot.timeOut && inputState != HIGH) {

    sentMsg = false;
    Serial.println("timeout... " + String(lastTime));
    Serial.print("Light: ");
    Serial.println(sensorValue, DEC);

    String content;
    root.printTo(content);
  //  espiot.mqPublish(content);

    lastTime = millis();
    lightUp = false;
  }

  yield();
  fade();

} //---------------------------------------------------------------

void fade() {
  if (lightUp)
    fadeIn();
  else
    fadeOut();
}

void fadeIn() {
  red = 1023;
  red = red - factor;
  if (red < 0)
    red = 0;

  blue = blue - factor;
  if (blue < 500)
    blue = 500;

  green = 1023;
  green = green - factor;
  if (green < 0)
    green = 0;

  digitalWrite(BUILTINLED, HIGH);
  analogWrite(BLUEPIN, blue);
  analogWrite(GREENPIN, green);
  analogWrite(REDPIN, red);
  delay(20);
}

void fadeOut() {

  red = 1023;
  red = red + factor;
  if (red > 1023)
    red = 1023;

  blue = blue + factor;
  if (blue > 1023)
    blue = 1023;

  green = 1023;
  green = green + factor;
  if (green > 1023)
    green = 1023;

  digitalWrite(BUILTINLED, HIGH);
  analogWrite(BLUEPIN, blue);
  analogWrite(GREENPIN, green);
  analogWrite(REDPIN, red);
  delay(200);
}

void getDeviceAddress(void) {

  Serial.println("\nGetting the address...");

  devicesFound = sensors.getDeviceCount();
  Serial.print("Num devices: ");
  Serial.println(devicesFound);

  for (int i = 0; i < devicesFound; i++)
    if (!sensors.getAddress(devices[i], i))
      Serial.println("Unable to find address for Device" + i);

  for (int i = 0; i < devicesFound; i++) {
    Serial.print("\nDevice " + (String)i + " Address: ");
    printAddress(devices[i]);
    Serial.println(printTemperature(devices[i]));
  }

  for (int i = 0; i < devicesFound; i++)
    sensors.setResolution(devices[i], TEMPERATURE_PRECISION);

  return;
}

String printAddress(DeviceAddress deviceAddress) {
  String out = "";
  for (uint8_t i = 0; i < 8; i++) {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    out += (String)deviceAddress[i];
  }
  return out;
}

String printTemperature(DeviceAddress deviceAddress) {
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC < 10)
    return "0" + (String)tempC;
  else
    return (String)tempC;
}
