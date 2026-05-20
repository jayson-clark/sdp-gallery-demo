// Gallery manifest.
//
// To add a real game:
//   1. Build it with web/build.sh (output appears at web/dist/<id>/).
//   2. Add an entry below with `available: true` and `id` matching the folder.
//
// Placeholder cards just show a "Coming Soon" overlay — they're not clickable.

window.GALLERY = [
  // --- Real, playable games ---------------------------------------------------
  {
    id: 'student',
    title: 'Galactic Flappy',
    tagline: 'Flappy Bird, but you visit the planets.',
    authors: ['John Doe', 'Jane Smith'],
    semester: 'Fall 2025',
    tags: ['Arcade', 'Side-scroller'],
    description:
    'A Flappy Bird homage with three biomes — Earth, Moon, and Jupiter — ' +
    'each with its own gravity tuning, parallax background, and bird ' +
    'sprite. Drag to start, tap to flap.',
    highlights: [
      'Three distinct gameplay biomes',
      'Sprite-based animation system',
      'Menu navigation with touch input',
    ],
    website: 'https://example.com/team-flappy',
    available: true,
  },
  {
    id: 'ecotoss',
    title: 'EcoToss',
    tagline: 'Recycling themed cup-pong',
    authors: ['Jayson Clark', 'Nithya Kondagari'],
    semester: 'Fall 2023',
    tags: ['3D', 'Physics', 'Arcade'],
    description:
      'A 3D rendered cup-pong game built entirely on the FEH ' +
      'simulator. The team wrote their own software rasterizer, mesh ' +
      'loader, and texture sampler on top of the simulator’s 2D LCD ' +
      'primitives.',
    highlights: [
      'Custom software 3D pipeline',
      '.obj model loading with textured triangles',
      'Persistent high-score tracking',
    ],
    website: 'https://example.com/team-ecotoss',
    available: true,
  },

  // --- Placeholders (for visual layout / "Coming Soon" mock) -----------------
  { title: 'placeholder', available: false },
  { title: 'placeholder', available: false },
  { title: 'placeholder', available: false },
  { title: 'placeholder', available: false },
  { title: 'placeholder', available: false },
  { title: 'placeholder', available: false },
  { title: 'placeholder', available: false },
];
