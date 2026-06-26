#ifndef EZ_IMAGE_HPP
#define EZ_IMAGE_HPP

#include <GLFW/glfw3.h> // Assumes GLFW/OpenGL context is available
#include <string>
#include <vector>
#include <iostream>

// --- Public Interface ---

struct EzImage {
    GLuint textureId = 0;
    int nativeWidth = 0;
    int nativeHeight = 0;
    
    // Position and Size (Makes rendering much cleaner)
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;  // If left at 0, defaults to nativeWidth
    float height = 0.0f; // If left at 0, defaults to nativeHeight
    
    // Transformations
    float rotation = 0.0f;       // In degrees
    bool flipX = false;
    bool flipY = false;
    
    // Color Adjustments
    float brightness = 1.0f;    // 0.0 to 2.0+ (1.0 is default)
    float opacity = 1.0f;       // 0.0 (transparent) to 1.0 (opaque)

    // Warping / UV control (Offsets for corners to warp the image)
    float warpTopLeft[2]     = {0.0f, 0.0f};
    float warpTopRight[2]    = {1.0f, 0.0f};
    float warpBottomLeft[2]  = {0.0f, 1.0f};
    float warpBottomRight[2] = {1.0f, 1.0f};

    // Self-drawing shortcut
    void draw();
};

class EzImageManager {
public:
    EzImageManager();
    ~EzImageManager();

    // Add / Remove from the game engine
    EzImage* loadImage(const std::string& filepath);
    void removeImage(EzImage* image);
    void clearAll();

    // Internal rendering logic called by EzImage::draw()
    void renderImage(EzImage* image);

private:
    std::vector<EzImage*> managedImages;
};

// Global pointer wrapper to let the EzImage struct know how to draw itself easily
extern EzImageManager* g_EzImageManagerInstance;

#endif // EZ_IMAGE_HPP

// --- Implementation ---
#ifdef EZIMAGE_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Set the global instance tracker
EzImageManager* g_EzImageManagerInstance = nullptr;

void EzImage::draw() {
    if (g_EzImageManagerInstance) {
        g_EzImageManagerInstance->renderImage(this);
    }
}

EzImageManager::EzImageManager() {
    g_EzImageManagerInstance = this;
}

EzImageManager::~EzImageManager() {
    clearAll();
    if (g_EzImageManagerInstance == this) {
        g_EzImageManagerInstance = nullptr;
    }
}

EzImage* EzImageManager::loadImage(const std::string& filepath) {
    int width, height, channels;

    stbi_set_flip_vertically_on_load(false);
    
    // Force 4 channels (RGBA) to gracefully handle PNG alpha channels automatically
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 4); 
    
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

    // Upload texture data as RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    EzImage* img = new EzImage();
    img->textureId = textureId;
    img->nativeWidth = width;
    img->nativeHeight = height;
    img->width = static_cast<float>(width);
    img->height = static_cast<float>(height);

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

void EzImageManager::renderImage(EzImage* image) {
    if (!image || image->textureId == 0) return;

    // --- AUTOMATIC PNG TRANSPARENCY SETUP ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, image->textureId);

    // Apply brightness and opacity adjustments seamlessly
    glColor4f(image->brightness, image->brightness, image->brightness, image->opacity);

    glPushMatrix();
    
    // Translation to position using internal struct coordinates
    glTranslatef(image->x + image->width / 2.0f, image->y + image->height / 2.0f, 0.0f);
    
    // Handle Rotation
    glRotatef(image->rotation, 0.0f, 0.0f, 1.0f);
    
    // Handle Flipping
    glScalef(image->flipX ? -1.0f : 1.0f, image->flipY ? -1.0f : 1.0f, 1.0f);

    // Re-center target coordinates
    glTranslatef(-image->width / 2.0f, -image->height / 2.0f, 0.0f);

    // Draw the warped/transformed quad
    glBegin(GL_QUADS);
    
    // Top-Left
    glTexCoord2f(image->warpTopLeft[0], image->warpTopLeft[1]);
    glVertex2f(0.0f, 0.0f);

    // Top-Right
    glTexCoord2f(image->warpTopRight[0], image->warpTopRight[1]);
    glVertex2f(image->width, 0.0f);

    // Bottom-Right
    glTexCoord2f(image->warpBottomRight[0], image->warpBottomRight[1]);
    glVertex2f(image->width, image->height);

    // Bottom-Left
    glTexCoord2f(image->warpBottomLeft[0], image->warpBottomLeft[1]);
    glVertex2f(0.0f, image->height);
    
    glEnd();
    glPopMatrix();
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Reset color buffer state
}

#endif // EZIMAGE_IMPLEMENTATION
