
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_stdinc.h"
#define SDL_MAIN_USE_CALLBACKS 1
#include "handmade.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <unistd.h>
#include <stdio.h>
#include <limits.h>


// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
global_variable GameMemory game_memory = {0};

global_variable SDL_Window *window = NULL;
global_variable SDL_Renderer *renderer = NULL;
global_variable RenderBuffer render_buffer = {0};
global_variable AudioSystem audio_system = {0};
global_variable SoundState sound_state = {0};
global_variable SDL_Gamepad *controller = NULL;
global_variable SDL_Texture *texture = NULL;
global_variable SDL_AudioSpec audio_spec = {0};
global_variable SDL_AudioStream *audio_stream = NULL;

global_variable bool soundBufferNeedsFilling = true;

// Timing globals
global_variable uint64 perf_start = 0;
global_variable uint64 last_counter = 0;
global_variable double64 dt = 0;
global_variable double64 t_total = 0;
global_variable uint64 perf_freq = 0;

// input
global_variable GameInputState input = {0};
global_variable GameInputState input_prev = {0};

// ------------------------------------------------------------
// Function Declarations
// ------------------------------------------------------------
internal_func bool InitGameMemory();

// game controller input
internal_func void UpdateButton(ButtonState *oldBState, ButtonState *newBState, bool isDown);
internal_func void ProcessKeyboardEvent(SDL_KeyboardEvent *e);
internal_func void ProcessControllerButton(SDL_GamepadButtonEvent *e);
internal_func void ProcessControllerAxis(SDL_GamepadAxisEvent *e);
internal_func float NormalizeStickValue(int16 val);

// audio and rendering

internal_func void ResizeRenderBuffer(RenderBuffer *buffer, uint32 Width, uint32 Height);
internal_func bool InitAudio(AudioSystem *audio_system, SoundState *sound_state);
internal_func void DestroyAudio(AudioSystem *audio_system);


void *PlatformReadEntireFile(char *filename){
    void *file_buffer = NULL;
    SDL_IOStream *file_handle = NULL;
    // get file stream/handle
    file_handle = SDL_IOFromFile(filename, "r");
    if (!file_handle) {
        SDL_Log("error getting file handle for '%s'", filename);
        return NULL;
    }

    // Get the file size
    Sint64 size = SDL_GetIOSize(file_handle);
    if (size <= 0) {
        SDL_Log("file size invalid for '%s'", filename);
        SDL_CloseIO(file_handle);
        return NULL;
    }

    // allocate memory for the file
    file_buffer = SDL_malloc((size_t)size);
    if (!file_buffer) {
        SDL_Log("error allocating memory for the file_buffer");
        SDL_CloseIO(file_handle);
        return NULL;
    }

    // read the file
    size_t bytesRead = SDL_ReadIO(file_handle, file_buffer, (size_t)size);
    if (bytesRead != (size_t)size) {
        SDL_Log("Failed to read entire file '%s'", filename);
        SDL_free(file_buffer);
        file_buffer = NULL;
    } else {
        SDL_Log("Read entire file '%s' successfully", filename);
    }

    SDL_CloseIO(file_handle);
    return file_buffer;
}

void PlatformFreeFileMemory(void *memory){
    SDL_free(memory);
    SDL_Log("File memory freed");
}
bool32 PlatformWriteEntireFile(char *filename, uint32 memory_size, void *memory){
    return true;
}

bool InitGameMemory(){
    game_memory.permanent_storage_size = Megabytes(64);
    game_memory.transient_storage_size = Gigabytes(2);

    size_t total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;

    void *block = SDL_calloc(1, total_size);
    if(!block){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                     "Failed to allocate %zu bytes for game memory",
                     total_size);
        return false;
    }

    // split the block
    game_memory.permanent_storage = block; // start of the game_memory_block
    game_memory.transient_storage = (uint8 *)block + game_memory.permanent_storage_size;

    return true;
}


// --- Called once at startup ---
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){

    char cwd[1000];
    getcwd(cwd, sizeof(cwd));
    printf("Current working directory: %s\n", cwd);

    uint32 init_height = 480;
    uint32 init_width = 640;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        SDL_Log("Failed to init SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // allocate game memory
    if(!InitGameMemory()){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate game memory");
        return SDL_APP_FAILURE; 
    }
    
    window = SDL_CreateWindow("Handmade Hero", init_width, init_height, SDL_WINDOW_RESIZABLE);

    // Initialize audio
    sound_state.frequency = 256.0f;  // A4 note
    sound_state.amplitude = 0.10f;
    sound_state.phase = 0.0f;

    if (!InitAudio(&audio_system, &sound_state)) {
        SDL_Log("Audio failed to init");
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    ResizeRenderBuffer(&render_buffer, init_width, init_height);

    // Timing setup
    perf_freq = SDL_GetPerformanceFrequency();
    perf_start = SDL_GetPerformanceCounter();
    last_counter = perf_start;

    SDL_Log("High precision timer freq = %llu Hz", (uint64)perf_freq);

    // --- Gamepad initialization ---
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    SDL_Log("Found %d gamepads", count);

    controller = NULL;
    for (int i = 0; i < count; ++i) {
        controller = SDL_OpenGamepad(ids[i]);
        if (controller) {
            SDL_Log("Gamepad connected: %s", SDL_GetGamepadName(controller));
            break;
        }
    }

    SDL_free(ids);

    if (!controller) {
        SDL_Log("No gamepad connected.");
    }

    // stick defaults
    input.start_x = 0.0f;
    input.start_y = 0.0f;
    input.end_x = 0.0f;
    input.end_y = 0.0f;

    input.min_x = input.min_y = 9999.0f;
    input.max_x = input.max_y = -9999.0f;
    
    SDL_Log("SDL initialized and window created");
    return SDL_APP_CONTINUE;
}

// --- Called every time there’s an event (key press, mouse, etc.) ---
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch(event->type){
        case SDL_EVENT_MOUSE_MOTION:{

        }
        break;
        case SDL_EVENT_QUIT:{
            return SDL_APP_SUCCESS;
        }
        break;
        case SDL_EVENT_WINDOW_RESIZED:{
            uint32 width = event->window.data1;
            uint32 height = event->window.data2;
            ResizeRenderBuffer(&render_buffer, width, height);
        }
        break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            ProcessKeyboardEvent(&event->key);
        break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            ProcessControllerButton(&event->gbutton);
        break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            ProcessControllerAxis(&event->gaxis);
        break;
        default:{
            break;
        }
    }
    return SDL_APP_CONTINUE;
}

// --- Called once per frame (your main loop logic goes here) ---
SDL_AppResult SDL_AppIterate(void *appstate){

    // timing
    uint64 now = SDL_GetPerformanceCounter();

    dt = (double64)(now - last_counter) / (double64)perf_freq;
    last_counter = now;

    t_total = (double64)(now - perf_start) / (double64)perf_freq;

    // Debug output
    // static uint32 frame_count = 0;
    // if (++frame_count % 60 == 0) {
    //     SDL_Log("FPS: %.1f   dt: %.6f   t_total: %.2f",
    //         1.0 / dt, dt, t_total);
    // }

    // Exit automatically after 5 seconds
    if (t_total > 100) {
        SDL_Log("Timeout reached, exiting...");
        return SDL_APP_SUCCESS;
    }

    input.is_analog = false;

    // copy analog values for next frame
    input.start_x = input.end_x;
    input.start_y = input.end_y;

    // determine if stick is currently moved outside deadzone
    input.is_analog = (fabsf(input.end_x) >= STICK_DEADZONE || fabsf(input.end_y) >= STICK_DEADZONE);

    // Determine how much audio is currently queued in the stream
    uint32 queued_bytes = SDL_GetAudioStreamAvailable(audio_stream);
    uint32 target_bytes = (uint32)(audio_system.buffer_size * 3);  // keep ~0.6s buffered

    soundBufferNeedsFilling = (queued_bytes < target_bytes);

    GameUpdateAndRender(&game_memory, &render_buffer, (float32) t_total, &audio_system, &sound_state, soundBufferNeedsFilling, &input);

    if(soundBufferNeedsFilling){
        SDL_PutAudioStreamData(audio_stream, audio_system.sound_buffer, audio_system.buffer_size);
    }

    SDL_UpdateTexture(texture, NULL, render_buffer.pixels, render_buffer.pitch);
    // (buffer.width * 4 ) = how far to move in memory from one row of pixels to the next.
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // Copy current input to previous at the start of the frame
    input_prev = input;
    
    return SDL_APP_CONTINUE;
}

// --- Called once on shutdown ---
void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_Log("Cleaning up...");

    DestroyAudio(&audio_system);
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (render_buffer.pixels) {
        SDL_free(render_buffer.pixels);
        render_buffer.pixels = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    if (controller){
        SDL_CloseGamepad(controller);
        controller = NULL;
    }

    if (game_memory.permanent_storage) {
        SDL_free(game_memory.permanent_storage);    // start of memory block
        game_memory.permanent_storage = NULL;
        game_memory.transient_storage = NULL; 
    }

    SDL_Quit();
}

internal_func void ResizeRenderBuffer(RenderBuffer *render_buffer, uint32 width, uint32 height){
    
    // Free old buffer if it exists
    if (render_buffer->pixels) {
        free(render_buffer->pixels);
        render_buffer->pixels = NULL;
    }
    // clear old texture
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    // update the buffer
    render_buffer->height = height;
    render_buffer->width = width;
    render_buffer->bytesPerPixel = 4; // ARGB8888  
    render_buffer->pitch = render_buffer->width * render_buffer->bytesPerPixel;

    // allocate space for pixels
    size_t total_bytes = (size_t)width * (size_t)height * render_buffer->bytesPerPixel;
    render_buffer->pixels = SDL_malloc(total_bytes);

    if (!render_buffer->pixels) {
        SDL_Log("Failed to allocate memory for render buffer pixels");
        return;
    }

    // zero the memory
    SDL_memset(render_buffer->pixels, 0, total_bytes);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         width, height);

    if (!texture) {
        SDL_Log("Failed to recreate texture: %s", SDL_GetError());
    } else {
        SDL_Log("Texture recreated: %dx%d", width, height);
    }
}

internal_func bool InitAudio(AudioSystem *audio_system, SoundState *sound_state){

    audio_spec.format = SDL_AUDIO_F32;   // 32-bit float audio
    audio_spec.channels = SOUND_CHANNELS;
    audio_spec.freq = SOUND_FREQ;

    audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, 
        &audio_spec, NULL, NULL);
    if (!audio_stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    //UpdateAudio(audio_system, sound_state);

    /* SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start! */
    SDL_ResumeAudioStreamDevice(audio_stream);


    // Allocate buffer for our generated samples
    audio_system->frames_per_buffer = (uint32)(audio_spec.freq * BUFFER_SECONDS);
    audio_system->samples_per_buffer = audio_system->frames_per_buffer * audio_spec.channels;
    audio_system->buffer_size = audio_system->samples_per_buffer * sizeof(float);
    audio_system->sound_buffer = (float *)SDL_malloc(audio_system->buffer_size);

    if (!audio_system->sound_buffer) {
        SDL_Log("Failed to allocate audio buffer");
        return false;
    }

    // zero the memory
    SDL_memset(audio_system->sound_buffer, 0, audio_system->buffer_size);


    SDL_Log("Audio initialized: %.1f sec buffer, %d Hz, %d channels",
        BUFFER_SECONDS, audio_spec.freq, audio_spec.channels);

    return true;
}

internal_func void DestroyAudio(AudioSystem *audio_system) {
    if (audio_stream) {
        // Pause audio playback before destroying the stream
        SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(audio_stream));
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
    }
    if (audio_system->sound_buffer) {
        SDL_free(audio_system->sound_buffer);
        audio_system->sound_buffer = NULL;
    }
    SDL_Log("Audio system cleaned up");
}


internal_func void UpdateButton(ButtonState *oldBState, ButtonState *newBState, bool isDown){
    newBState->ended_down = isDown;
    newBState->half_transition_count = (oldBState->ended_down != isDown) ? 1 : 0;
}

internal_func void ProcessKeyboardEvent(SDL_KeyboardEvent *e) {
    SDL_Scancode sc = e->scancode;
    bool isDown = (e->type == SDL_EVENT_KEY_DOWN);
    switch (sc) {
    case SDL_SCANCODE_W:
        UpdateButton(&input_prev.move_up, &input.move_up, isDown);
        break;
    case SDL_SCANCODE_S:
        UpdateButton(&input_prev.move_down, &input.move_down, isDown);
        break;
    case SDL_SCANCODE_A:
        UpdateButton(&input_prev.move_left, &input.move_left, isDown);
        break;
    case SDL_SCANCODE_D:
        UpdateButton(&input_prev.move_right, &input.move_right, isDown);
        break;
    case SDL_SCANCODE_Q:
        UpdateButton(&input_prev.action_A, &input.action_A, isDown);
        break;
    case SDL_SCANCODE_E:
        UpdateButton(&input_prev.action_B, &input.action_B, isDown);
        break;
    default:
        break;
    }
}

internal_func void ProcessControllerButton(SDL_GamepadButtonEvent *e) {
    if (!controller) return;
    bool isDown = (e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
    switch (e->button) {
        case SDL_GAMEPAD_BUTTON_LABEL_A:{
            UpdateButton(&input_prev.action_A, &input.action_A, isDown); 
        } 
        break;
        case SDL_GAMEPAD_BUTTON_LABEL_B:{
            UpdateButton(&input_prev.action_B, &input.action_B, isDown);
        } 
        break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:{
            UpdateButton(&input_prev.move_up, &input.move_up, isDown);
        }
        break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:{
            UpdateButton(&input_prev.move_down, &input.move_down, isDown);
        }
        break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:{
            UpdateButton(&input_prev.move_left, &input.move_left, isDown);
        }  
        break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:{
            UpdateButton(&input_prev.move_right, &input.move_right, isDown);
        } 
        break;
    }
}

internal_func void ProcessControllerAxis(SDL_GamepadAxisEvent *e) {
    if (!controller) return;
    
    float32 val = NormalizeStickValue(e->value);

    if (fabsf(val) < STICK_DEADZONE){
        val = 0.0f;
    } else {
        //SDL_Log("Gamepad axis %d raw=%d norm=%.4f", e->axis, e->value, val);
    }

    

    switch(e->axis){
        case SDL_GAMEPAD_AXIS_LEFTX:
            input.end_x = val;

            if (val < input.min_x) input.min_x = val;
            if (val > input.max_x) input.max_x = val;
            break;

        case SDL_GAMEPAD_AXIS_LEFTY:
            input.end_y = val;

            if (val < input.min_y) input.min_y = val;
            if (val > input.max_y) input.max_y = val;
            break;
    }


    // using sticks like dpad
    bool up    = (input.end_y < -0.5f);
    bool down  = (input.end_y >  0.5f);
    bool left  = (input.end_x < -0.5f);
    bool right = (input.end_x >  0.5f);

    UpdateButton(&input_prev.move_up,    &input.move_up,    up);
    UpdateButton(&input_prev.move_down,  &input.move_down,  down);
    UpdateButton(&input_prev.move_left,  &input.move_left,  left);
    UpdateButton(&input_prev.move_right, &input.move_right, right);

}

internal_func float NormalizeStickValue(int16 raw){
    // normalize the value
    if (raw < 0) {
        return (float)raw / 32768.0f;   // maps -32768 -> -1.0
    } else {
        return (float)raw / 32767.0f;   // maps  32767 ->  1.0
    }
}
