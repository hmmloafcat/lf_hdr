#ifndef EZ_AUDIO_HPP
#define EZ_AUDIO_HPP

#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <vector>
#include <iostream>

// --- Public Interface ---

class EzAudio {
public:
    EzAudio();
    ~EzAudio();

    // System management
    bool init();
    void shutdown();

    // Audio control
    ALuint loadSound(const std::string& filepath);
    void playSound(ALuint buffer, bool loop = false);
    void stopAll();

private:
    ALCdevice* device = nullptr;
    ALCcontext* context = nullptr;
    std::vector<ALuint> buffers;
    std::vector<ALuint> sources;
};

#endif // EZ_AUDIO_HPP

// --- Implementation ---
#ifdef EZAUDIO_IMPLEMENTATION

// Third-party audio decoding libraries
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

EzAudio::EzAudio() {}

EzAudio::~EzAudio() {
    shutdown();
}

bool EzAudio::init() {
    // Open default audio device
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "[EzAudio] Failed to open OpenAL device.\n";
        return false;
    }

    // Create and activate context
    context = alcCreateContext(device, nullptr);
    if (!context) {
        alcCloseDevice(device);
        std::cerr << "[EzAudio] Failed to create OpenAL context.\n";
        return false;
    }
    alcMakeContextCurrent(context);
    return true;
}

void EzAudio::shutdown() {
    // Clean up active sources
    for (ALuint source : sources) {
        alSourceStop(source);
        alDeleteSources(1, &source);
    }
    sources.clear();

    // Clean up loaded buffers
    for (ALuint buffer : buffers) {
        alDeleteBuffers(1, &buffer);
    }
    buffers.clear();

    // Reset OpenAL context and device
    alcMakeContextCurrent(nullptr);
    if (context) alcDestroyContext(context);
    if (device) alcCloseDevice(device);
}

ALuint EzAudio::loadSound(const std::string& filepath) {
    ALenum format;
    ALsizei sampleRate;
    std::vector<int16_t> pcmData;

    // Determine file extension
    std::string ext = "";
    size_t dotIdx = filepath.find_last_of(".");
    if (dotIdx != std::string::npos) {
        ext = filepath.substr(dotIdx + 1);
    }

    // Decode WAV
    if (ext == "wav") {
        unsigned int channels;
        unsigned int sampleRateRaw;
        drwav_uint64 totalPCMFrameCount;
        int16_t* pSampleData = drwav_open_file_and_read_pcm_frames_s16(filepath.c_str(), &channels, &sampleRateRaw, &totalPCMFrameCount, nullptr);
        
        if (!pSampleData) return 0;
        
        pcmData.assign(pSampleData, pSampleData + (totalPCMFrameCount * channels));
        format = (channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        sampleRate = sampleRateRaw;
        drwav_free(pSampleData, nullptr);
    }
    // Decode MP3
    else if (ext == "mp3") {
        drmp3_config config;
        drmp3_uint64 totalPCMFrameCount;
        drmp3_int16* pSampleData = drmp3_open_file_and_read_pcm_frames_s16(filepath.c_str(), &config, &totalPCMFrameCount, nullptr);
        
        if (!pSampleData) return 0;
        
        pcmData.assign(pSampleData, pSampleData + (totalPCMFrameCount * config.channels));
        format = (config.channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        sampleRate = config.sampleRate;
        drmp3_free(pSampleData, nullptr);
    }
    // Decode FLAC
    else if (ext == "flac") {
        unsigned int channels;
        unsigned int sampleRateRaw;
        drflac_uint64 totalPCMFrameCount;
        int16_t* pSampleData = drflac_open_file_and_read_pcm_frames_s16(filepath.c_str(), &channels, &sampleRateRaw, &totalPCMFrameCount, nullptr);
        
        if (!pSampleData) return 0;
        
        pcmData.assign(pSampleData, pSampleData + (totalPCMFrameCount * channels));
        format = (channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        sampleRate = sampleRateRaw;
        drflac_free(pSampleData, nullptr);
    }
    // Decode OGG (VORBIS)
    else if (ext == "ogg") {
        int channels;
        int sampleRateRaw;
        short* pSampleData;
        int samples = stb_vorbis_decode_filename(filepath.c_str(), &channels, &sampleRateRaw, &pSampleData);
        
        if (samples <= 0) return 0;
        
        pcmData.assign(pSampleData, pSampleData + (samples * channels));
        format = (channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        sampleRate = sampleRateRaw;
        free(pSampleData);
    }
    else {
        std::cerr << "[EzAudio] Unsupported format: " << ext << "\n";
        return 0;
    }

    // Upload PCM data to OpenAL buffer
    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, pcmData.data(), pcmData.size() * sizeof(int16_t), sampleRate);
    
    buffers.push_back(buffer);
    return buffer;
}

void EzAudio::playSound(ALuint buffer, bool loop) {
    if (buffer == 0) return;

    // Find an idle source or generate a new one
    ALuint source = 0;
    for (ALuint s : sources) {
        ALint state;
        alGetSourcei(s, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING && state != AL_PAUSED) {
            source = s;
            break;
        }
    }

    if (source == 0) {
        alGenSources(1, &source);
        sources.push_back(source);
    }

    // Bind buffer to source and play
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcePlay(source);
}

void EzAudio::stopAll() {
    for (ALuint source : sources) {
        alSourceStop(source);
    }
}

#endif  // EZ_AUDIO_IMPLEMENTATION
