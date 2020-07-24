/******************************************************************************
 * Copyright 2018 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
 
#include <CloudIoTCore.h>
#include "esp8266_mqtt.h"
#include <Wire.h> // I2C communication
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

// Activate debug messages or not
#define DEBUG 1

// LED pin
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

// Sea level pressure for accurate altitude estimation
#define SEALEVELPRESSURE_HPA (1013.25)

// Initializing BME sensor
Adafruit_BME280 bme;

// Variables for the sensor's readings
float m_humidity;
float m_temperature;
float m_pressure;

// Generally, you should use "unsigned long" for variables that hold time
const unsigned long UPDATE_INTERVAL = 60 * 1000; // Time interval to send new data in milliseconds

// String to store the data packet
char msg_weather_data_to_MQTT[80];

void read_sensors() {
    readBME();
}

void readBME(){
  m_pressure = bme.readPressure()/1000.0;
  m_temperature = bme.readTemperature();
  m_humidity = bme.readHumidity();
  
  Serial.print("Temperatura (BMP): ");
  Serial.print(m_temperature);
  Serial.println("  °C");

  Serial.print("Pressao (BMP): ");
  Serial.print(m_pressure);
  Serial.println(" KPa");

  Serial.print("Altitude: ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");
}

void send_data(){
  String payload = 
    String("{") + 
    String("\"timestamp\":") + time(nullptr) + 
    String(",\"temperature\":") + m_temperature +
    String(",\"humidity\":") + m_humidity +
    String(",\"pressure\":") + m_pressure +
    String("}");
  publishTelemetry(payload);
  Serial.print("Data sent: ");
  Serial.println(payload);
  sprintf(msg_weather_data_to_MQTT, payload.c_str());
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(3000);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Initializing the BME280 sensor.
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println("WeMos Meteorology Station");
  Serial.println("");
  setupCloudIoT(); // Creates globals for MQTT
  
  read_sensors();
  send_data();
}

unsigned long lastMillis = 0;
void loop()
{
  mqttClient->loop();
  delay(10); // <- fixes some issues with WiFi stability

  if (!mqttClient->connected())
  {
    ESP.wdtDisable();
    connect();
    ESP.wdtEnable(0);
  }

  // TODO: Replace with your code here
  if (millis() - lastMillis > UPDATE_INTERVAL)
  {
    lastMillis = millis();
    read_sensors();
    send_data();
  }
  digitalWrite(LED_BUILTIN, HIGH);
}
