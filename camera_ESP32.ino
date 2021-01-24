#include "esp_camera.h"
#include <WiFi.h>

// defining camera pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Wifi Credentials go here
char ssid[] = "";
char pass[] = "";

int status = WL_IDLE_STATUS;
IPAddress server(192,168,1,155);
int port = 25425;

WiFiClient client;
String mac;

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
    if (responseFront == "pi address:")
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
 * Prepares the ESP32 by connecting to wifi and initializing the camera
 */
void setup() 
{
  Serial.begin(115200); // enables Serial, this is only used for testing

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

  // initialize camera, config settings are copied from example documetation here: https://github.com/espressif/esp32-camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10; //lower is higher quality
  config.fb_count = 1;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.println("Camera initialization failed");
    while(1);
  }

  Serial.println("Setup complete");
  delay(100);
}

/*
 * Main loop, this runs continuously.
 * First it connects to the server.
 * Second it informs the server of this devices purpose so that the server knows it will be receiving data.
 * Third it captures an image.
 * Fourth it sends the image to the server.
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
  client.print("info camera " + mac);
  delay(1000);

  // capture an image
  Serial.println("Capturing image");
  camera_fb_t * frameBuffer = esp_camera_fb_get();
  if (!frameBuffer)
  {
    Serial.println("Failed to capture image");
  }
  else
  {
    // send the image to the server
    Serial.println("Sending image to server");
    client.write(frameBuffer->buf, frameBuffer->len);
    esp_camera_fb_return(frameBuffer);
  }

  delay(5000);
}
