#pragma once

#include "AudioConfig.h"
#include "AudioTools/AudioOutput.h"
#include "AudioTools/AudioStreams.h"
#include "AudioTools/AudioTypes.h"
#include "AudioTools/AudioCopy.h"
#include "Stream.h"

namespace audio_tools {

/**
 * @brief Docoding of encoded audio into PCM data
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class AudioDecoder : public AudioWriter, public AudioBaseInfoSource {
public:
  AudioDecoder() = default;
  virtual ~AudioDecoder() = default;
  virtual AudioBaseInfo audioInfo() = 0;
  // for most decoder this is not needed
  virtual void setAudioInfo(AudioBaseInfo from) override {}
  virtual void setOutputStream(AudioStream &out_stream) {
    Print *p_print = &out_stream;
    setOutputStream(*p_print);
    setNotifyAudioChange(out_stream);
  }
  virtual void setOutputStream(AudioPrint &out_stream) {
    Print *p_print = &out_stream;
    setOutputStream(*p_print);
    setNotifyAudioChange(out_stream);
  }
  virtual void setOutputStream(Print &out_stream) = 0;
};

/**
 * @brief  Encoding of PCM data
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class AudioEncoder : public AudioWriter {
public:
  AudioEncoder() = default;
  virtual ~AudioEncoder() = default;
  virtual const char *mime() = 0;
};

/**
 * @brief Dummpy no implmentation Codec. This is used so that we can initialize
 * some pointers to decoders and encoders to make sure that they do not point to
 * null.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class CodecNOP : public AudioDecoder, public AudioEncoder {
public:
  static CodecNOP *instance() {
    static CodecNOP self;
    return &self;
  }

  virtual void begin() {}
  virtual void end() {}
  virtual void setOutputStream(Print &out_stream) {}
  virtual void setNotifyAudioChange(AudioBaseInfoDependent &bi) {}
  virtual void setAudioInfo(AudioBaseInfo info) {}

  virtual AudioBaseInfo audioInfo() {
    AudioBaseInfo info;
    return info;
  }
  virtual operator bool() { return false; }
  virtual int readStream(Stream &in) { return 0; };

  // just output silence
  virtual size_t write(const void *in_ptr, size_t in_size) {
    memset((void *)in_ptr, 0, in_size);
    return in_size;
  }

  virtual const char *mime() { return nullptr; }
};

/**
 * @brief A Streaming Decoder where we provide both the input and output
 * as streams.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class StreamingDecoder {
public:
  /// Starts the processing
  virtual void begin() = 0;

  /// Releases the reserved memory
  virtual void end() = 0;

  /// Defines the output Stream
  virtual void setOutputStream(Print &outStream) = 0;

  /// Register Output Stream to be notified about changes
  virtual void setNotifyAudioChange(AudioBaseInfoDependent &bi) = 0;

  /// Defines the output streams and register to be notified
  virtual void setOutputStream(AudioStream &out_stream) {
    Print *p_print = &out_stream;
    setOutputStream(*p_print);
    setNotifyAudioChange(out_stream);
  }

  /// Defines the output streams and register to be notified
  virtual void setOutputStream(AudioPrint &out_stream) {
    Print *p_print = &out_stream;
    setOutputStream(*p_print);
    setNotifyAudioChange(out_stream);
  }

  /// Defines the input data stream
  virtual void setInputStream(Stream &inStream) = 0;

  /// Provides the last available MP3FrameInfo
  virtual AudioBaseInfo audioInfo() = 0;

  /// checks if the class is active
  virtual operator bool() = 0;

  /// Process a single read operation - to be called in the loop
  virtual bool copy() = 0;

protected:
  virtual size_t readBytes(uint8_t *buffer, size_t len) = 0;
};

/**
 * @brief A more natural Stream class to process encoded data (aac, wav,
 * mp3...).
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class EncodedAudioStream : public AudioPrint {
public:
  /// Constructor for AudioStream with automatic notification of audio changes
  EncodedAudioStream(AudioStream *outputStream, AudioDecoder *decoder) {
    LOGD(LOG_METHOD);
    ptr_out = outputStream;
    decoder_ptr = decoder;
    decoder_ptr->setOutputStream(*outputStream);
    decoder_ptr->setNotifyAudioChange(*outputStream);
    writer_ptr = decoder_ptr;
    active = false;
  }

  /// Constructor for AudioPrint with automatic notification of audio changes
  EncodedAudioStream(AudioPrint *outputStream, AudioDecoder *decoder) {
    LOGD(LOG_METHOD);
    ptr_out = outputStream;
    decoder_ptr = decoder;
    decoder_ptr->setOutputStream(*outputStream);
    decoder_ptr->setNotifyAudioChange(*outputStream);
    writer_ptr = decoder_ptr;
    active = false;
  }


  /**
   * @brief Construct a new Encoded Stream object - used for decoding
   *
   * @param outputStream
   * @param decoder
   */
  EncodedAudioStream(Print &outputStream, AudioDecoder &decoder) {
    LOGD(LOG_METHOD);
    ptr_out = &outputStream;
    decoder_ptr = &decoder;
    decoder_ptr->setOutputStream(outputStream);
    writer_ptr = decoder_ptr;
    active = false;
  }

  /**
   * @brief Construct a new Encoded Audio Stream object - used for decoding
   *
   * @param outputStream
   * @param decoder
   */
  EncodedAudioStream(Print *outputStream, AudioDecoder *decoder) {
    LOGD(LOG_METHOD);
    ptr_out = outputStream;
    decoder_ptr = decoder;
    decoder_ptr->setOutputStream(*outputStream);
    writer_ptr = decoder_ptr;
    active = false;
  }

  /**
   * @brief Construct a new Encoded Audio Stream object - used for encoding
   *
   * @param outputStream
   * @param encoder
   */
  EncodedAudioStream(Print &outputStream, AudioEncoder &encoder) {
    LOGD(LOG_METHOD);
    ptr_out = &outputStream;
    encoder_ptr = &encoder;
    encoder_ptr->setOutputStream(outputStream);
    writer_ptr = encoder_ptr;
    active = false;
  }

  /**
   * @brief Construct a new Encoded Audio Stream object - used for encoding
   *
   * @param outputStream
   * @param encoder
   */
  EncodedAudioStream(Print *outputStream, AudioEncoder *encoder) {
    LOGD(LOG_METHOD);
    ptr_out = outputStream;
    encoder_ptr = encoder;
    encoder_ptr->setOutputStream(*outputStream);
    writer_ptr = encoder_ptr;
    active = false;
  }


  /**
   * @brief Construct a new Encoded Audio Stream object - the Output and
   * Encoder/Decoder needs to be defined with the begin method
   *
   */
  EncodedAudioStream() {
    LOGD(LOG_METHOD);
    active = false;
  }

  /**
   * @brief Destroy the Encoded Audio Stream object
   *
   */
  virtual ~EncodedAudioStream() {
    if (write_buffer != nullptr) {
      delete[] write_buffer;
    }
  }

  /// Define object which need to be notified if the basinfo is changing
  void setNotifyAudioChange(AudioBaseInfoDependent &bi) override {
    LOGI(LOG_METHOD);
    decoder_ptr->setNotifyAudioChange(bi);
  }

  AudioBaseInfo defaultConfig() {
    AudioBaseInfo cfg;
    cfg.channels = 2;
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    return cfg;
  }

  virtual void setAudioInfo(AudioBaseInfo info) {
    LOGD(LOG_METHOD);
    AudioPrint::setAudioInfo(info);
    decoder_ptr->setAudioInfo(info);
    encoder_ptr->setAudioInfo(info);
  }

  /// Starts the processing - sets the status to active
  void begin(Print *outputStream, AudioEncoder *encoder) {
    LOGD(LOG_METHOD);
    ptr_out = outputStream;
    encoder_ptr = encoder;
    encoder_ptr->setOutputStream(*outputStream);
    writer_ptr = encoder_ptr;
    begin();
  }

  /// Starts the processing - sets the status to active
  void begin(Print *outputStream, AudioDecoder *decoder) {
    LOGD(LOG_METHOD);
    ptr_out = outputStream;
    decoder_ptr = decoder;
    decoder_ptr->setOutputStream(*outputStream);
    writer_ptr = decoder_ptr;
    begin();
  }

  /// Starts the processing - sets the status to active
  void begin() {
    LOGD(LOG_METHOD);
    const CodecNOP *nop = CodecNOP::instance();
    if (decoder_ptr != nop || encoder_ptr != nop) {
      decoder_ptr->begin();
      encoder_ptr->begin();
      active = true;
    } else {
      LOGW("no decoder or encoder defined");
    }
  }

  /// Starts the processing - sets the status to active
  void begin(AudioBaseInfo info) {
    LOGD(LOG_METHOD);
    const CodecNOP *nop = CodecNOP::instance();
    if (decoder_ptr != nop || encoder_ptr != nop) {
      // some decoders need this - e.g. opus
      decoder_ptr->setAudioInfo(info);
      decoder_ptr->begin();
      encoder_ptr->setAudioInfo(info);
      encoder_ptr->begin();
      active = true;
    } else {
      LOGW("no decoder or encoder defined");
    }
  }
  /// Ends the processing
  void end() {
    LOGI(LOG_METHOD);
    decoder_ptr->end();
    encoder_ptr->end();
    active = false;
  }

  /// encode the data
  virtual size_t write(const uint8_t *data, size_t len) override {
    LOGD("%s: %zu", LOG_METHOD, len);
    if (len == 0) {
      LOGI("write: %d", 0);
      return 0;
    }

    if (writer_ptr == nullptr || data == nullptr) {
      LOGE("NPE");
      return 0;
    }

    size_t result = writer_ptr->write(data, len);
    return result;
  }

  int availableForWrite() override { return ptr_out->availableForWrite(); }

  /// Returns true if status is active and we still have data to be processed
  operator bool() { return active; }

  /// Provides the initialized decoder
  AudioDecoder &decoder() { return *decoder_ptr; }

  /// Provides the initialized encoder
  AudioEncoder &encoder() { return *encoder_ptr; }

protected:
  // ExternalBufferStream ext_buffer;
  AudioDecoder *decoder_ptr = CodecNOP::instance(); // decoder
  AudioEncoder *encoder_ptr = CodecNOP::instance(); // decoder
  AudioWriter *writer_ptr = nullptr;
  Print *ptr_out = nullptr;

  uint8_t *write_buffer = nullptr;
  int write_buffer_pos = 0;
  const int write_buffer_size = 256;
  bool active;
};

/**
 * @brief Provides the functionality to read decoded data from a encoded
 * data source. This class is less memory efficient then the EncodedAudioStream 
 * which writes decoded data to a final stream, but in some cases this is worth the price. If the buffer is too small you
 * can increase it by calling the resize() method.
 * 
 */
class DecoderStream : public AudioStreamX {
public:
  DecoderStream() = default;

    /// Assigns a potentially biffer buffer
  DecoderStream(BaseBuffer<uint8_t> *newBuffer){
    LOGI(LOG_METHOD);
    setBuffer(newBuffer);
  }

  /// Constructor for AudioStream with automatic notification of audio changes
  DecoderStream(Stream &inputStream, AudioDecoder &decoder) {
    LOGD(LOG_METHOD);
    ptr_in = &inputStream;
    decoder_ptr = &decoder;
    active = false;
  }

  DecoderStream(Stream *inputStream, AudioDecoder *decoder) {
    LOGD(LOG_METHOD);
    ptr_in = inputStream;
    decoder_ptr = decoder;
    active = false;
  }

  bool begin(Stream &inputStream, AudioDecoder &decoder) {
    LOGD(LOG_METHOD);
    ptr_in = &inputStream;
    decoder_ptr = &decoder;
    return begin();
  }

  bool begin(Stream *inputStream, AudioDecoder *decoder) {
    LOGD(LOG_METHOD);
    ptr_in = inputStream;
    decoder_ptr = decoder;
    return begin();
  }

  bool begin() {
    LOGD(LOG_METHOD);
    if (ptr_in==nullptr) return false;
    dec_stream.begin(&buffer_blocking, decoder_ptr);
    if (p_notify!=nullptr){
      dec_stream.setNotifyAudioChange(*p_notify);
    }
    copier.begin(dec_stream, *ptr_in);
    buffer.begin();
    // fill with initial data
    if (auto_load){
      refill();
    }
    active = true;
    return active ;
  }

  void end() {
    LOGD(LOG_METHOD);
    buffer.end();
    active = false;
  }

  size_t readBytes(uint8_t *data, size_t len){
    if (!active) return 0;
    size_t result = buffer.readBytes(data, len);
    if (auto_load && result==0){
      refill();
      result = buffer.readBytes(data, len);
    }

    if (result==0){
      LOGW("readBytes: %d", result);
    }

    return result;
  }

  int available() {
    size_t result = buffer.available();
    if (auto_load && result==0){
      refill();
      result = buffer.available();
    }
    return result;
  }

  void setNotifyAudioChange(AudioBaseInfoDependent &bi) override {
    p_notify = &bi;
    dec_stream.decoder().setNotifyAudioChange(bi);
  }

  AudioBaseInfo audioInfo() {
    return dec_stream.decoder().audioInfo();
  }

  operator bool() {
    return active && available()>0;
  }

  bool isEmpty() {
    return buffer.isEmpty();
  }

  /// Assigns a potentially biffer buffer
  void setBuffer(BaseBuffer<uint8_t> *newBuffer){
    LOGI(LOG_METHOD);
    buffer.setBuffer(newBuffer);
  }

  /// Refills buffer)
  bool copy() {
    LOGD(LOG_METHOD);
    bool data_available = false;
    if (buffer.availableForWrite()>0){
      copier.copy();
      int len = buffer.available();
      LOGD("buffer: %d", len);
      if (len>0){
        data_available = true;
      }
    } else {
      yield();
    }
    return data_available;
  }

  // automatically reloads the pcm buffer if it is empty
  void setAutoReload(bool flag){
    auto_load = flag;
  }

protected:
  // ExternalBufferStream ext_buffer;
  AudioDecoder *decoder_ptr = nullptr; // decoder
  Stream *ptr_in = nullptr;
  EncodedAudioStream dec_stream;
  BufferedStream buffer{512,20};
  BlockingStream buffer_blocking{buffer, 2000};
  StreamCopy copier;
  bool active;
  bool auto_load = true;
  AudioBaseInfoDependent *p_notify=nullptr;

  void refill() {
    while(buffer.available()==0 && buffer.availableForWrite()>0){
      copier.copy();
    }
  }


};

} // namespace audio_tools
