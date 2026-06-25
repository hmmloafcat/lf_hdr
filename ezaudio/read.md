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

int main() {
    EzAudio audio;
    
    // Initialize OpenAL context
    if (!audio.init()) {
        return -1;
    }

    // Load sound
    Sound jumpSound = audio.loadSound("assets/sounds/jump.wav");
    
    if (jumpSound.buffer == 0) {
        std::cerr << "Failed to load. You prob forgot it\n";
        audio.shutdown();
        return -1;
    }

    // Play the sound
    audio.playSound(jumpSound);

    // Keep program alive long enough to hear the sound
    std::cout << "Press Enter to exit" << std::endl;
    std::cin.get();

    // Shutdown
    audio.shutdown();
    return 0;
}
```

## Credits

Check README.md for more info
