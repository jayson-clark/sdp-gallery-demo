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
    id: 'ecotoss',
    title: 'EcoToss',
    tagline: 'Recycling themed cup-pong',
    authors: ['Jayson Clark', 'Nithya Kondagari'],
    semester: 'Spring 2025',
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
  {
    id: 'student',
    title: 'Galactic Flappy',
    tagline: 'Flappy Bird, but you visit the planets.',
    authors: ['Sam Rivera', 'Taylor Brooks'],
    semester: 'Spring 2025',
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

  // --- Placeholders (for visual layout / "Coming Soon" mock) -----------------
  {
    title: 'Echoes of Aurora',
    tagline: 'A puzzle platformer about light and sound.',
    authors: ['Riley Okafor', 'Morgan Yu'],
    semester: 'Spring 2025',
    tags: ['Puzzle', 'Platformer'],
    description: 'Lorem ipsum placeholder description.',
    available: false,
  },
  {
    title: 'Conveyor Chaos',
    tagline: 'Sort the packages before the queue jams.',
    authors: ['Devon Park', 'Casey Singh'],
    semester: 'Spring 2025',
    tags: ['Puzzle', 'Time-attack'],
    description: 'Lorem ipsum placeholder description.',
    available: false,
  },
  {
    title: 'Pixel Patrol',
    tagline: 'Top-down stealth at 60×45 pixels per tile.',
    authors: ['Quinn Alvarez', 'Skye Nakamura'],
    semester: 'Spring 2025',
    tags: ['Stealth', 'Top-down'],
    description: 'Lorem ipsum placeholder description.',
    available: false,
  },
  {
    title: 'Tidepool Tactics',
    tagline: 'Turn-based combat between sea creatures.',
    authors: ['Harper Ali', 'Reese Wong'],
    semester: 'Spring 2025',
    tags: ['Strategy', 'Turn-based'],
    description: 'Lorem ipsum placeholder description.',
    available: false,
  },
  {
    title: 'Synth Garden',
    tagline: 'Grow plants by playing a melodic loop.',
    authors: ['Eli Tran', 'Frankie Bell'],
    semester: 'Spring 2025',
    tags: ['Music', 'Creative'],
    description: 'Lorem ipsum placeholder description.',
    available: false,
  },
  {
    title: 'Crater Run',
    tagline: 'Endless runner across a procedural lunar surface.',
    authors: ['Sage Holloway', 'Avery Daiyo'],
    semester: 'Spring 2025',
    tags: ['Runner', 'Procedural'],
    description: 'Lorem ipsum placeholder description.',
    available: false,
  },
  {
    title: 'Lantern Letters',
    tagline: 'A typing rhythm game lit by paper lanterns.',
    authors: ['Nico Vargas', 'Hayden Liu'],
    semester: 'Spring 2025',
    tags: ['Typing', 'Rhythm'],
    description: 'Lorem ipsum placeholder description.',
    available: false,
  },
];
