#include <Espiot.h>

#include <Adafruit_Sensor.h>
#include <DallasTemperature.h>
#include <OneWire.h>

Espiot espiot;

int lightTreshold = 0; // 0 - dark, >100 - light

String sensorData = "";
int sensorValue;

// CONF
String ver = "1.0.1";
long lastTime = millis();

//DS18B20
#define ONE_WIRE_BUS 15
#define TEMPERATURE_PRECISION 12 // 8 9 10 12

float temp = NULL;

// PIR
#define PIRPIN 4

//RGB
#define REDPIN 13
#define GREENPIN 12
#define BLUEPIN 14

int red = 1023;
int green = 1023;
int blue = 0;

#define BUILTINLED 2
#define RELEY 50

String MODE = "AUTO";
boolean sentMsg = false;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

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
  //espiot.enableVccMeasure();
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
    espiot.server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
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

} //--

// -----------------------------------------------------------------------------
// loop ------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void loop() {
  yield();

  espiot.loop();

  sensorValue = analogRead(A0); // read analog input pin 0

  sensors.requestTemperatures();
  temp = sensors.getTempC(0);

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
      Serial.println(F("Sensor high..."));
      digitalWrite(BUILTINLED, LOW);
      digitalWrite(RELEY, HIGH);

      analogWrite(BLUEPIN, blue);
      analogWrite(GREENPIN, green);
      analogWrite(REDPIN, red);

      lastTime = millis();
      if(!sentMsg){
        String content;
        root["motion"] = true;
        root.printTo(content);
        espiot.mqPublishSubTopic(content, "motion");
        sentMsg = true;
      }
    } else{
        root["motion"] = false;
    }
  }

  if (millis() > lastTime + espiot.timeOut && inputState != HIGH) {
    digitalWrite(RELEY, LOW);
    digitalWrite(BUILTINLED, HIGH);
    analogWrite(BLUEPIN, 1023);
    analogWrite(GREENPIN, 1023);
    analogWrite(REDPIN, 1023);

    sentMsg = false;
    Serial.println("timeout... " + String(lastTime));
    Serial.print("Light: ");
    Serial.println(sensorValue, DEC);

    String content;
    root.printTo(content);
    espiot.mqPublish(content);

    lastTime = millis();
  }

  yield();

} //---------------------------------------------------------------
