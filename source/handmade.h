#pragma once
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define BUFFER_SECONDS 0.2f  // length of each buffer (100ms)
#define SOUND_FREQ 48000
#define SOUND_CHANNELS 2
#define MAX_KEYS 512
#define STICK_DEADZONE 0.10f

typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int16_t int16;
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
    uint32 channels;
    float32 *sound_buffer;
    size_t buffer_size;
} AudioSystem;

typedef struct {
    bool ended_down;          // // Is the button currently pressed?
    uint8 half_transition_count; // // Did the button change state this frame? 0 or 1
} ButtonState;

typedef struct{
    bool is_analog;

    //analog stick state last frame
    float32 start_x;
    float32 start_y;

    float32 max_x;
    float32 max_y;

    float32 min_x;
    float32 min_y;

    // analog stick state this frame
    float32 end_x;
    float32 end_y;

    union {
        struct {
            ButtonState move_up; // key w, dpad up
            ButtonState move_down;   // key s, dpad down
            ButtonState move_left;   // key a, dpad left
            ButtonState move_right;  // key d, dpad right
            ButtonState action_A;    // key q, a button
            ButtonState action_B;    // key e, b button
        };
        ButtonState keys[6]; // legacy array, union gives named buttons
    };
} GameInputState;

// platform independent functions
void GameUpdateAndRender(RenderBuffer *buffer, float t, AudioSystem *audio_system, SoundState *sound_state, bool soundBufferNeedsFilling,
                        GameInputState *input);

void UpdatePixels(RenderBuffer *buffer,float t);
void UpdateAudio(AudioSystem *audio_system, SoundState *sound_state);
void GenerateSineWave(AudioSystem *audio_system, SoundState *sound_state);
// void GenerateSquareWave(AudioSystem *audio_system, SoundState *sound_state);
static void UpdateGameInput(GameInputState *input);
