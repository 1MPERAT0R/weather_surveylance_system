#include <WiFi.h>
#include <Wire.h>
#include "SparkFunBME280.h"
#include <WiFiUdp.h>

char ssid[] = "";
char pass[] = "";

int status = WL_IDLE_STATUS;
IPAddress server(192,168,1,155); // Needs to be adjusted frequently
int port = 25425;

WiFiClient client;
String mac;

BME280 airSensor;

WiFiUDP udp;
IPAddress broadcastIP(192,168,1,255);

/*
 * If this device is unable to connect to the server, it will do a udp broadcast asking for the servers IP.
 * If the server is available it will respond with the correct IP address.
 */
void findServer()
{
  String message = "pi address?";
  
  udp.beginPacket(broadcastIP, 25426);
  for (int x = 0; x < message.length(); x++)
  {
    udp.write(message[x]);
  }
  udp.endPacket();

  delay(1000);

  String response;
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    while (udp.available())
      response += (char)udp.read();

    String responseFront = response.substring(0, 11);
    Serial.println(responseFront);
    if (responseFront == "pi address:") // response message will start with @ to ensure it is for me
    {
      response = response.substring(11);
      Serial.println(response);
      uint8_t zero = (uint8_t)response.substring(0,3).toInt();
      uint8_t one = (uint8_t)response.substring(4,7).toInt();
      uint8_t two = (uint8_t)response.substring(8,9).toInt();
      uint8_t three = (uint8_t)response.substring(10).toInt();
      server[0] = zero;
      server[1] = one;
      server[2] = two;
      server[3] = three;
    }
  }
  delay(2000);
}

/*
 * Prepares the ESP32 by making sure the BME280 is connected and then connects to wifi
 */
void setup() 
{
  Serial.begin(115200); // enables Serial, this is only used for testing
  Wire.begin(22, 21); // SDA, SCL   Defines the pins used for I2c connection to BME280 sensor

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

  udp.begin(25426);
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
    Serial.print("Failed to connect to server at ");
    Serial.print(server);
    Serial.println(", asking for server IP, will retry in 3 seconds");
    findServer();
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
