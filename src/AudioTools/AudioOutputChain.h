#pragma once
#include "AudioConfig.h"
#include "AudioTools/AudioPrint.h"
#include "AudioTools/AudioStreams.h"
#include "AudioTools/AudioStreamsConverter.h"
#include "AudioTools/AudioCopy.h"
#include "AudioBasic/Collections.h"

namespace audio_tools {

/**
 * @brief We try to avoid complex object initialization by just managing
 * a simple output processing chain
 */

class AudioOutputChain : public AudioPrint {
public:
    // Default constructor
    AudioOutputChain() = default;

    // Constructor with a source us a AudioCopy
    AudioOutputChain& from(AudioStream &input){
        p_input = &input;
        AudioPrint::setAudioInfo(input.audioInfo());
        return *this;
    }

    // Constructor with a source which saves us a AudioCopy
    AudioOutputChain& from(Stream &input, AudioBaseInfo cfg){
        p_input = &input;
        AudioPrint::setAudioInfo(cfg);
        return *this;
    }

    AudioOutputChain& convert(AudioStream &out){
        AudioStream *p_audio = &out;
        StreamAssignable *p_assign = (StreamAssignable *)p_audio;
        addList(new AudioOutputChainEntryStreamAssignable(p_audio, p_assign));
        return *this;
    }

    /// Simple AudioPrint will be added to multiout
    AudioOutputChain& output(AudioPrint &out){
        multi_out.add(out);
        addMultiOut();
        return *this;
    }

    /// Simple AudioStream will be added to multiout
    AudioOutputChain& output(AudioStream &out){
        multi_out.add(out);
        addMultiOut();
        return *this;
    }

    // add transforming stream
    AudioOutputChain&  reformat(AudioBaseInfo info){
        addList(new AudioOutputChainEntryFormatChange(info));
        return *this;
    }

    AudioOutputChain& convert(FormatConverterStream &fc){
        addList(new AudioOutputChainEntryFormatChange(fc.audioInfo()));
        return *this;
    }

    virtual size_t write(const uint8_t *buffer, size_t size) override {
        if(list.empty()) return 0;
        return first()->write(buffer,size);
    }

    /// starts the processing by linking 
    bool begin(){
        // setup chain
        for (int i=0;i<list.size()-1;i++){
            if (list[i]->stream!=nullptr){
                setAudioInfo(list[i+1]->begin(audioInfo(), list[i]->stream));
            } else {
                setAudioInfo(list[i+1]->begin(audioInfo(), list[i]->print));
            }
        }       
        setupCopy();
        return true;
    }

    /// Optional copy - when source has been defined
    size_t copy(){
        size_t result = 0;
        if (p_input!=nullptr){
            result = copier.copy();
        }
        return result;
    }

protected:

    struct AudioOutputChainEntry {
        AudioOutputChainEntry()=default;
        AudioOutputChainEntry(AudioStream *p_stream){
            stream=p_stream;
            out=p_stream;
        }
        AudioOutputChainEntry(AudioPrint *p_stream){
            print=p_stream;
            out = p_stream;
        }
        virtual AudioBaseInfo begin(AudioBaseInfo ai, Stream *p_stream){
            return begin(ai);
        }
        virtual AudioBaseInfo begin(AudioBaseInfo ai, Print *p_stream){
            return begin(ai);
        }
        AudioBaseInfo begin(AudioBaseInfo ai){
            if (stream!=nullptr){
                stream->setAudioInfo(ai);            
                stream->begin();
            }
            if(print!=nullptr){
                print->setAudioInfo(ai);            
            }
            return ai;
        }
        virtual size_t write(const uint8_t *buffer, size_t size)  {
            return out->write(buffer,size);
        }
        AudioStream *stream=nullptr;
        AudioPrint *print=nullptr;
        Print *out=nullptr;
    };

    struct AudioOutputChainEntryStreamAssignable : public AudioOutputChainEntry {
        AudioOutputChainEntryStreamAssignable(AudioStream *p_stream, StreamAssignable *p_assign) {
            stream=(AudioStream*)p_stream;
            out = (Print*) p_stream;
            assignable = p_assign;
        }
        AudioBaseInfo begin(AudioBaseInfo ai, Stream *p_stream) override {
            assignable->setStream(*p_stream);
            stream->setAudioInfo(ai);            
            stream->begin();
            return ai;
        }
        AudioBaseInfo begin(AudioBaseInfo ai, Print *p_stream) override {
            assignable->setStream(*p_stream);
            stream->setAudioInfo(ai);            
            stream->begin();
            return ai;
        }
      protected:
        AudioStream *stream=nullptr;
        StreamAssignable *assignable=nullptr;
    };

    struct AudioOutputChainEntryFormatChange : public AudioOutputChainEntry {
        AudioOutputChainEntryFormatChange(AudioBaseInfo info) : AudioOutputChainEntry(&fc_stream){
            new_info = info;
            out = &fc_stream;
            stream = &fc_stream;        }
        AudioBaseInfo begin(AudioBaseInfo ai, Stream *p_stream) override {
            completeNewInfo(ai);
            fc_stream.setStream(*p_stream);
            fc_stream.begin(ai, new_info);
            return new_info;
        }
        AudioBaseInfo begin(AudioBaseInfo ai, Print *p_stream) override {
            completeNewInfo(ai);
            fc_stream.setStream(*p_stream);
            fc_stream.begin(ai, new_info);
            return new_info;
        }

      protected:
        void completeNewInfo(AudioBaseInfo ai) {
            if (new_info.sample_rate==0) {
                new_info.sample_rate = ai.sample_rate;
            }
            if (new_info.channels==0) {
                new_info.channels = ai.channels;
            }
            if (new_info.bits_per_sample==0) {
                new_info.bits_per_sample = ai.bits_per_sample;
            }
        }
        AudioBaseInfo new_info;
        FormatConverterStream fc_stream;
    };

    Stream *p_input=nullptr;
    Vector<AudioOutputChainEntry*> list;
    StreamCopy copier;                         
    MultiOutput multi_out;
    AudioOutputChainEntry multi_out_entry{&multi_out};
    bool multi_added = false;

    AudioOutputChainEntry* first() {
        return *(list.begin());
    }

    AudioOutputChainEntry* last() {
        return list.back();
    }

    void addMultiOut(){
        if (!multi_added){
            multi_added = true;
            addList(&multi_out_entry);
        }
    }

    void addList(AudioOutputChainEntry* out){
        list.push_back(out);
    }

    void setupCopy(){
        if(p_input!=nullptr && list.size()==1){
            copier.begin(*first()->out, *p_input);
        }
    }
};

}