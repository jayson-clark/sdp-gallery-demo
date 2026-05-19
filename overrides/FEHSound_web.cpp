// FEHSound_web.cpp — Web Audio backend for FEHSound on Emscripten.
// Replaces the native FEHSound.cpp when building for the browser.
//
// Each FEHSound instance maps to a JS-side audio resource (an HTMLAudioElement).
// Decoding/playback is delegated to the browser, so we just shuttle commands.

#include "FEHSound.h"
#include <emscripten.h>
#include <atomic>

namespace {
std::atomic<int> g_nextId{1};
}

EM_JS(void, fehsound_js_load, (int id, const char* path), {
    if (!Module._fehSounds) Module._fehSounds = {};
    var url = UTF8ToString(path);
    var audio = new Audio();
    // Files are mounted under MEMFS via --preload-file; expose via blob URL.
    try {
        var data = FS.readFile(url);
        var blob = new Blob([data], { type: 'audio/wav' });
        audio.src = URL.createObjectURL(blob);
    } catch (e) {
        // Fall back to a relative URL — works if the asset was copied next to index.html.
        audio.src = url;
    }
    audio.preload = 'auto';
    Module._fehSounds[id] = audio;
});

EM_JS(void, fehsound_js_play,    (int id), { var a = Module._fehSounds && Module._fehSounds[id]; if (a) { a.play().catch(function(){}); } });
EM_JS(void, fehsound_js_pause,   (int id), { var a = Module._fehSounds && Module._fehSounds[id]; if (a) { a.pause(); } });
EM_JS(void, fehsound_js_restart, (int id), { var a = Module._fehSounds && Module._fehSounds[id]; if (a) { a.currentTime = 0; a.play().catch(function(){}); } });
EM_JS(void, fehsound_js_seek,    (int id, double t), { var a = Module._fehSounds && Module._fehSounds[id]; if (a) { a.currentTime = t; a.play().catch(function(){}); } });
EM_JS(void, fehsound_js_volume,  (int id, double v), { var a = Module._fehSounds && Module._fehSounds[id]; if (a) { a.volume = Math.max(0, Math.min(1, v)); } });
EM_JS(void, fehsound_js_free,    (int id), { if (Module._fehSounds) { delete Module._fehSounds[id]; } });

// We stash the JS-side id inside FEHSound::tempFilePath (an std::string member
// declared in FEHSound.h). That lets us avoid editing the upstream header.
static int idFromHandle(const std::string& s) { return std::atoi(s.c_str()); }

FEHSound::FEHSound(const std::string& fp) : filepath(fp) {
    volume = 1.0;
    wavLoaded = true;
    lastScaledVolume = 1.0;
    tempFileValid = true;
    int id = g_nextId.fetch_add(1);
    tempFilePath = std::to_string(id);
    fehsound_js_load(id, filepath.c_str());
}

FEHSound::~FEHSound() {
    fehsound_js_free(idFromHandle(tempFilePath));
}

void FEHSound::play()                    { fehsound_js_play(idFromHandle(tempFilePath)); }
void FEHSound::pause()                   { fehsound_js_pause(idFromHandle(tempFilePath)); }
void FEHSound::restart()                 { fehsound_js_restart(idFromHandle(tempFilePath)); }
void FEHSound::playFrom(double time)     { fehsound_js_seek(idFromHandle(tempFilePath), time); }
void FEHSound::setVolume(double vol)     { volume = vol; fehsound_js_volume(idFromHandle(tempFilePath), vol); }
double FEHSound::getVolume() const       { return volume; }

// Stubs for private helpers referenced by the header. Never called on web.
bool FEHSound::loadWavFile() { return true; }
bool FEHSound::createScaledWavFile() { return true; }
void FEHSound::applyVolumeToSamples(std::vector<char>&, int, int, double) {}
void FEHSound::platformInit() {}
void FEHSound::platformCleanup() {}
void FEHSound::platformPlay() {}
void FEHSound::platformPause() {}
void FEHSound::platformRestart() {}
void FEHSound::platformPlayFrom(double) {}
void FEHSound::platformOpenScaledFile() {}
