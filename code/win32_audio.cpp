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


struct Audio_State {
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


void static fini_audio(Audio_State *state) {
    if (state) {
        for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
            if (state->sources[index].voice) {
                fini_source(&state->sources[index]);
            }
        }
        
        if (state->mastering_voice) {
            state->mastering_voice->DestroyVoice();
            state->mastering_voice = nullptr;
        }

        if (state->xaudio) {
            state->xaudio->Release();
            state->xaudio = nullptr;
        }
    }
}


b32 static critical_error(Audio_State *state, HRESULT hresult, char const *error_message) {
    b32 result = false;
    
    if (FAILED(hresult)) {
        LOG_ERROR(state->log, error_message, hresult);        
        result = false;
        fini_audio(state);
    }

    return result;
}


b32 static init_audio(Audio_State *state, Log *log) {
    state->log = log;
    HRESULT hresult = XAudio2Create(&state->xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (critical_error(state, hresult, "failed to create the xaudio object"))  return false;

    hresult = state->xaudio->CreateMasteringVoice(&state->mastering_voice);
    if (critical_error(state, hresult, "failed to create the mastering voice"))  return false;

    state->submix_voice = nullptr;

    for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
        state->sources[index].voice = nullptr;
        //state->sources[index].is_playing = false;
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


b32 static init_voice(Audio_State *state, Audio_Source *source, Wav *wav) {
    set_format_from_wav(source, wav);
    XAUDIO2_SEND_DESCRIPTOR send_desc;
    send_desc.Flags = 0;
    send_desc.pOutputVoice = state->submix_voice;
    XAUDIO2_VOICE_SENDS voice_sends;
    voice_sends.SendCount = 1;
    voice_sends.pSends = &send_desc;

    HRESULT hresult = state->xaudio->CreateSourceVoice(&source->voice, &source->format, 0, XAUDIO2_DEFAULT_FREQ_RATIO,
                                                       0, &voice_sends, 0);//&state->callback, &voice_sends, 0);
    if (critical_error(state, hresult, "failed to create voice"))  return false;

    return true;
}


b32 static init_voices(Audio_State *state, Wav *wav) {
    b32 result = false;

    if (state && wav) {
        //
        // Create submix voice
        XAUDIO2_SEND_DESCRIPTOR send_desc;
        send_desc.Flags = 0;
        send_desc.pOutputVoice = state->mastering_voice;
        XAUDIO2_VOICE_SENDS voice_sends;
        voice_sends.SendCount = 1;
        voice_sends.pSends = &send_desc;
        
        HRESULT hresult = state->xaudio->CreateSubmixVoice(&state->submix_voice, wav->format.number_of_channels, wav->format.sample_rate,
                                                           0, 0, &voice_sends, 0);
        if (critical_error(state, hresult, "failed to create the submix voice"))  return false;

        //
        // Create source voices
        for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
            result = init_voice(state, &state->sources[index], wav);
            if (!result)  break;
        }
    }

    return result;
}


b32 static play_wav(Audio_State *state, Wav *wav) {
    b32 result = false;
    
    if (state && wav) {
        for (u32 index = 0; index < kAudio_Total_Voice_Count; ++index) {
            Audio_Source *source = &state->sources[index];
            if (!source)             continue;

            XAUDIO2_VOICE_STATE voice_state;
            source->voice->GetState(&voice_state);
            if (voice_state.pCurrentBufferContext || voice_state.BuffersQueued > 0)  continue;

            
            //
            // Check formats
            #ifdef DEBUG
            if ((source->format.nChannels != wav->format.number_of_channels) ||
                (source->format.nSamplesPerSec != wav->format.sample_rate) ||
                (source->format.nAvgBytesPerSec!= wav->format.average_bytes_per_second) ||
                (source->format.nBlockAlign != wav->format.block_align) ||
                (source->format.wBitsPerSample != wav->format.bits_per_sample))
            {
                fini_source(source);
                result = init_voice(state, source, wav);
                assert(result);
            }
            #endif
            

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
                printf("%s() failed to submit source buffer, error = 0x%x\n", __FUNCTION__, hresult);
            }

            source->voice->Start();
            break;
        }
    }

    return result;
}
