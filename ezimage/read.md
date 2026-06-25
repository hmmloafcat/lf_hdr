# EzImage

A library for decoding and loading images to your game

Supports:

- JPG/JPEG

- PNG

- TGA

etc.

## Usage

Read the wiki for more info.

```cpp
#include <GLFW/glfw3.h>
#include <iostream>

// Define the implementation macro in exactly one source file before including the header
#define EZIMAGE_IMPLEMENTATION
#include "ez_image.hpp"

// Window size constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main() {
    // Initialize GLFW and create a window
    if (!glfwInit()) {
        std::cerr << "Failed to load GLFW :(\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "EzImage Example", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to load the window :(\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-sync

    // Set up basic 2D orthographic projection
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0); // 0,0 top-left, matching screen coords
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Initialize the Image Manager and load an image
    EzImageManager imageManager;
    
    // Loading a sample player asset
    EzImage* playerSprite = imageManager.loadImage("assets/player.png");
    
    if (!playerSprite) {
        std::cerr << "The sprite is not there bud\n";
        glfwTerminate();
        return -1;
    }

    // Customize the sprite properties using the clean struct API
    playerSprite->x = 200.0f;          // Position x on screen
    playerSprite->y = 150.0f;          // Position y on screen
    playerSprite->width = 128.0f;       // Override the width (scales automatically)
    playerSprite->height = 128.0f;     // Override height
    playerSprite->opacity = 0.85f;      // Slight transparency effect
    playerSprite->brightness = 1.2f;   // Make it slightly brighter

    // Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        // Clear screen with a nice dark gray background
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Dynamically update properties over time (like spinning the character)
        playerSprite->rotation += 1.0f;
        if (playerSprite->rotation >= 360.0f) {
            playerSprite->rotation = 0.0f;
        }

        // Render our sprite using its internal drawing shortcut
        playerSprite->draw();

        // Swap buffers and poll window events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup (The manager destructor clears loaded textures automatically, 
    // but you can also do it explicitly)
    imageManager.clearAll();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```
