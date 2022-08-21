/**
 * @file streams-sd_mp3_alt-audiokit.ino
 * @author Phil Schatzmann
 * @brief decode MP3 file and output it on I2S
 * @version 0.1
 * @date 2022-8-21
 * 
 * @copyright Copyright (c) 2021 
 */

#include <SPI.h>
#include <SD.h>
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "AudioLibs/AudioKit.h"

AudioKitStream i2s; // final output of decoded stream
MP3DecoderHelix decoder;
DecoderStream in; // Decodded audio source
StreamCopy copier; 
File audioFile;

void setup(){
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);  

  // setup file
  SD.begin(PIN_AUDIO_KIT_SD_CARD_CS);
  audioFile = SD.open("/001.mp3");

  // setup i2s
  i2s.begin(i2s.defaultConfig(TX_MODE));

  // setup decoded input
  in.setNotifyAutoChange(i2s);
  in.begin(audioFile, decoder);

  // begin copy
  copier.begin(i2s, in);

}

void loop(){
  if (!copier.copy()) {
    stop();
  }
}
