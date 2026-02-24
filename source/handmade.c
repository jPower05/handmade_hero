
#include "handmade.h"
#define PI 3.14159265358979323846

internal_func void UpdatePixels(RenderBuffer *buffer, GameState *game_state);
internal_func void DrawCheckerboard(RenderBuffer *buffer, float32 x_offset, float32 y_offset);
internal_func void UpdateAudio(AudioSystem *audio_system, SoundState *sound_state);
internal_func void GenerateSineWave(AudioSystem *audio_system, SoundState *sound_state);
// void GenerateSquareWave(AudioSystem *audio_system, SoundState *sound_state);
internal_func void UpdateGameInput(GameInputState *input, GameState *game_state);

global_variable char *button_names[6] = {
    "moveUp", "moveDown", "moveLeft", "moveRight", "actionA", "actionB"
};


void GameUpdateAndRender(GameMemory *game_memory, RenderBuffer *buffer, float t, 
    AudioSystem *audio_system, SoundState *sound_state, bool soundBufferNeedsFilling,
        GameInputState *input){
    // cast the permanent storage to a game state struct so we can use it
    GameState *game_state = (GameState *)game_memory->permanent_storage;
    
    if(!game_memory->is_inititialized){
        // file loading (note '/' at start is important for absolute path)
        char filename[128];
        realpath("source/test.txt", filename);
        printf("Trying to open file: %s\n", filename);

        void *bitmap_memory = PlatformReadEntireFile(filename);
        if(bitmap_memory){
            printf("success");
            PlatformFreeFileMemory(bitmap_memory);
            bitmap_memory = NULL;
        }

        game_state->counter = 0;
        game_memory->is_inititialized = true;
    }
    UpdatePixels(buffer, game_state);
    UpdateGameInput(input, game_state);

    if(soundBufferNeedsFilling){
        UpdateAudio(audio_system, sound_state);
    }
}

internal_func void UpdatePixels(RenderBuffer *buffer, GameState *game_state){
    // write directly to the buffers pixels
    DrawCheckerboard(buffer, game_state->x_offset, game_state->y_offset);

    // uint32 height = buffer->height;
    // uint32 width = buffer->width;
    // uint32 *pixels = (uint32 *)buffer->pixels;

    // for(uint32 y = 0; y < height; ++y){
    //     uint32 *row = (uint32 *)pixels + (y * width); // pointer to the start of the current row
    //     for(uint32 x = 0; x < width; ++x){
    //         uint8 red = (uint8)((sin((x + t *100) * 0.01f) * 0.5f + 0.5f) *255);
    //         uint8 blue = (uint8)((sin((x + y + t *100) * 0.01f) * 0.5f + 0.5f) *255);
    //         uint8 green = (uint8)((sin((y + t *100) * 0.01f) * 0.5f + 0.5f) *255);;
    //         uint8 alpha = 255;
    //         // dereference the pointer to set the pixel value
    //         *(row + x) = blue | (green << 8) | (red << 16) | (alpha << 24);
    //     }
    // }
}

internal_func void DrawCheckerboard(RenderBuffer *buffer, float32 x_offset, float32 y_offset){
    uint32 height = buffer->height;
    uint32 width = buffer->width;
    uint32 *pixels = (uint32 *)buffer->pixels;

    const int tileSize = 60;

    for (uint32 y = 0; y < height; ++y) {
        uint32 *row = (uint32 *)pixels + (y * width); // pointer to the start of the current row
        float fy = (float)y + y_offset;
        uint32 tileY = (uint32)floorf(fy / tileSize);; // convert y coordinate to tile index
        for (uint32 x = 0; x < width; ++x) {
            float fx = (float)x + x_offset;
            uint32 tileX = (uint32)floorf(fx / tileSize);   // convert x coordinate to tile index
            bool evenTile = (tileX + tileY) % 2 == 0;
            // Determine which color to use based on the tile position
            uint8 red, green, blue;
            uint8 alpha = 255;

            if (evenTile){
                red   = 30;
                green = 30;
                blue  = 30;
            }
            else{
                red   = 200;
                green = 200;
                blue  = 200;
            }
            *(row + x) = blue | (green << 8) | (red << 16) | (alpha << 24); 
        }
    }
}

internal_func void UpdateAudio(AudioSystem *audio_system, SoundState *sound_state) {
    GenerateSineWave(audio_system, sound_state);
    //GenerateSquareWave(audio_system, sound_state);
}

internal_func void GenerateSineWave(AudioSystem *audio_system, SoundState *sound_state){
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
internal_func void UpdateGameInput(GameInputState *input, GameState *game_state){

    // movement speed in pixels per frame
    const float32 speed = 5.0f;

    // check directional buttons
    if(input->move_up.ended_down){
        game_state->y_offset -= speed;
    }
    if(input->move_down.ended_down){
        game_state->y_offset += speed;
    }
    if(input->move_left.ended_down){
        game_state->x_offset -= speed;
    }
    if(input->move_right.ended_down){
        game_state->x_offset += speed;
    }

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