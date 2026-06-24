#ifndef EZ_IMAGE_HPP
#define EZ_IMAGE_HPP

#include <GLFW/glfw3.h> // Assumes GLFW/OpenGL context is available
#include <string>
#include <vector>
#include <iostream>

// --- Public Interface ---

struct EzImage {
    GLuint textureId = 0;
    int width = 0;
    int height = 0;
    
    // Transformations
    float rotation = 0.0f;       // In degrees
    bool flipX = false;
    bool flipY = false;
    
    // Color Adjustments
    float brightness = 1.0f;    // 0.0 to 2.0+ (1.0 is default)
    float saturation = 1.0f;    // 0.0 to 2.0+ (1.0 is default)
    float hueShift = 0.0f;      // -180.0 to 180.0 (0.0 is default)

    // Warping / UV control (Offsets for corners to warp the image)
    float warpTopLeft[2]     = {0.0f, 0.0f};
    float warpTopRight[2]    = {1.0f, 0.0f};
    float warpBottomLeft[2]  = {0.0f, 1.0f};
    float warpBottomRight[2] = {1.0f, 1.0f};
};

class EzImageManager {
public:
    EzImageManager();
    ~EzImageManager();

    // Add / Remove from the game engine
    EzImage* loadImage(const std::string& filepath);
    void removeImage(EzImage* image);
    void clearAll();

    // Render an image onto the screen using immediate mode (simple for 2D/GLFW setups)
    void drawImage(EzImage* image, float x, float y, float destWidth = 0.0f, float destHeight = 0.0f);

private:
    std::vector<EzImage*> managedImages;
};

#endif // EZ_IMAGE_HPP

// --- Implementation ---
#ifdef EZIMAGE_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

EzImageManager::EzImageManager() {}

EzImageManager::~EzImageManager() {
    clearAll();
}

EzImage* EzImageManager::loadImage(const std::string& filepath) {
    int width, height, channels;
    
    // Flip loaded texture on y-axis by default for OpenGL standard coordinate mapping
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 4); // Force RGBA
    
    if (!data) {
        std::cerr << "[EzImage] Failed to load image: " << filepath << "\n";
        return nullptr;
    }

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Setup basic texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    EzImage* img = new EzImage();
    img->textureId = textureId;
    img->width = width;
    img->height = height;

    managedImages.push_back(img);
    return img;
}

void EzImageManager::removeImage(EzImage* image) {
    if (!image) return;

    for (auto it = managedImages.begin(); it != managedImages.end(); ++it) {
        if (*it == image) {
            glDeleteTextures(1, &(image->textureId));
            delete image;
            managedImages.erase(it);
            break;
        }
    }
}

void EzImageManager::clearAll() {
    for (EzImage* img : managedImages) {
        glDeleteTextures(1, &(img->textureId));
        delete img;
    }
    managedImages.clear();
}

void EzImageManager::drawImage(EzImage* image, float x, float y, float destWidth, float destHeight) {
    if (!image || image->textureId == 0) return;

    // Default to native image dimensions if target dimensions aren't specified
    if (destWidth <= 0.0f)  destWidth = static_cast<float>(image->width);
    if (destHeight <= 0.0f) destHeight = static_cast<float>(image->height);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, image->textureId);

    // Dynamic Color adjustments (Using glColor4f environment properties for immediate pipeline)
    // Note: Advanced Hue shifting typically needs a fragment shader, but this applies basic color masking/brightness
    glColor4f(image->brightness, image->brightness, image->brightness, 1.0f);

    glPushMatrix();
    
    // Translation to position
    glTranslatef(x + destWidth / 2.0f, y + destHeight / 2.0f, 0.0f);
    
    // Handle Rotation
    glRotatef(image->rotation, 0.0f, 0.0f, 1.0f);
    
    // Handle Flipping
    glScalef(image->flipX ? -1.0f : 1.0f, image->flipY ? -1.0f : 1.0f, 1.0f);

    // Re-center target coordinates
    glTranslatef(-destWidth / 2.0f, -destHeight / 2.0f, 0.0f);

    // Draw the warped/transformed quad
    glBegin(GL_QUADS);
    
    // Top-Left
    glTexCoord2f(image->warpTopLeft[0], image->warpTopLeft[1]);
    glVertex2f(0.0f, 0.0f);

    // Top-Right
    glTexCoord2f(image->warpTopRight[0], image->warpTopRight[1]);
    glVertex2f(destWidth, 0.0f);

    // Bottom-Right
    glTexCoord2f(image->warpBottomRight[0], image->warpBottomRight[1]);
    glVertex2f(destWidth, destHeight);

    // Bottom-Left
    glTexCoord2f(image->warpBottomLeft[0], image->warpBottomLeft[1]);
    glVertex2f(0.0f, destHeight);
    
    glEnd();
    glPopMatrix();
    
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Reset color buffer state
}

#endif // EZIMAGE_IMPLEMENTATION
