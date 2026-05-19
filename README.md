# FEH Simulator → Web (WebAssembly)

Compile any student FEH project to WebAssembly so it runs in a browser, with
**no changes to the student's source code**. Designed for embedding in a
website gallery via plain `<script>` or `<iframe>`.

## How it works

The FEH simulator libraries are built on top of [TIGR](https://github.com/erkkah/tigr),
a tiny windowing/drawing library that normally uses native OpenGL + Cocoa/X11/Win32.
TIGR doesn't exist on the web, so this directory ships:

- **`tigr_web.cpp`** — a drop-in replacement that implements the same TIGR API
  on top of an HTML5 canvas and Emscripten input events.
- **`overrides/FEHSound_web.cpp`** — a Web Audio replacement for `FEHSound.cpp`
  (the native one uses Cocoa/Win32/ALSA).

The FEH library sources (`FEHLCD.cpp`, `FEHKeyboard.cpp`, `FEHImages.cpp`, etc.)
**compile unchanged** and link against the web shims.

Student code that calls `while(1) { LCD.Update(); }` works because the build
enables Emscripten's **Asyncify**: `tigrUpdate()` yields the Wasm stack to the
browser on every frame, and the student's loop resumes after.

## One-time setup

Install Emscripten:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh         # adds emcc to PATH for this shell
```

## Building a student project

From this directory:

```bash
./build.sh ../cpp_template hello_world
```

Outputs to `web/dist/hello_world/`:

```
index.html      ← standalone preview page
embed.html      ← minimal page intended for iframe embedding
game.js         ← Emscripten-generated module loader
game.wasm       ← the compiled game
game.data       ← (only if the project has an assets/ folder)
style.css
```

To preview locally:

```bash
cd dist/hello_world
python3 -m http.server 8000
# open http://localhost:8000
```

## Embedding in a website

### Option 1 — iframe (zero JS)

Copy the `dist/hello_world/` folder onto any static web host, then anywhere:

```html
<iframe src="https://your.site/hello_world/embed.html"
        width="640" height="480"
        style="border:0"
        allow="autoplay"></iframe>
```

The canvas inside the iframe auto-scales to fit while preserving 4:3 aspect.

### Option 2 — direct embed (more control)

Drop `game.js` (and `game.wasm`, `game.data`) onto your page and point it at
your own canvas:

```html
<canvas id="my-game" width="320" height="240" tabindex="0"
        style="image-rendering: pixelated; width: 640px; height: 480px;"></canvas>

<script src="hello_world/game.js"></script>
<script>
  createFEHGame({ canvas: document.getElementById('my-game') });
</script>
```

`createFEHGame()` returns a promise that resolves when the game is running.

## Gallery layout suggestion

```
gallery/
├── index.html              ← your gallery landing page (lists all games)
├── games/
│   ├── pong/               ← copied from web/dist/pong/
│   ├── snake/
│   └── ...
└── (any static host: GitHub Pages, Netlify, plain S3 — works fine)
```

Each game directory is self-contained — no shared state between games. Each
gets its own iframe and its own Wasm instance.

## Project layout this assumes

```
SDP_Simulator/
├── cpp_template/           ← example student project
│   ├── main.cpp
│   ├── (other student .cpp files)
│   └── simulator_libraries/
│       ├── FEHLCD.cpp ...
│       └── tigr.cpp        ← ignored for web; replaced by tigr_web.cpp
└── web/                    ← this directory
    ├── build.sh
    ├── Makefile
    ├── tigr_web.cpp
    ├── overrides/FEHSound_web.cpp
    ├── shell/              ← HTML/CSS templates
    └── dist/<game>/        ← build outputs
```

`build.sh <project> [name]` expects `<project>` to contain a
`simulator_libraries/` subfolder (run the project's normal `make update` to
populate it from the OSU FEH repo).

## Assets (images, sounds, SD-card files)

If the student project has an `assets/` folder, it's automatically bundled into
the virtual filesystem at `/` so that:

```cpp
FEHImage img;
img.Open("sprite.png");     // resolves to /sprite.png in MEMFS
```

Place files at `cpp_template/assets/sprite.png`. To bundle a different folder,
pass it explicitly: `ASSET_DIR=../my_game/data ./build.sh ../my_game`.

## Tradeoffs / known limitations

- **Asyncify bloat**: adds ~30–50% to wasm size. Per game is ~400–700 KB
  gzipped. Fine for a gallery.
- **`FEHSD` writes don't persist** across reloads. They land in the in-memory
  filesystem only. (Wire up IDBFS in `Module.preRun` if persistence matters.)
- **Sound**: uses `<audio>` elements. Browsers require a user interaction
  before audio plays — autoplay-on-load won't work; trigger sounds from a
  click/keypress.
- **Mouse vs. touch**: a single touch is reported as a left-button mouse
  click via `tigrMouse`, which keeps `LCD.Touch()` working on phones.
- **`tigrPrint`/`tigrLoadFont` are stubbed**. FEHLCD ships its own bitmap font
  and doesn't use them, but if a student game called them directly it would
  draw nothing.
