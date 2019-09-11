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
#include <DHT.h> // DHT11 humidity/temperature sensor
#include <Wire.h> // I2C communication
#include <Adafruit_BMP085.h> // BMP180 barometric pressure and temperature sensor

// Activate debug messages or not
#define DEBUG 1

// LED pin
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

// Defining DHT sensor model and pin
#define DHTTYPE DHT11   // DHT Shield uses DHT 11
#define DHTPIN D4       // DHT Shield uses pin D4

// Initialize DHT sensor
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

// Initialize BMP180 sensor class
Adafruit_BMP085 bmp;

float humidity, temperature, pressure;       // Raw float values from the sensor
char str_humidity[10], str_temperature[10];  // Rounded sensor values and as strings

// Generally, you should use "unsigned long" for variables that hold time
unsigned long last_millis = 0;      // When the sensors were last read
unsigned long dht_interval = 2000; // DHT sensors could take up to 2s to update
const unsigned long UPDATE_INTERVAL = 60 * 1000; // Time interval to send new data in milliseconds

// String to store the data packet
char msg_weather_data_to_MQTT[50];

void read_sensors() {
  // Wait at least 2 seconds seconds between measurements.
  // If the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor.
  // Works better than delay for things happening elsewhere also.
  unsigned long current_millis = millis();
  if (current_millis - last_millis >= dht_interval) {
    // Save the last time you read the sensor
    // last_millis = current_millis;
    readDHT();
  }
  readBmp();
}

void readDHT(){
  // Reading temperature and humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  humidity = dht.readHumidity();        // Read humidity as a percent
  temperature = dht.readTemperature();  // Read temperature as Celsius

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Convert the floats to strings and round to 2 decimal places
  dtostrf(humidity, 1, 2, str_humidity);
  dtostrf(temperature, 1, 2, str_temperature);

  Serial.print("Umidade (DHT): ");
  Serial.print(str_humidity);
  Serial.println();
  Serial.print("Temperatura (DHT): ");
  Serial.print(str_temperature);
  Serial.println(" °C");
}

void readBmp(){
  pressure = bmp.readPressure()/1000.0;
  Serial.print("Temperatura (BMP): ");
  Serial.print(bmp.readTemperature());
  Serial.println("  °C");
 
  Serial.print("Pressao (BMP): ");
  Serial.print(pressure);
  Serial.println(" KPa");

  Serial.print("Altitude: ");
  Serial.print(bmp.readAltitude());
  Serial.println(" m");
}

void send_data(){
  String payload = 
    String("{") + 
    String("\"timestamp\":") + time(nullptr) + 
    String(",\"temperature\":") + temperature +
    String(",\"humidity\":") + humidity +
    String(",\"pressure\":") + pressure +
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
  delay(100);
  Serial.println();
  dht.begin();
  // Initializing the BMP180 sensor.
  if (!bmp.begin()) 
  {
    Serial.println("Could not find BMP180 or BMP085 sensor at 0x77");
    while (1) {}
  }

  Serial.println("WeMos Meteorology Station");
  Serial.println("");
  setupCloudIoT(); // Creates globals for MQTT
  pinMode(LED_BUILTIN, OUTPUT);
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
}
