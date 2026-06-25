#ifndef EZ_AUDIO_HPP
#define EZ_AUDIO_HPP

#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cctype>

// --- Public Interface ---

class Sound {
public:
    ALuint buffer = 0;
    bool loop = false;
    bool paused = false;
    float volume = 1.0f;
    float pan = 0.0f;
    float speed = 1.0f;
    float start = 0.0f;

    Sound() = default;
    Sound(ALuint buf) : buffer(buf) {}
};

class EzAudio {
public:
    EzAudio();
    ~EzAudio();

    // System management
    bool init();
    void shutdown();

    // Audio control
    Sound loadSound(const std::string& filepath);
    ALuint playSound(const Sound& sound);
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

// stb_vorbis displays linker errors
// So, the implementation will be removed
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

Sound EzAudio::loadSound(const std::string& filepath) {
    ALenum format;
    ALsizei sampleRate;
    std::vector<int16_t> pcmData;

    // Determine file extension
    std::string ext = "";
    size_t dotIdx = filepath.find_last_of(".");
    if (dotIdx != std::string::npos) {
        ext = filepath.substr(dotIdx + 1);
        // Standardize extension strings to lowercase (Fixes issues like .WAV)
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    // Decode WAV
    if (ext == "wav") {
        unsigned int channels;
        unsigned int sampleRateRaw;
        drwav_uint64 totalPCMFrameCount;
        int16_t* pSampleData = drwav_open_file_and_read_pcm_frames_s16(filepath.c_str(), &channels, &sampleRateRaw, &totalPCMFrameCount, nullptr);
        
        if (!pSampleData) return Sound(0);
        
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
        
        if (!pSampleData) return Sound(0);
        
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
        
        if (!pSampleData) return Sound(0);
        
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
        
        if (samples <= 0) return Sound(0);
        
        pcmData.assign(pSampleData, pSampleData + (samples * channels));
        format = (channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        sampleRate = sampleRateRaw;
        free(pSampleData);
    }
    else {
        std::cerr << "[EzAudio] Unsupported format: " << ext << "\n";
        return Sound(0);
    }

    // Upload PCM data to OpenAL buffer
    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, pcmData.data(), pcmData.size() * sizeof(int16_t), sampleRate);
    
    buffers.push_back(buffer);
    return Sound(buffer);
}

ALuint EzAudio::playSound(const Sound& sound) {
    if (sound.buffer == 0) return 0;

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

    // Bind buffer to source and process loop setting
    alSourcei(source, AL_BUFFER, sound.buffer);
    alSourcei(source, AL_LOOPING, sound.loop ? AL_TRUE : AL_FALSE);
    
    // Apply requested volume and speed (pitch) parameters
    alSourcef(source, AL_GAIN, sound.volume);
    alSourcef(source, AL_PITCH, sound.speed);
    
    // Process panning. Setting source strictly relative to listener limits variables 
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(source, AL_POSITION, sound.pan, 0.0f, 0.0f);

    // Apply specific start offset
    if (sound.start > 0.0f) {
        alSourcef(source, AL_SEC_OFFSET, sound.start);
    }

    // Engage playback immediately unless starting paused
    if (!sound.paused) {
        alSourcePlay(source);
    }

    // Return the source so the user can issue an alSourcePlay(source) later if they used the paused property
    return source;
}

void EzAudio::stopAll() {
    for (ALuint source : sources) {
        alSourceStop(source);
    }
}

#endif  // EZ_AUDIO_IMPLEMENTATION
