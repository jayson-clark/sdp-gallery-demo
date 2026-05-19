// tigr_web.cpp — Drop-in TIGR replacement for the FEH Simulator on the web.
//
// Implements just the subset of the TIGR API used by the FEH simulator
// libraries (FEHLCD, FEHKeyboard, FEHImages, etc.), backed by an HTML5
// canvas and Emscripten input events. Compiled with -sASYNCIFY so that
// tigrUpdate() can yield to the browser without student code changes.

#include "tigr.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <vector>
#include <algorithm>
#include <chrono>

#include <emscripten.h>
#include <emscripten/html5.h>

// STB_IMAGE_STATIC keeps every stb_* symbol local to this translation unit.
// Necessary because student projects (e.g. EcoToss) sometimes ship their own
// copy of stb_image with STB_IMAGE_IMPLEMENTATION elsewhere — two extern
// definitions of the same symbol would be a duplicate-symbol link error.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"

// --------------------------------------------------------------------------
// Internal: input state shared with JS
// --------------------------------------------------------------------------

// 256 key slots → 8 uint32_t words (matches tigrKeyboardData() contract).
static uint32_t g_keys_held[8]    = {0};
static uint32_t g_keys_pressed[8] = {0};   // cleared after read this frame
static int      g_last_char       = 0;
static int      g_mouse_x         = 0;
static int      g_mouse_y         = 0;
static int      g_mouse_buttons   = 0;
static int      g_window_closed   = 0;

static inline void set_bit(uint32_t* arr, int bit) { arr[(bit >> 5) & 7] |= (1u << (bit & 31)); }
static inline void clr_bit(uint32_t* arr, int bit) { arr[(bit >> 5) & 7] &= ~(1u << (bit & 31)); }

// Called from JS on key events. Code is a TKey value (0–255).
extern "C" EMSCRIPTEN_KEEPALIVE void tigr_web_key_event(int code, int down) {
    if (code < 0 || code > 255) return;
    if (down) {
        if (!(g_keys_held[(code >> 5) & 7] & (1u << (code & 31)))) {
            set_bit(g_keys_pressed, code);
        }
        set_bit(g_keys_held, code);
    } else {
        clr_bit(g_keys_held, code);
    }
}

extern "C" EMSCRIPTEN_KEEPALIVE void tigr_web_char_event(int codepoint) {
    g_last_char = codepoint;
}

extern "C" EMSCRIPTEN_KEEPALIVE void tigr_web_mouse_event(int x, int y, int buttons) {
    g_mouse_x = x;
    g_mouse_y = y;
    g_mouse_buttons = buttons;
}

extern "C" EMSCRIPTEN_KEEPALIVE void tigr_web_close_event() {
    g_window_closed = 1;
}

// --------------------------------------------------------------------------
// JS-side: canvas wiring + event listeners.
// Runs exactly once when tigrWindow() is first called.
// --------------------------------------------------------------------------

EM_JS(void, tigr_web_js_init, (int w, int h), {
    if (Module._tigrInitialised) return;
    Module._tigrInitialised = true;

    var canvas = Module.canvas;
    if (!canvas) {
        canvas = document.getElementById('canvas');
        Module.canvas = canvas;
    }
    if (!canvas) {
        console.error('[tigr_web] No canvas element found. Set Module.canvas before run.');
        return;
    }
    canvas.width  = w;
    canvas.height = h;
    Module._tigrCtx = canvas.getContext('2d');
    Module._tigrImage = Module._tigrCtx.createImageData(w, h);

    // ---- Key mapping (DOM KeyboardEvent.code → TKey scancode) ----
    // TKey enum: TK_PAD0=128, F1=144, BACKSPACE=156, ESCAPE=164, LEFT=170...
    var map = {
        'Backspace':156, 'Tab':157, 'Enter':158, 'NumpadEnter':140,
        'ShiftLeft':180, 'ShiftRight':181, 'ControlLeft':182, 'ControlRight':183,
        'AltLeft':184, 'AltRight':185, 'Pause':162, 'CapsLock':163,
        'Escape':164, 'Space':165, 'PageUp':166, 'PageDown':167,
        'End':168, 'Home':169, 'ArrowLeft':170, 'ArrowUp':171,
        'ArrowRight':172, 'ArrowDown':173, 'Insert':174, 'Delete':175,
        'MetaLeft':176, 'MetaRight':177, 'NumLock':178, 'ScrollLock':179,
        'Semicolon':186, 'Equal':187, 'Comma':188, 'Minus':189,
        'Period':190, 'Slash':191, 'Backquote':192, 'BracketLeft':193,
        'Backslash':194, 'BracketRight':195, 'Quote':196,
        'Numpad0':128,'Numpad1':129,'Numpad2':130,'Numpad3':131,'Numpad4':132,
        'Numpad5':133,'Numpad6':134,'Numpad7':135,'Numpad8':136,'Numpad9':137,
        'NumpadMultiply':138,'NumpadAdd':139,'NumpadSubtract':141,
        'NumpadDecimal':142,'NumpadDivide':143
    };
    for (var i = 1; i <= 12; i++) map['F'+i] = 143 + i;          // F1=144 ... F12=155
    for (var i = 0; i <= 9; i++) map['Digit'+i] = 48 + i;        // '0'..'9'
    for (var i = 0; i < 26; i++) map['Key'+String.fromCharCode(65+i)] = 65 + i; // 'A'..'Z'

    function translate(e) {
        if (map[e.code] !== undefined) return map[e.code];
        // Fallback: single printable char → its uppercase ASCII code
        if (e.key && e.key.length === 1) return e.key.toUpperCase().charCodeAt(0);
        return -1;
    }

    // Attach to window so the embed works whether or not the canvas has focus.
    window.addEventListener('keydown', function(e) {
        var c = translate(e);
        if (c >= 0) {
            Module._tigr_web_key_event(c, 1);
            // Prevent the browser from scrolling / tab-switching on game keys.
            if (e.code === 'Tab' || e.code === 'Space' ||
                e.code === 'ArrowUp' || e.code === 'ArrowDown' ||
                e.code === 'ArrowLeft' || e.code === 'ArrowRight') {
                e.preventDefault();
            }
        }
        if (e.key && e.key.length === 1) {
            Module._tigr_web_char_event(e.key.charCodeAt(0));
        } else if (e.key === 'Enter') {
            Module._tigr_web_char_event(10);
        } else if (e.key === 'Backspace') {
            Module._tigr_web_char_event(8);
        }
    });
    window.addEventListener('keyup', function(e) {
        var c = translate(e);
        if (c >= 0) Module._tigr_web_key_event(c, 0);
    });

    // ---- Mouse / touch ----
    function updateMouse(e) {
        var rect = canvas.getBoundingClientRect();
        var sx = canvas.width  / rect.width;
        var sy = canvas.height / rect.height;
        var x = Math.floor((e.clientX - rect.left) * sx);
        var y = Math.floor((e.clientY - rect.top)  * sy);
        Module._tigr_web_mouse_event(x, y, Module._tigrMouseButtons | 0);
    }
    Module._tigrMouseButtons = 0;
    canvas.addEventListener('mousemove', updateMouse);
    canvas.addEventListener('mousedown', function(e) {
        Module._tigrMouseButtons |= (1 << e.button);
        updateMouse(e);
    });
    canvas.addEventListener('mouseup', function(e) {
        Module._tigrMouseButtons &= ~(1 << e.button);
        updateMouse(e);
    });
    canvas.addEventListener('contextmenu', function(e) { e.preventDefault(); });

    function updateTouch(e) {
        if (e.touches.length === 0) {
            Module._tigrMouseButtons = 0;
            Module._tigr_web_mouse_event(Module._tigrLastX|0, Module._tigrLastY|0, 0);
            return;
        }
        var t = e.touches[0];
        var rect = canvas.getBoundingClientRect();
        var sx = canvas.width  / rect.width;
        var sy = canvas.height / rect.height;
        var x = Math.floor((t.clientX - rect.left) * sx);
        var y = Math.floor((t.clientY - rect.top)  * sy);
        Module._tigrLastX = x; Module._tigrLastY = y;
        Module._tigrMouseButtons = 1;
        Module._tigr_web_mouse_event(x, y, 1);
        e.preventDefault();
    }
    canvas.addEventListener('touchstart', updateTouch, { passive: false });
    canvas.addEventListener('touchmove',  updateTouch, { passive: false });
    canvas.addEventListener('touchend',   updateTouch, { passive: false });
});

EM_JS(void, tigr_web_js_present, (uintptr_t pixPtr, int w, int h), {
    var ctx = Module._tigrCtx;
    var img = Module._tigrImage;
    if (!ctx || !img) return;
    // Tigr stores TPixel as r,g,b,a — same layout as ImageData. One memcpy.
    var heap = HEAPU8.subarray(pixPtr, pixPtr + w * h * 4);
    img.data.set(heap);
    ctx.putImageData(img, 0, 0);
});

// --------------------------------------------------------------------------
// Pixel-op helpers (pure C++, no platform deps)
// --------------------------------------------------------------------------

static inline TPixel* px(Tigr* b, int x, int y) { return &b->pix[y * b->w + x]; }

// --------------------------------------------------------------------------
// TIGR API implementations
// --------------------------------------------------------------------------

static Tigr* g_mainWindow = nullptr;

extern "C" Tigr* tigrWindow(int w, int h, const char* /*title*/, int /*flags*/) {
    Tigr* b = (Tigr*)calloc(1, sizeof(Tigr));
    b->w = w;
    b->h = h;
    b->pix = (TPixel*)calloc(w * h, sizeof(TPixel));
    b->handle = (void*)1; // non-null marks this as a "window"
    tigr_web_js_init(w, h);
    g_mainWindow = b;
    return b;
}

extern "C" Tigr* tigrBitmap(int w, int h) {
    Tigr* b = (Tigr*)calloc(1, sizeof(Tigr));
    b->w = w;
    b->h = h;
    b->pix = (TPixel*)calloc(w * h, sizeof(TPixel));
    b->handle = nullptr;
    return b;
}

extern "C" void tigrFree(Tigr* b) {
    if (!b) return;
    if (b == g_mainWindow) g_mainWindow = nullptr;
    free(b->pix);
    free(b);
}

extern "C" int tigrClosed(Tigr* /*b*/) { return g_window_closed; }

extern "C" void tigrUpdate(Tigr* b) {
    if (b && b == g_mainWindow) {
        tigr_web_js_present((uintptr_t)b->pix, b->w, b->h);
    }
    // Clear "just-pressed" edges before yielding, so the next frame starts fresh.
    for (int i = 0; i < 8; i++) g_keys_pressed[i] = 0;
    // Yield to the browser so it can render the canvas update we just posted
    // and dispatch any pending input events. 0ms here means "as soon as
    // possible" — we don't artificially cap the frame rate. A native game
    // running at 200fps gets to run at 200fps on the web too.
    emscripten_sleep(0);
}

extern "C" TPixel tigrGet(Tigr* b, int x, int y) {
    if (x < 0 || y < 0 || x >= b->w || y >= b->h) { TPixel z = {0,0,0,0}; return z; }
    return *px(b, x, y);
}

extern "C" void tigrPlot(Tigr* b, int x, int y, TPixel p) {
    if (x < 0 || y < 0 || x >= b->w || y >= b->h) return;
    *px(b, x, y) = p;
}

extern "C" void tigrClear(Tigr* b, TPixel c) {
    int n = b->w * b->h;
    for (int i = 0; i < n; i++) b->pix[i] = c;
}

extern "C" void tigrFill(Tigr* b, int x, int y, int w, int h, TPixel c) {
    int x0 = std::max(0, x), y0 = std::max(0, y);
    int x1 = std::min(b->w, x + w), y1 = std::min(b->h, y + h);
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++)
            *px(b, xx, yy) = c;
}

extern "C" void tigrRect(Tigr* b, int x, int y, int w, int h, TPixel c) {
    if (w <= 0 || h <= 0) return;
    for (int i = 0; i < w; i++) {
        tigrPlot(b, x + i, y, c);
        tigrPlot(b, x + i, y + h - 1, c);
    }
    for (int i = 0; i < h; i++) {
        tigrPlot(b, x, y + i, c);
        tigrPlot(b, x + w - 1, y + i, c);
    }
}

extern "C" void tigrLine(Tigr* b, int x0, int y0, int x1, int y1, TPixel c) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        tigrPlot(b, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static inline TPixel blend(TPixel dst, TPixel src, float alpha) {
    float a = (src.a / 255.0f) * alpha;
    TPixel out;
    out.r = (unsigned char)(src.r * a + dst.r * (1 - a));
    out.g = (unsigned char)(src.g * a + dst.g * (1 - a));
    out.b = (unsigned char)(src.b * a + dst.b * (1 - a));
    out.a = 0xff;
    return out;
}

extern "C" void tigrBlit(Tigr* dst, Tigr* src, int dx, int dy, int sx, int sy, int w, int h) {
    for (int yy = 0; yy < h; yy++) {
        int dyy = dy + yy, syy = sy + yy;
        if (dyy < 0 || dyy >= dst->h || syy < 0 || syy >= src->h) continue;
        for (int xx = 0; xx < w; xx++) {
            int dxx = dx + xx, sxx = sx + xx;
            if (dxx < 0 || dxx >= dst->w || sxx < 0 || sxx >= src->w) continue;
            *px(dst, dxx, dyy) = *px(src, sxx, syy);
        }
    }
}

extern "C" void tigrBlitAlpha(Tigr* dst, Tigr* src, int dx, int dy, int sx, int sy, int w, int h, float alpha) {
    for (int yy = 0; yy < h; yy++) {
        int dyy = dy + yy, syy = sy + yy;
        if (dyy < 0 || dyy >= dst->h || syy < 0 || syy >= src->h) continue;
        for (int xx = 0; xx < w; xx++) {
            int dxx = dx + xx, sxx = sx + xx;
            if (dxx < 0 || dxx >= dst->w || sxx < 0 || sxx >= src->w) continue;
            *px(dst, dxx, dyy) = blend(*px(dst, dxx, dyy), *px(src, sxx, syy), alpha);
        }
    }
}

extern "C" void tigrBlitTint(Tigr* dst, Tigr* src, int dx, int dy, int sx, int sy, int w, int h, TPixel tint) {
    for (int yy = 0; yy < h; yy++) {
        int dyy = dy + yy, syy = sy + yy;
        if (dyy < 0 || dyy >= dst->h || syy < 0 || syy >= src->h) continue;
        for (int xx = 0; xx < w; xx++) {
            int dxx = dx + xx, sxx = sx + xx;
            if (dxx < 0 || dxx >= dst->w || sxx < 0 || sxx >= src->w) continue;
            TPixel s = *px(src, sxx, syy);
            TPixel out;
            out.r = (unsigned char)((s.r * tint.r) / 255);
            out.g = (unsigned char)((s.g * tint.g) / 255);
            out.b = (unsigned char)((s.b * tint.b) / 255);
            out.a = (unsigned char)((s.a * tint.a) / 255);
            if (out.a > 0) *px(dst, dxx, dyy) = out;
        }
    }
}

// --------------------------------------------------------------------------
// Input API
// --------------------------------------------------------------------------

extern "C" void tigrMouse(Tigr* /*b*/, int* x, int* y, int* buttons) {
    if (x) *x = g_mouse_x;
    if (y) *y = g_mouse_y;
    if (buttons) *buttons = g_mouse_buttons;
}

extern "C" int tigrTouch(Tigr* b, TigrTouchPoint* points, int maxPoints) {
    if (g_mouse_buttons && maxPoints > 0 && points) {
        points[0].x = g_mouse_x;
        points[0].y = g_mouse_y;
        return 1;
    }
    return 0;
}

extern "C" int tigrKeyDown(Tigr* /*b*/, int key) {
    if (key < 0 || key > 255) return 0;
    return (g_keys_pressed[(key >> 5) & 7] & (1u << (key & 31))) ? 1 : 0;
}

extern "C" int tigrKeyHeld(Tigr* /*b*/, int key) {
    if (key < 0 || key > 255) return 0;
    return (g_keys_held[(key >> 5) & 7] & (1u << (key & 31))) ? 1 : 0;
}

extern "C" int tigrReadChar(Tigr* /*b*/) {
    int c = g_last_char;
    g_last_char = 0;
    return c;
}

std::vector<uint32_t> tigrKeyboardData(Tigr* /*b*/) {
    return std::vector<uint32_t>(g_keys_held, g_keys_held + 8);
}

// --------------------------------------------------------------------------
// Image I/O (PNG via stb_image)
// --------------------------------------------------------------------------

extern "C" Tigr* tigrLoadImageMem(const void* data, int length) {
    int w, h, comp;
    unsigned char* raw = stbi_load_from_memory((const unsigned char*)data, length, &w, &h, &comp, 4);
    if (!raw) return nullptr;
    Tigr* b = tigrBitmap(w, h);
    std::memcpy(b->pix, raw, w * h * 4);
    stbi_image_free(raw);
    return b;
}

extern "C" Tigr* tigrLoadImage(const char* fileName) {
    FILE* f = std::fopen(fileName, "rb");
    if (!f) return nullptr;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<unsigned char> buf(sz);
    if (std::fread(buf.data(), 1, sz, f) != (size_t)sz) { std::fclose(f); return nullptr; }
    std::fclose(f);
    return tigrLoadImageMem(buf.data(), (int)sz);
}

extern "C" int tigrSaveImage(const char* /*fileName*/, Tigr* /*b*/) { return 0; }

// --------------------------------------------------------------------------
// Misc TIGR helpers used incidentally
// --------------------------------------------------------------------------

extern "C" float tigrTime() {
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;
    return dt;
}

extern "C" void tigrError(Tigr* /*b*/, const char* msg, ...) {
    va_list ap; va_start(ap, msg);
    std::vfprintf(stderr, msg, ap);
    va_end(ap);
    std::fputc('\n', stderr);
}

extern "C" void* tigrReadFile(const char* fileName, int* length) {
    FILE* f = std::fopen(fileName, "rb");
    if (!f) return nullptr;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    char* buf = (char*)std::malloc(sz + 1);
    if (std::fread(buf, 1, sz, f) != (size_t)sz) { std::free(buf); std::fclose(f); return nullptr; }
    std::fclose(f);
    buf[sz] = 0;
    if (length) *length = (int)sz;
    return buf;
}

// Unused by FEH libs but referenced by tigr.h — provide stubs so any
// stray student usage links cleanly.
extern "C" int   tigrBeginOpenGL(Tigr*)                 { return 0; }
extern "C" void  tigrSetPostShader(Tigr*, const char*, int) {}
extern "C" void  tigrSetPostFX(Tigr*, float, float, float, float) {}
extern "C" TigrFont* tigrLoadFont(Tigr*, int)           { return nullptr; }
extern "C" void  tigrFreeFont(TigrFont*)                {}
extern "C" void  tigrPrint(Tigr*, TigrFont*, int, int, TPixel, const char*, ...) {}
extern "C" int   tigrTextWidth(TigrFont*, const char*)  { return 0; }
extern "C" int   tigrTextHeight(TigrFont*, const char*) { return 0; }
TigrFont* tfont = nullptr;
extern "C" int   tigrInflate(void*, unsigned, const void*, unsigned) { return 0; }

extern "C" const char* tigrDecodeUTF8(const char* text, int* cp) {
    unsigned char c = (unsigned char)*text;
    if (c < 0x80) { *cp = c; return text + 1; }
    // Minimal UTF-8 decode (good enough for ASCII-heavy classroom games).
    *cp = '?';
    return text + 1;
}

extern "C" char* tigrEncodeUTF8(char* text, int cp) {
    if (cp < 0x80) { *text++ = (char)cp; return text; }
    *text++ = '?';
    return text;
}
