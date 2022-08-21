/**
 * @file streams-sd_mp3-audiokit.ino
 * @author Phil Schatzmann
 * @brief decode MP3 file and output it on I2S
 * @version 0.1
 * @date 2021-9-25
 * 
 * @copyright Copyright (c) 2021 
 */

#include <SPI.h>
#include <SD.h>
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "AudioLibs/AudioKit.h"

AudioKitStream i2s; // final output of decoded stream
EncodedAudioStream decoder(&i2s, new MP3DecoderHelix()); // Decoding stream
StreamCopy copier; 
File audioFile;

void setup(){
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Info);  

  // setup file
  SD.begin(PIN_AUDIO_KIT_SD_CARD_CS);
  audioFile = SD.open("/001.mp3");

  // setup i2s
  auto config = i2s.defaultConfig(TX_MODE);
  i2s.begin(config);

  // setup I2S based on sampling rate provided by decoder
  decoder.setNotifyAudioChange(i2s);
  decoder.begin();

  // begin copy
  copier.begin(decoder, audioFile);

}

void loop(){
  if (!copier.copy()) {
    stop();
  }
}
