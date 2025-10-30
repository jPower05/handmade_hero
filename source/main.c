#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <math.h>

#define BUFFER_SECONDS 0.2f  // length of each buffer (100ms)
#define SOUND_FREQ 48000

typedef struct{
    int width;
    int height;
    int bytesPerPixel;
    int pitch; // how many bytes a pointer needs to move to get to the next row
    void *pixels;
    SDL_Texture *texture;
} RenderBuffer;

typedef struct {
    float phase;
    float frequency;
    float amplitude;
} SoundState;

typedef struct {
    SDL_AudioStream *stream;
    SDL_AudioSpec spec;
    float *samples;
    int frames_per_buffer;
    int samples_per_buffer;
    size_t buffer_size;
} AudioSystem;


void ResizeRenderBuffer(RenderBuffer *buffer, int Width, int Height);
void UpdatePixels(RenderBuffer *buffer,float t);
bool InitAudio(AudioSystem *audioSystem, SoundState *soundState);
void UpdateAudio(AudioSystem *audioSystem, SoundState *soundState);
void DestroyAudio(AudioSystem *audioSystem);

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static RenderBuffer buffer = {0};
static AudioSystem audioSystem = {0};
static SoundState soundState = {0};
static SDL_Gamepad *controller = NULL;

// --- Called once at startup ---
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){
    int init_height = 480;
    int init_width = 640;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        SDL_Log("Failed to init SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    window = SDL_CreateWindow("Handmade Hero", init_width, init_height, SDL_WINDOW_RESIZABLE);

    // Initialize audio
    soundState.frequency = 440.0f;  // A4 note
    soundState.amplitude = 0.25f;
    soundState.phase = 0.0f;

    if (!InitAudio(&audioSystem, &soundState)) {
        SDL_Log("⚠️ Audio failed to init");
    }
    
    ResizeRenderBuffer(&buffer, init_width, init_height);

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
            int width = event->window.data1;
            int height = event->window.data2;
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
    static Uint32 start_time = 0;
    if (start_time == 0)
        start_time = SDL_GetTicks();

    Uint32 elapsed = SDL_GetTicks() - start_time;
    float t = elapsed / 1000.0f; // time in seconds

    // Exit automatically after 5 seconds
    if (SDL_GetTicks() - start_time > 10000) {
        SDL_Log("⏱ Timeout reached, exiting...");
        return SDL_APP_SUCCESS;
    }

    UpdatePixels(&buffer, t);
    UpdateAudio(&audioSystem, &soundState);

    SDL_UpdateTexture(buffer.texture, NULL, buffer.pixels, buffer.pitch);
    // (buffer.width * 4 ) = how far to move in memory from one row of pixels to the next.
    SDL_RenderTexture(renderer, buffer.texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

// --- Called once on shutdown ---
void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_Log("🧹 Cleaning up...");
    DestroyAudio(&audioSystem);
    if (buffer.texture) {
        SDL_DestroyTexture(buffer.texture);
        buffer.texture = NULL;
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

void ResizeRenderBuffer(RenderBuffer *buffer, int width, int height){
    
    // Free old buffer if it exists
    if (buffer->pixels) {
        free(buffer->pixels);
        buffer->pixels = NULL;
    }
    // clear old texture
    if (buffer->texture) {
        SDL_DestroyTexture(buffer->texture);
        buffer->texture = NULL;
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

    buffer->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         width, height);

    if (!buffer->texture) {
        SDL_Log("⚠️ Failed to recreate texture: %s", SDL_GetError());
    } else {
        SDL_Log("📐 Texture recreated: %dx%d", width, height);
    }
}

void UpdatePixels(RenderBuffer *buffer, float t){
    // write directly to the buffers pixels

    int height = buffer->height;
    int width = buffer->width;
    uint32_t *pixels = (uint32_t *)buffer->pixels;

    for(int y = 0; y < height; ++y){
        uint32_t *row = (uint32_t *)pixels + (y * width); // pointer to the start of the current row
        for(int x = 0; x < width; ++x){
            uint8_t red = (uint8_t)((sin((x + t *100) * 0.01f) * 0.5f + 0.5f) *255);
            uint8_t blue = (uint8_t)((sin((x + y + t *100) * 0.01f) * 0.5f + 0.5f) *255);
            uint8_t green = (uint8_t)((sin((y + t *100) * 0.01f) * 0.5f + 0.5f) *255);;
            uint8_t alpha = 255;
            // dereference the pointer to set the pixel value
            *(row + x) = blue | (green << 8) | (red << 16) | (alpha << 24);
        }
    }
}

bool InitAudio(AudioSystem *audioSystem, SoundState *soundState){
    
    audioSystem->spec.format = SDL_AUDIO_F32;   // 32-bit float audio
    audioSystem->spec.channels = 2;             // stereo
    audioSystem->spec.freq = SOUND_FREQ;        // 48kHz

    // Open an audio stream for playback
    audioSystem->stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,  // default output
        &audioSystem->spec,                 // desired format
        NULL,                               // actual format (optional)
        NULL                                // callback (we’re streaming manually)
    );

    if (!audioSystem->stream) {
        SDL_Log("❌ Failed to open audio stream: %s", SDL_GetError());
        return false;
    }

    // Resume playback (start the device)
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(audioSystem->stream));

    // Allocate buffer for our generated samples
    audioSystem->frames_per_buffer = (int)(audioSystem->spec.freq * BUFFER_SECONDS);
    audioSystem->samples_per_buffer = audioSystem->frames_per_buffer * audioSystem->spec.channels;
    audioSystem->buffer_size = audioSystem->samples_per_buffer * sizeof(float);
    audioSystem->samples = (float *)SDL_malloc(audioSystem->buffer_size);

    if (!audioSystem->samples) {
        SDL_Log("❌ Failed to allocate audio buffer");
        return false;
    }

    SDL_Log("🔊 Audio initialized: %.1f sec buffer, %d Hz, %d channels",
        BUFFER_SECONDS, audioSystem->spec.freq, audioSystem->spec.channels);

    return true;
}

void DestroyAudio(AudioSystem *audioSystem) {
    if (audioSystem->stream) {
        // Pause audio playback before destroying the stream
        SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(audioSystem->stream));
        SDL_DestroyAudioStream(audioSystem->stream);
        audioSystem->stream = NULL;
    }
    if (audioSystem->samples) {
        SDL_free(audioSystem->samples);
        audioSystem->samples = NULL;
    }
    SDL_Log("🧹 Audio system cleaned up");
}

void UpdateAudio(AudioSystem *audioSystem, SoundState *soundState) {
    // Determine how much audio is currently queued in the stream
    int queued_bytes = SDL_GetAudioStreamAvailable(audioSystem->stream);
    int target_bytes = (int)(audioSystem->buffer_size * 3);  // keep ~0.6s buffered

    if (queued_bytes < target_bytes) {
        int frames = audioSystem->frames_per_buffer;
        int channels = audioSystem->spec.channels;
        float *samples = audioSystem->samples;

        // Generate a simple sine wave
        for (int i = 0; i < frames; ++i) {
            float sample = sinf(2.0f * (float)M_PI * soundState->frequency * ((float)i / audioSystem->spec.freq) + soundState->phase)
                         * soundState->amplitude;
            for (int c = 0; c < channels; ++c)
                samples[i * channels + c] = sample;
        }

        // Advance the phase for continuous playback
        soundState->phase += 2.0f * (float)M_PI * soundState->frequency * ((float)frames / audioSystem->spec.freq);
        if (soundState->phase > 2.0f * (float)M_PI)
            soundState->phase -= 2.0f * (float)M_PI;

        // Feed the samples to the audio stream
        SDL_PutAudioStreamData(audioSystem->stream, samples, audioSystem->buffer_size);
    }
}
