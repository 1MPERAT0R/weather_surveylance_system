#include <WiFi.h>
#include <Wire.h>
#include "SparkFunBME280.h"

char ssid[] = "";
char pass[] = "";

int status = WL_IDLE_STATUS;
IPAddress server(192,168,1,187); // Needs to be adjusted frequently
int port = 25425;

WiFiClient client;
String mac;

BME280 airSensor;

/*
 * Prepares the ESP32 by making sure the BME280 is connected and then connects to wifi
 */
void setup() 
{
  Serial.begin(115200); // enables Serial, this is only used for testing
  Wire.begin(4, 15); // SCL, SDA   Defines the pins used for I2c connection to BME280 sensor

  Serial.println("Connecting to BME280");
  if (!airSensor.beginI2C())
  {
    Serial.println("Failed to connect to BME280, check wiring");
    while(true);
  }
  Serial.println("BME 280 Connected");


  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.println("WiFi Connected");

  mac = String(WiFi.macAddress());


  // tell the second core to blink an LED so that it is easy to tell the ESP32 is on.
  pinMode(LED_BUILTIN, OUTPUT);
  xTaskCreatePinnedToCore(TaskBlink, "TaskBlink", 1024, NULL, 2, NULL, 0);
}

/*
 * Main loop, this runs continuously.
 * First it connects to the server.
 * Second it informs the server of this devices purpose so that the server knows it will be receiving data.
 * Third it captures and sends the data to the server.
 * Repeat forever.
 */
void loop() 
{
  Serial.println("Connecting to Server");
  while (!client.connect(server, port))
  {
    Serial.println("Failed to connect to server, retrying in 3 seconds");
    delay(3000);
  }
  Serial.println("Server Connected");

  Serial.println("Informing server of this devices purpose");
  client.print("info weather " + mac);
  delay(1000);

  Serial.println("Sending data");
  client.print(String(airSensor.readFloatHumidity()) + " " + String(airSensor.readFloatPressure()) + " " + String(airSensor.readTempC()));
  client.stop();
  Serial.println("Disconnected from Server");

  delay(5000);
}

/*
 * Blinks the builtin LED indefinitely
 */
void TaskBlink(void *pvParameters)
{
  while(true)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(700);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(700);
  }
}
