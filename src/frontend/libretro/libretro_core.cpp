#include "trident/libretro.h"

#include "../core/cpu/cpu.hpp"
#include "../core/memory/memory.hpp"
#include "../core/kernel/kernel.hpp"
#include "../core/gpu/gpu.hpp"
#include "../core/audio/audio.hpp"
#include "../core/loader/loader.hpp"
#include "../core/patcher/patcher.hpp"

#include <cstring>
#include <string>
#include <vector>
#include <cstdarg>

#define TRIDENT_VERSION "0.1.0"

static retro_environment_t envCb;
static retro_video_refresh_t videoCb;
static retro_audio_sample_batch_t audioBatchCb;
static retro_audio_sample_t audioSampleCb;
static retro_input_poll_t inputPollCb;
static retro_input_state_t inputStateCb;

static retro_log_callback logCb;

static void logPrintf(int level, const char* fmt, ...) {
    if (!logCb.log) return;
    va_list args;
    va_start(args, fmt);
    char buf[2048];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    logCb.log(level, "%s", buf);
}

static std::unique_ptr<Trident::CPU>    cpu;
static std::unique_ptr<Trident::Memory> memory;
static std::unique_ptr<Trident::Kernel> kernel;
static std::unique_ptr<Trident::GPU>    gpu;
static std::unique_ptr<Trident::Audio>  audio;

static std::vector<uint32_t> framebuffer;
static std::string systemDir;
static std::string saveDir;
static bool gameLoaded = false;

// Memory descriptors for RetroAchievements
static std::vector<retro_memory_descriptor> memDescs;
static retro_memory_map memMap;

static void setupMemoryMap() {
    memDescs.clear();
    auto regions = memory->getContiguousMappedRegions();
    for (auto& region : regions) {
        retro_memory_descriptor desc = {};
        desc.flags = 0;
        desc.ptr = region.pointer;
        desc.offset = 0;
        desc.start = region.start;
        desc.select = 0;
        desc.disconnect = 0;
        desc.len = region.length;
        desc.addrspace = nullptr;
        memDescs.push_back(desc);
    }

    if (!memDescs.empty()) {
        memMap.descriptors = memDescs.data();
        memMap.num_descriptors = static_cast<unsigned>(memDescs.size());
        envCb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &memMap);
    }
}

void retro_set_environment(retro_environment_t cb) {
    envCb = cb;

    bool achievementsSupport = true;
    envCb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievementsSupport);

    bool noGame = false;
    envCb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &noGame);

    struct retro_variable vars[] = {
        {"trident_patch_path", "ROM Patch File; disabled"},
        {"trident_ai_upscale", "AI Texture Upscaling; disabled|2x|4x"},
        {"trident_system_language", "System Language; English|Japanese|French|German|Italian|Spanish"},
        {nullptr, nullptr},
    };
    envCb(RETRO_ENVIRONMENT_SET_VARIABLES, vars);
}

void retro_set_video_refresh(retro_video_refresh_t cb) { videoCb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audioSampleCb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audioBatchCb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { inputPollCb = cb; }
void retro_set_input_state(retro_input_state_t cb) { inputStateCb = cb; }

void retro_init(void) {
    envCb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logCb);

    const char* dir = nullptr;
    if (envCb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir) {
        systemDir = dir;
    }
    if (envCb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir) {
        saveDir = dir;
    }

    cpu = std::make_unique<Trident::CPU>();
    memory = std::make_unique<Trident::Memory>();
    kernel = std::make_unique<Trident::Kernel>();
    gpu = std::make_unique<Trident::GPU>();
    audio = std::make_unique<Trident::Audio>();

    framebuffer.resize(Trident::GPU::OUTPUT_WIDTH * Trident::GPU::OUTPUT_HEIGHT);
}

void retro_deinit(void) {
    cpu.reset();
    memory.reset();
    kernel.reset();
    gpu.reset();
    audio.reset();
    gameLoaded = false;
}

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

void retro_get_system_info(struct retro_system_info* info) {
    info->library_name = "Trident";
    info->library_version = TRIDENT_VERSION;
    info->valid_extensions = "3ds|3dsx|elf|axf|cci|cxi|app|ncch";
    info->need_fullpath = true;
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info* info) {
    info->geometry.base_width = Trident::GPU::OUTPUT_WIDTH;
    info->geometry.base_height = Trident::GPU::OUTPUT_HEIGHT;
    info->geometry.max_width = Trident::GPU::OUTPUT_WIDTH;
    info->geometry.max_height = Trident::GPU::OUTPUT_HEIGHT;
    info->geometry.aspect_ratio = static_cast<float>(Trident::GPU::OUTPUT_WIDTH) /
                                  static_cast<float>(Trident::GPU::OUTPUT_HEIGHT);
    info->timing.fps = 60.0;
    info->timing.sample_rate = Trident::Audio::SAMPLE_RATE;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
    (void)port; (void)device;
}

void retro_reset(void) {
    if (!gameLoaded) return;
    cpu->reset();
    memory->reset();
    kernel->reset();
    gpu->reset();
    audio->reset();
}

bool retro_load_game(const struct retro_game_info* game) {
    if (!game || !game->path) return false;

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    envCb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

    // Initialize subsystems
    memory->init(false);
    cpu->init(*memory);
    kernel->init(*cpu, *memory);
    gpu->init(*memory);
    audio->init(*memory);

    // Set up default memory mappings
    // Code region
    memory->mapPages(Trident::VADDR_CODE_START, memory->getFCRAM(),
                     Trident::VADDR_CODE_END - Trident::VADDR_CODE_START,
                     Trident::PageFlags::All);

    // Heap region
    memory->mapPages(Trident::VADDR_HEAP_START,
                     memory->getFCRAM() + (Trident::VADDR_CODE_END - Trident::VADDR_CODE_START),
                     Trident::VADDR_HEAP_END - Trident::VADDR_HEAP_START,
                     Trident::PageFlags::ReadWrite);

    // VRAM
    memory->mapPages(Trident::VADDR_VRAM_START, memory->getVRAM(),
                     Trident::VRAM_SIZE, Trident::PageFlags::ReadWrite);

    // Load ROM
    auto result = Trident::Loader::loadFile(game->path, *memory);
    if (!result.success) {
        logPrintf(RETRO_LOG_ERROR, "[Trident] Failed to load ROM: %s\n",
                  result.errorMessage.c_str());
        return false;
    }

    logPrintf(RETRO_LOG_INFO, "[Trident] Loaded ROM, entry=0x%08X, programId=0x%016llX\n",
              result.entryPoint, result.programId);

    // Set up CPU entry point
    cpu->setPC(result.entryPoint);
    cpu->setSP(Trident::VADDR_HEAP_END - 0x1000); // Top of heap as initial stack

    // Install SVC handler
    cpu->setSVCHandler([](uint32_t svc) { kernel->handleSVC(svc); });

    // Set up RetroAchievements memory map
    setupMemoryMap();

    // Input descriptors
    struct retro_input_descriptor desc[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right"},
        {0, 0, 0, 0, nullptr},
    };
    envCb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    gameLoaded = true;
    return true;
}

void retro_unload_game(void) {
    gameLoaded = false;
}

void retro_run(void) {
    if (!gameLoaded) return;

    inputPollCb();

    // Read input
    // TODO: Map libretro input to 3DS HID registers

    // Run CPU for one frame (~268 MHz / 60 fps = ~4.47M cycles)
    constexpr uint64_t CYCLES_PER_FRAME = 268111856 / 60;
    cpu->run(CYCLES_PER_FRAME);

    // Render
    gpu->renderFrame(framebuffer.data());
    videoCb(framebuffer.data(), Trident::GPU::OUTPUT_WIDTH,
            Trident::GPU::OUTPUT_HEIGHT,
            Trident::GPU::OUTPUT_WIDTH * sizeof(uint32_t));

    // Audio
    int16_t audioBuffer[Trident::Audio::SAMPLES_PER_FRAME * 2];
    size_t samples = audio->generateSamples(audioBuffer, Trident::Audio::SAMPLES_PER_FRAME);
    audioBatchCb(audioBuffer, samples);
}

size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void* data, size_t size) { (void)data; (void)size; return false; }
bool retro_unserialize(const void* data, size_t size) { (void)data; (void)size; return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char* code) {
    (void)index; (void)enabled; (void)code;
}
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void* retro_get_memory_data(unsigned id) {
    if (id == RETRO_MEMORY_SYSTEM_RAM && memory) return memory->getFCRAM();
    return nullptr;
}

size_t retro_get_memory_size(unsigned id) {
    if (id == RETRO_MEMORY_SYSTEM_RAM && memory) return memory->getFCRAMSize();
    return 0;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info) {
    (void)game_type; (void)info; (void)num_info;
    return false;
}
