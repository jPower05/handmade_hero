#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <math.h>

typedef struct{
    int width;
    int height;
    int bytesPerPixel;
    void *pixels;
    SDL_Texture *texture;
} RenderBuffer;

void SDLResizeRenderBuffer(RenderBuffer *buffer, int Width, int Height);
void SDLUpdatePixels(RenderBuffer *buffer,float t);

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static RenderBuffer buffer = {0};
static bool isWhite = true;

// --- Called once at startup ---
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){
    int init_height = 480;
    int init_width = 640;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to init SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    window = SDL_CreateWindow("Handmade Hero", init_width, init_height, SDL_WINDOW_RESIZABLE);

    renderer = SDL_CreateRenderer(window, NULL);
    
    SDLResizeRenderBuffer(&buffer, init_width, init_height);
    
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
            SDLResizeRenderBuffer(&buffer, width, height);
        }
        break;
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

    SDLUpdatePixels(&buffer, t);

    SDL_UpdateTexture(buffer.texture, NULL, buffer.pixels, buffer.width * 4);
    // (buffer.width * 4 ) = how far to move in memory from one row of pixels to the next.
    SDL_RenderTexture(renderer, buffer.texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

// --- Called once on shutdown ---
void SDL_AppQuit(void *appstate, SDL_AppResult result){
    SDL_Log("🧹 Cleaning up...");
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
    SDL_Quit();
}

void SDLResizeRenderBuffer(RenderBuffer *buffer, int width, int height){
    
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

void SDLUpdatePixels(RenderBuffer *buffer, float t){
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
