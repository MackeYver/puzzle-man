#include "XAUDIO2REDIST.H"

//
// Structs
//

#define kAudio_Total_Voice_Count 3

struct Audio_Source {
    XAUDIO2_BUFFER buffer;
    WAVEFORMATEX format;
    IXAudio2SourceVoice *voice = nullptr;
    //b32 is_playing = false;
};

/*
typedef struct tWAVEFORMATEX {
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
  WORD  wBitsPerSample;
  WORD  cbSize;
}
*/


// class Voice_Callback : public IXAudio2VoiceCallback {
// public:
//     Voice_Callback()  {}
//     ~Voice_Callback() {}

//     // Called when the voice has just finished playing a contiguous audio stream.
//     void OnStreamEnd() {}

//     // Unused methods are stubs
//     void OnVoiceProcessingPassEnd()                        {}
//     void OnVoiceProcessingPassStart(u32 samples_required)  {}    
//     void OnLoopEnd(void *buffer_context)                   {}
//     void OnVoiceError(void *buffer_context, HRESULT error) {}

//     void OnBufferEnd(void *buffer_context) {
//         Audio_Source *source = static_cast<Audio_Source *>(buffer_context);
//         source->is_playing = false;
//     }
    
//     void OnBufferStart(void *buffer_context) {
//         Audio_Source *source = static_cast<Audio_Source *>(buffer_context);
//         source->is_playing = true;
//     }
// };


struct Audio {
    IXAudio2 *xaudio = nullptr;
    IXAudio2MasteringVoice *mastering_voice = nullptr;
    IXAudio2SubmixVoice *submix_voice = nullptr;
    Audio_Source sources[kAudio_Total_Voice_Count];
    Log *log = nullptr;
    //Voice_Callback callback;
};




//
// Init and fini
//
void static fini_source(Audio_Source *source) {
    if (source) {
        source->voice->DestroyVoice();
        source->voice = nullptr;
    }
}


void static fini_audio(Audio *audio) {
    if (audio) {
        for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
            if (audio->sources[index].voice) {
                fini_source(&audio->sources[index]);
            }
        }
        
        if (audio->mastering_voice) {
            audio->mastering_voice->DestroyVoice();
            audio->mastering_voice = nullptr;
        }

        if (audio->xaudio) {
            audio->xaudio->Release();
            audio->xaudio = nullptr;
        }
    }
}


b32 static critical_error(Audio *audio, HRESULT hresult, char const *error_message) {
    b32 result = false;
    
    if (FAILED(hresult)) {
        LOG_ERROR(audio->log, error_message, hresult);        
        result = false;
        fini_audio(audio);
    }

    return result;
}


b32 static init_audio(Audio *audio, Log *log) {
    audio->log = log;
    HRESULT hresult = XAudio2Create(&audio->xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (critical_error(audio, hresult, "failed to create the xaudio object"))  return false;

    hresult = audio->xaudio->CreateMasteringVoice(&audio->mastering_voice);
    if (critical_error(audio, hresult, "failed to create the mastering voice"))  return false;

    audio->submix_voice = nullptr;

    for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
        audio->sources[index].voice = nullptr;
        //audio->sources[index].is_playing = false;
    }
        
    return true;
}




//
// Voices
//

void static set_format_from_wav(Audio_Source *source, Wav *wav) {
    if (source && wav) {
        source->format.wFormatTag = WAVE_FORMAT_PCM;
        source->format.nChannels = wav->format.number_of_channels;
        source->format.nSamplesPerSec = wav->format.sample_rate;
        source->format.nAvgBytesPerSec = wav->format.average_bytes_per_second;
        source->format.nBlockAlign = wav->format.block_align;
        source->format.wBitsPerSample = wav->format.bits_per_sample;
        source->format.cbSize = wav->format.extra_format_size;        
    }
}


b32 static init_voice(Audio *audio, Audio_Source *source, Wav *wav) {
    set_format_from_wav(source, wav);
    XAUDIO2_SEND_DESCRIPTOR send_desc;
    send_desc.Flags = 0;
    send_desc.pOutputVoice = audio->submix_voice;
    XAUDIO2_VOICE_SENDS voice_sends;
    voice_sends.SendCount = 1;
    voice_sends.pSends = &send_desc;

    HRESULT hresult = audio->xaudio->CreateSourceVoice(&source->voice, &source->format, 0, XAUDIO2_DEFAULT_FREQ_RATIO,
                                                       0, &voice_sends, 0);//&audio->callback, &voice_sends, 0);
    if (critical_error(audio, hresult, "failed to create voice"))  return false;

    return true;
}


b32 static init_voices(Audio *audio, Wav *wav) {
    b32 result = false;

    if (audio && wav) {
        //
        // Create submix voice
        XAUDIO2_SEND_DESCRIPTOR send_desc;
        send_desc.Flags = 0;
        send_desc.pOutputVoice = audio->mastering_voice;
        XAUDIO2_VOICE_SENDS voice_sends;
        voice_sends.SendCount = 1;
        voice_sends.pSends = &send_desc;
        
        HRESULT hresult = audio->xaudio->CreateSubmixVoice(&audio->submix_voice, wav->format.number_of_channels, wav->format.sample_rate,
                                                           0, 0, &voice_sends, 0);
        if (critical_error(audio, hresult, "failed to create the submix voice"))  return false;

        //
        // Create source voices
        for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
            result = init_voice(audio, &audio->sources[index], wav);
            if (!result)  break;
        }
    }

    return result;
}


b32 static play_wav(Audio *audio, Wav *wav) {
    b32 result = false;
    
    if (audio && wav) {
        for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
            Audio_Source *source = &audio->sources[index];
            if (!source)             continue;

            XAUDIO2_VOICE_STATE voice_audio;
            source->voice->GetState(&voice_audio);
            if (voice_audio.pCurrentBufferContext || voice_audio.BuffersQueued > 0)  continue;

            
            //
            // Check formats
            if ((source->format.nChannels != wav->format.number_of_channels) ||
                (source->format.nSamplesPerSec != wav->format.sample_rate) ||
                (source->format.nAvgBytesPerSec!= wav->format.average_bytes_per_second) ||
                (source->format.nBlockAlign != wav->format.block_align) ||
                (source->format.wBitsPerSample != wav->format.bits_per_sample))
            {
                fini_source(source);
                result = init_voice(audio, source, wav);
                assert(result);
            }
            

            // setup buffer
            source->buffer.Flags = XAUDIO2_END_OF_STREAM;
            source->buffer.AudioBytes = static_cast<u32>(wav->data_size);
            source->buffer.pAudioData = wav->data;
            source->buffer.PlayBegin = 0;
            source->buffer.PlayLength = 0;
            source->buffer.LoopBegin = 0;
            source->buffer.LoopLength = 0;
            source->buffer.LoopCount = 0;
            source->buffer.pContext = nullptr;

            HRESULT hresult = source->voice->SubmitSourceBuffer(&source->buffer);
            if (FAILED(hresult)) {                
                char buffer[512];
                _snprintf_s(buffer, 512, _TRUNCATE, "failed to submit source buffer, error: %d", hresult);
                critical_error(audio, hresult, buffer);
            }

            source->voice->Start();
            break;
        }
    }

    return result;
}
