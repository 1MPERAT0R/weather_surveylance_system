# weather_surveylance_system
A system that collects basic weather information and captures pictures then displays the data on a webpage.

The server program is made to be run on a raspberry pi but with some minor alterations to the handoutAddress() function it should run on windows computers too.

The ESP32 programs require the SSID and password of the same WiFi network that the server is connected to. The server's IP address can also be given but is optional, the ESP32s
will make a UDP broadcast asking for the server's IP address if they need too.

Hardware used:
Server - Raspberry Pi 4, other models should work.
Weather ESP32 - NodeMCU ESP32, any ESP32 board should work but may require different pins.
Camera ESP32 - AI-Thinker ESP32-CAM

As many ESP32s can be connected to the server as desired. I've tested with one Weather and two Camera ESP32s but more shouldn't cause any problems.
