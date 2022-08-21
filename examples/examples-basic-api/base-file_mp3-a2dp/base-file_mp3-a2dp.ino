/**
 * @file file_raw-serial.ino
 * @author Phil Schatzmann
 * @brief see https://github.com/pschatzmann/arduino-audio-tools/blob/main/examples/file_mp3-a2dp/README.md
 * Compile as Hughe App
 * @author Phil Schatzmann
 * @copyright GPLv3
 */

#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include <SPI.h>
#include <SD.h>

const char* file_name = "/test/002.mp3";
const int sd_ss_pin = 5;
File sound_file;
DecoderStream input{new NBuffer<uint8_t>(512,30)};  //mp3 needs quite a large buffer
VolumeStream volume(input); // volume control
BluetoothA2DPSource a2dp_source; // a2dp sender
MP3DecoderHelix helix; // mp3 decoder

// callback used by A2DP to provide the sound data
int32_t get_sound_data(Frame* data, int32_t fameCount) {
    const int frame_size = 4;
    // read decoded bytes
    return volume.readBytes((uint8_t*)data, fameCount*frame_size) / frame_size;   
}

// Arduino Setup
void setup(void) {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial,AudioLogger::Info);
  Serial.println("starting...");

  // Setup SD and open file
  SD.begin(sd_ss_pin);
  sound_file = SD.open(file_name, FILE_READ);
  if (!sound_file){
    Serial.println("file open error");
    return;
  }

  auto cfg = volume.defaultConfig();
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  volume.begin(cfg); 
  volume.setVolume(0.3);

  // open decoded stream
  input.begin(sound_file, helix);

  // start the bluetooth
  Serial.println("starting A2DP...");
  a2dp_source.set_auto_reconnect(false);
  a2dp_source.start("LEXON MINO L", get_sound_data);  
  Serial.println("started");
}

// Arduino loop - repeated processing 
void loop() {
  input.copy();
}