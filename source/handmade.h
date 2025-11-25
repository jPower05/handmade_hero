#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <math.h>

typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float float32;
typedef double double64;

typedef struct{
    uint32 width;
    uint32 height;
    uint32 bytesPerPixel;
    uint32 pitch; // how many bytes a pointer needs to move to get to the next row
    void *pixels;
} RenderBuffer;

typedef struct {
    float32 phase;
    float32 frequency;
    float32 amplitude;
} SoundState;

typedef struct {
    uint32 frames_per_buffer;
    uint32 samples_per_buffer;
    float32 *samples;
    size_t buffer_size;
} AudioSystem;

// platform independent functions
bool GameUpdateAndRender(RenderBuffer *buffer,float t, AudioSystem *audioSystem, SoundState *soundState,
    uint32 queued_bytes, uint32 target_bytes, uint32 channels);

void UpdatePixels(RenderBuffer *buffer,float t);
bool UpdateAudio(AudioSystem *audioSystem, SoundState *soundState, uint32 queued_bytes, uint32 target_bytes, uint32 channels);

// platform sdl dependent functions

void ResizeRenderBuffer(RenderBuffer *buffer, uint32 Width, uint32 Height);
bool InitAudio(AudioSystem *audioSystem, SoundState *soundState);
void DestroyAudio(AudioSystem *audioSystem);
void GenerateSineWave(AudioSystem *audioSystem, SoundState *soundState);
void GenerateSquareWave(AudioSystem *audioSystem, SoundState *soundState);