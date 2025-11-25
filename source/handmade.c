#include "handmade.h"

bool GameUpdateAndRender(RenderBuffer *buffer,float t, AudioSystem *audioSystem, SoundState *soundState,
    uint32 queued_bytes, uint32 target_bytes, uint32 channels){
    UpdatePixels(buffer,t);
    return UpdateAudio(audioSystem, soundState, queued_bytes, target_bytes, channels);
}

void UpdatePixels(RenderBuffer *buffer, float t){
    // write directly to the buffers pixels

    uint32 height = buffer->height;
    uint32 width = buffer->width;
    uint32_t *pixels = (uint32_t *)buffer->pixels;

    for(uint32 y = 0; y < height; ++y){
        uint32_t *row = (uint32_t *)pixels + (y * width); // pointer to the start of the current row
        for(uint32 x = 0; x < width; ++x){
            uint8_t red = (uint8_t)((sin((x + t *100) * 0.01f) * 0.5f + 0.5f) *255);
            uint8_t blue = (uint8_t)((sin((x + y + t *100) * 0.01f) * 0.5f + 0.5f) *255);
            uint8_t green = (uint8_t)((sin((y + t *100) * 0.01f) * 0.5f + 0.5f) *255);;
            uint8_t alpha = 255;
            // dereference the pointer to set the pixel value
            *(row + x) = blue | (green << 8) | (red << 16) | (alpha << 24);
        }
    }
}

bool UpdateAudio(AudioSystem *audioSystem, SoundState *soundState, uint32 queued_bytes, uint32 target_bytes, uint32 channels) {
    if (queued_bytes < target_bytes) {
        uint32 frames = audioSystem->frames_per_buffer;
        float *samples = audioSystem->samples;

        GenerateSineWave(audioSystem, soundState);
        //GenerateSquareWave(audioSystem, soundState);
        return true;
    }
    return false;
}