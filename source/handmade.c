#include "handmade.h"
#define PI 3.14159265358979323846

const char *button_names[6] = {
    "moveUp", "moveDown", "moveLeft", "moveRight", "actionA", "actionB"
};

void GameUpdateAndRender(GameMemory *game_memory, RenderBuffer *buffer,float t, AudioSystem *audio_system, SoundState *sound_state, bool soundBufferNeedsFilling,
                        GameInputState *input){
    GameState *game_state = (GameState *)game_memory->permanent_storage;
    if(!game_memory->is_inititialized){
        game_state->counter = 0;
        game_memory->is_inititialized = true;
    }
    UpdatePixels(buffer,t);
    UpdateGameInput(input);

    if(soundBufferNeedsFilling){
        UpdateAudio(audio_system, sound_state);
    }
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

void UpdateAudio(AudioSystem *audio_system, SoundState *sound_state) {
    GenerateSineWave(audio_system, sound_state);
    //GenerateSquareWave(audio_system, sound_state);
}

void GenerateSineWave(AudioSystem *audio_system, SoundState *sound_state){
    uint32 frames = audio_system->frames_per_buffer;
    float *samples = audio_system->sound_buffer;
    // Generate a simple sine wave
    for (uint32 i = 0; i < frames; ++i) {
        float sample = sinf(2.0f * (float)PI * sound_state->frequency * ((float)i / SOUND_FREQ) + sound_state->phase)
                        * sound_state->amplitude;
        for (uint32 c = 0; c < SOUND_CHANNELS; ++c){
            samples[i * SOUND_CHANNELS + c] = sample;
        }       
    }

    // Phase stores where the last sine wave ended.
    // Advance the phase for continuous playback so that the buffer begins where the last one ended
    // This ensures a smooth sine wave of sound
    sound_state->phase += 2.0f * (float)PI * sound_state->frequency * ((float)frames / SOUND_FREQ);
    if (sound_state->phase > 2.0f * (float)PI){
        sound_state->phase -= 2.0f * (float)PI;
    }
        
}
/*
    ---------- Button Logic ---------------

    1. Button is pressed this frame (was up, now down) -> ie: the button just went down.
        oldBState->ended_down = false
        isDown = true
        newBState->ended_down = true
        newBState->half_transition_count = 1 (because state changed)

    2. Button is released this frame (was down, now up) -> ie: the button just went up.
        oldBState->ended_down = true
        isDown = false
        newBState->ended_down = false
        newBState->half_transition_count = 1 (because state changed)

    3. Button is held down (was down, still down) -> ie: the button is being held.
        oldBState->ended_down = true
        isDown = true
        newBState->ended_down = true
        newBState->half_transition_count = 0 (no change)

    4. Button is not pressed (was up, still up) -> ie: the button is idle.
        oldBState->ended_down = false
        isDown = false
        newBState->ended_down = false
        newBState->half_transition_count = 0 (no change)

*/
static void UpdateGameInput(GameInputState *input){

    
    for (int i = 0; i < 6; ++i) {   // 6 buttons in your union array
        ButtonState *btn = &input->keys[i];

        if (btn->half_transition_count && btn->ended_down) {
            printf("%s pressed\n", button_names[i]);
        } else if (btn->half_transition_count && !btn->ended_down) {
            printf("%s released\n", button_names[i]);
        } else if (!btn->half_transition_count && btn->ended_down) {
            printf("%s held down\n", button_names[i]);
        }

        // Reset transition count after processing
        btn->half_transition_count = 0;
    }

    // there has been stick movement
    if(input->is_analog){
        printf("Analog end_x = %.3f   end_y = %.3f\n", input->end_x, input->end_y);
    }
}