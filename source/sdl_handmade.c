#define SDL_MAIN_USE_CALLBACKS 1

#include "handmade.h"
#define PI 3.14159265358979323846

#define BUFFER_SECONDS 0.2f  // length of each buffer (100ms)
#define SOUND_FREQ 48000


// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static RenderBuffer buffer = {0};
static AudioSystem audioSystem = {0};
static SoundState soundState = {0};
static SDL_Gamepad *controller = NULL;
static SDL_Texture *texture = NULL;
static SDL_AudioSpec audioSpec = {0};
static SDL_AudioStream *audioStream = NULL;

// Timing globals
static uint64 perf_start = 0;
static uint64 last_counter = 0;
static double64 dt = 0;
static double64 t_total = 0;
static uint64 perf_freq = 0;


// --- Called once at startup ---
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){
    uint32 init_height = 480;
    uint32 init_width = 640;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        SDL_Log("Failed to init SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    window = SDL_CreateWindow("Handmade Hero", init_width, init_height, SDL_WINDOW_RESIZABLE);

    // Initialize audio
    soundState.frequency = 256.0f;  // A4 note
    soundState.amplitude = 0.10f;
    soundState.phase = 0.0f;

    if (!InitAudio(&audioSystem, &soundState)) {
        SDL_Log("⚠️ Audio failed to init");
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("❌ Failed to create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    ResizeRenderBuffer(&buffer, init_width, init_height);

    // Timing setup
    perf_freq = SDL_GetPerformanceFrequency();
    perf_start = SDL_GetPerformanceCounter();
    last_counter = perf_start;

    SDL_Log("High precision timer freq = %llu Hz", (uint64)perf_freq);

    // --- Gamepad initialization ---
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    SDL_Log("🎮 Found %d gamepads", count);

    controller = NULL;
    for (int i = 0; i < count; ++i) {
        controller = SDL_OpenGamepad(ids[i]);
        if (controller) {
            SDL_Log("✅ Gamepad connected: %s", SDL_GetGamepadName(controller));
            break;
        }
    }

    SDL_free(ids);

    if (!controller) {
        SDL_Log("⚠️ No gamepad connected.");
    }
    
    SDL_Log("✅ SDL initialized and window created");
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
            ResizeRenderBuffer(&buffer, width, height);
        }
        break;

        case SDL_EVENT_KEY_DOWN: {
            SDL_Scancode sc = event->key.scancode;

            switch (sc) {
                case SDL_SCANCODE_ESCAPE:
                    SDL_Log("👋 Escape pressed — exiting");
                    return SDL_APP_SUCCESS;

                case SDL_SCANCODE_W:{
                    SDL_Log("⌨️ Key pressed: %s", SDL_GetScancodeName(sc));
                    break;
                }
                case SDL_SCANCODE_S:{
                    SDL_Log("⌨️ Key pressed: %s", SDL_GetScancodeName(sc));
                    break;
                }
                case SDL_SCANCODE_A:{
                    SDL_Log("⌨️ Key pressed: %s", SDL_GetScancodeName(sc));
                    break;
                }
                case SDL_SCANCODE_D:{
                    SDL_Log("⌨️ Key pressed: %s", SDL_GetScancodeName(sc));
                    break;
                }
                
                default:
                    SDL_Log("⌨️ Key pressed: %s", SDL_GetScancodeName(sc));
                    break;
            }
        } 
        break;

        case SDL_EVENT_KEY_UP: {
            SDL_Scancode sc = event->key.scancode;
            SDL_Log("🪶 Key released: %s", SDL_GetScancodeName(sc));
        } 
        break;


        // 🎮 Controller removed
        case SDL_EVENT_GAMEPAD_REMOVED:{
            SDL_Log("🎮 Controller removed (instance id: %d)", event->gdevice.which);
            if (controller){
                SDL_CloseGamepad(controller);
                controller = NULL;
                SDL_Log("🛑 Controller closed");
            }
        }
        break;

        // 🔘 Button down
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:{
            SDL_Log("🔘 Button down: %s", SDL_GetGamepadStringForButton(event->gbutton.button));
        }
        break;

        // 🔘 Button up
        case SDL_EVENT_GAMEPAD_BUTTON_UP:{
            SDL_Log("🔘 Button up: %s", SDL_GetGamepadStringForButton(event->gbutton.button));
        }
        break;

        // // 🎚️ Analog stick or trigger moved
        // case SDL_EVENT_GAMEPAD_AXIS_MOTION:{
        //     float norm = event->gaxis.value / 32767.0f;
        //     SDL_Log("🎚️ Axis motion: %s = %.3f",
        //         SDL_GetGamepadStringForAxis(event->gaxis.axis),
        //             norm);
        // }
        // break;

        default:{
            return SDL_APP_CONTINUE;
        }
    }
    return SDL_APP_CONTINUE;
}

// --- Called once per frame (your main loop logic goes here) ---
SDL_AppResult SDL_AppIterate(void *appstate)
{
    // timing
    uint64 now = SDL_GetPerformanceCounter();

    dt = (double64)(now - last_counter) / (double64)perf_freq;
    last_counter = now;

    t_total = (double64)(now - perf_start) / (double64)perf_freq;

    // Debug output
    static uint32 frame_count = 0;
    if (++frame_count % 60 == 0) {
        SDL_Log("FPS: %.1f   dt: %.6f   t_total: %.2f",
            1.0 / dt, dt, t_total);
    }

    // Exit automatically after 5 seconds
    if (t_total > 10) {
        SDL_Log("⏱ Timeout reached, exiting...");
        return SDL_APP_SUCCESS;
    }
    // Determine how much audio is currently queued in the stream
    uint32 queued_bytes = SDL_GetAudioStreamAvailable(audioStream);
    uint32 target_bytes = (uint32)(audioSystem.buffer_size * 3);  // keep ~0.6s buffered

    if(GameUpdateAndRender(&buffer, (float32) t_total, &audioSystem, &soundState, queued_bytes, target_bytes, audioSpec.channels)){
        // Feed the samples to the audio stream
        SDL_PutAudioStreamData(audioStream, audioSystem.samples, audioSystem.buffer_size);
    }

    


    //UpdatePixels(&buffer, (float32) t_total);
    //UpdateAudio(&audioSystem, &soundState);

    SDL_UpdateTexture(texture, NULL, buffer.pixels, buffer.pitch);
    // (buffer.width * 4 ) = how far to move in memory from one row of pixels to the next.
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

// --- Called once on shutdown ---
void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_Log("🧹 Cleaning up...");
    DestroyAudio(&audioSystem);
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (buffer.pixels) {
        free(buffer.pixels);
        buffer.pixels = NULL;
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
    SDL_Quit();
}

void ResizeRenderBuffer(RenderBuffer *buffer, uint32 width, uint32 height){
    
    // Free old buffer if it exists
    if (buffer->pixels) {
        free(buffer->pixels);
        buffer->pixels = NULL;
    }
    // clear old texture
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    // update the buffer
    buffer->height = height;
    buffer->width = width;
    buffer->bytesPerPixel = 4; // ARGB8888  
    buffer->pitch = buffer->width * buffer->bytesPerPixel;

    // allocate space for pixels
    buffer->pixels = calloc(width * height, buffer->bytesPerPixel); // automatically zeroes all bytes
    if (!buffer->pixels) {
        SDL_Log("⚠️ Failed to allocate memory for render buffer pixels");
        return;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         width, height);

    if (!texture) {
        SDL_Log("⚠️ Failed to recreate texture: %s", SDL_GetError());
    } else {
        SDL_Log("📐 Texture recreated: %dx%d", width, height);
    }
}

bool InitAudio(AudioSystem *audioSystem, SoundState *soundState){

    audioSpec.format = SDL_AUDIO_F32;   // 32-bit float audio
    audioSpec.channels = 2;
    audioSpec.freq = SOUND_FREQ;

    audioStream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, 
        &audioSpec, NULL, NULL);
    if (!audioStream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    //UpdateAudio(audioSystem, soundState);

    /* SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start! */
    SDL_ResumeAudioStreamDevice(audioStream);


    // Allocate buffer for our generated samples
    audioSystem->frames_per_buffer = (uint32)(audioSpec.freq * BUFFER_SECONDS);
    audioSystem->samples_per_buffer = audioSystem->frames_per_buffer * audioSpec.channels;
    audioSystem->buffer_size = audioSystem->samples_per_buffer * sizeof(float);
    audioSystem->samples = (float *)SDL_malloc(audioSystem->buffer_size);

    if (!audioSystem->samples) {
        SDL_Log("❌ Failed to allocate audio buffer");
        return false;
    }

    SDL_Log("🔊 Audio initialized: %.1f sec buffer, %d Hz, %d channels",
        BUFFER_SECONDS, audioSpec.freq, audioSpec.channels);

    return true;
}

void DestroyAudio(AudioSystem *audioSystem) {
    if (audioStream) {
        // Pause audio playback before destroying the stream
        SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(audioStream));
        SDL_DestroyAudioStream(audioStream);
        audioStream = NULL;
    }
    if (audioSystem->samples) {
        SDL_free(audioSystem->samples);
        audioSystem->samples = NULL;
    }
    SDL_Log("🧹 Audio system cleaned up");
}

// void UpdateAudio(AudioSystem *audioSystem, SoundState *soundState) {
//     // Determine how much audio is currently queued in the stream
//     uint32 queued_bytes = SDL_GetAudioStreamAvailable(audioStream);
//     uint32 target_bytes = (uint32)(audioSystem->buffer_size * 3);  // keep ~0.6s buffered

//     if (queued_bytes < target_bytes) {
//         uint32 frames = audioSystem->frames_per_buffer;
//         uint32 channels = audioSpec.channels;
//         float *samples = audioSystem->samples;

//         GenerateSineWave(audioSystem, soundState);
//         //GenerateSquareWave(audioSystem, soundState);

//         // Feed the samples to the audio stream
//         SDL_PutAudioStreamData(audioStream, samples, audioSystem->buffer_size);
//     }
// }

void GenerateSineWave(AudioSystem *audioSystem, SoundState *soundState){
    uint32 frames = audioSystem->frames_per_buffer;
    uint32 channels = audioSpec.channels;
    float *samples = audioSystem->samples;
    // Generate a simple sine wave
    for (uint32 i = 0; i < frames; ++i) {
        float sample = sinf(2.0f * (float)PI * soundState->frequency * ((float)i / audioSpec.freq) + soundState->phase)
                        * soundState->amplitude;
        for (uint32 c = 0; c < channels; ++c){
            samples[i * channels + c] = sample;
        }       
    }

    // Advance the phase for continuous playback
    soundState->phase += 2.0f * (float)PI * soundState->frequency * ((float)frames / audioSpec.freq);
    if (soundState->phase > 2.0f * (float)PI){
        soundState->phase -= 2.0f * (float)PI;
    }
        
}

void GenerateSquareWave(AudioSystem *audioSystem, SoundState *soundState){
    uint32 frames = audioSystem->frames_per_buffer;
    uint32 channels = audioSpec.channels;
    float *samples = audioSystem->samples;
    uint32 sample_rate = audioSpec.freq;
    float phase = soundState->phase;
    float phase_increment = (soundState->frequency / (float)sample_rate);  // how much phase advances per sample

    for (uint32 i = 0; i < frames; ++i) {
        // Generate a simple square wave: +amplitude for first half, -amplitude for second half
        float value = (fmodf(phase, 1.0f) < 0.5f) ? soundState->amplitude : -soundState->amplitude;

        // Write to both left and right channels (stereo)
        for (uint32 ch = 0; ch < channels; ++ch) {
            samples[i * channels + ch] = value;
        }

        phase += phase_increment;
        if (phase >= 1.0f){
            phase -= 1.0f;  // wrap phase around after one cycle
        } 
    }

    soundState->phase = phase;  // store phase for continuity across calls

}
