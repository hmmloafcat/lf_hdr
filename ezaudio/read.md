# EzAudio

A header for audio playing

EzAudio is a small library to play audio easily with almost no hassle

Supports:
OGG

MP3

FLAC

WAV

Example:

```cpp
#define EZAUDIO_IMPLEMENTATION
#include "lfhdr_ezaudio.hpp"
#include <thread>
#include <chrono>

int main() {
    EzAudio audio;

    if (!audio.init()) {
        return -1;
    }

    // Load audio files into OpenAL buffers
    ALuint backgroundMusic = audio.loadSound("assets/music.ogg");

    // Play background music on a loop
    audio.playSound(backgroundMusic, true);

    // Keep the program running long enough to hear the audio
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop all playing audio before exiting
    audio.stopAll();
    audio.shutdown();

    return 0;
}
```

## Credits

Check README.md for more info
