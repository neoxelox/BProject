# BProject
Server &amp; Frontend to manage your blinds with an ESP32

**NOTE!** I'm refactoring all of that and more to a new project [Shutters](https://github.com/Neoxelox/Shutters) in Golang!

BProject is a simple web server and a mini frontend for ESP32 to manage your blinds via radiofrecuency.
The server is built in C++ with libraries from Arduino. The frontend is built in vanilla HTML/CSS/JS with Websockets but also uses a bit of http methods to have a mini login system.
In order to use this code you have to decode your blinds' controllers and serialize the pulse code with this *schema*:
**1**: Short high pulse
**0**: Short low pulse
**H**: Long low pulse (Header)
**N**: Long wait for new command

To build the webpage on the ESP32 you have to use a tool to insert data into the ESP32 memory which you can find here https://github.com/me-no-dev/arduino-esp32fs-plugin .

The project also have RTC & Temperature/Humidity sensors. This code is not necessary to control the blinds but it's useful in the dashboard. All the pinouts for the sensors and actuators and defined in the headers of the source code.

BTW: Notice that the code is commented in spanish as I don't have time to translate it, but it shouldn't be difficult to understand it :).
