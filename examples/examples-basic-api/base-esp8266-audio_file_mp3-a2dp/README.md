# Stream mp3 SD File to A2DP Bluetooth

We are reading a decoded SD mp3 file with the help of the ESP8266-Audio Library

The SD module is connected with the help of the SPI bus

### Pins:

We connect the SD to the ESP32:

| SD      | ESP32
|---------|---------------
| VCC     | 5V
| GND     | GND
| CS      | CS GP5
| SCK     | SCK GP18
| MOSI    | MOSI GP23
| MISO    | MISO GP19


### Dependencies

- https://github.com/pschatzmann/arduino-audio-tools
- https://github.com/pschatzmann/ESP32-A2DP
- https://github.com/earlephilhower/ESP8266Audio

### Compile Options

 - Partition Scheme: Huge App
