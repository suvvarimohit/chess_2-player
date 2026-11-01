# Chess — Local Two-Player

A local, human-versus-human chess game. The desktop app uses C++20 and SFML 3.1. The browser build reuses the same C++ chess rules as WebAssembly and recreates the desktop board layout in a responsive HTML canvas. There is no computer opponent or network dependency during play.

## Features

- Click-to-move with legal destination and capture highlights
- Castling, en passant, and promotion to queen, rook, bishop, or knight
- Check highlighting, checkmate, stalemate, threefold repetition, 50-move rule, and insufficient-material draw detection
- Algebraic move history and captured-piece display
- Undo one move, start a new game, and flip the board
- Same piece sprites, colors, board coordinates, move panel, and layout in desktop and browser builds
- Bundled piece sprites and Adwaita Sans font with their license notices

## Desktop GUI

Requirements: a C++20 compiler, CMake 3.20 or newer, and SFML 3.1 with Graphics, Window, and System modules.

Arch Linux:

```sh
sudo pacman -S --needed cmake sfml
```

Build and run from the repository root:

```sh
cmake -S . -B build
cmake --build build
./build/chess
```

This is the native C++/SFML GUI build. It uses the `chess` target and the `build/` directory.

## Browser build

Requirements: the Emscripten SDK and a web browser. The browser UI is HTML canvas; all chess legality, state, notation, undo, and draw detection are implemented by the shared C++ engine compiled to WebAssembly.

After installing and activating Emscripten, build from the repository root:

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --target chess_web
```

The browser package is generated in `build-web/web/`. Serve it over HTTP (browsers do not reliably load WebAssembly from `file://`):

```sh
cd build-web/web
python3 -m http.server 8000
```

Open `http://localhost:8000` in a browser. The Emscripten target does not find or link SFML, and it uses `build-web/`; the native target remains available separately in `build/`.

## GitHub Pages deployment

To publish from this repository, first build the browser version above. Set **Settings → Pages → Build and deployment → Source** to **Deploy from a branch**, choose `gh-pages` and `/ (root)`, then publish the generated files with:

```sh
./scripts/deploy-pages.sh
```

The script publishes only the generated browser files to the `gh-pages` branch; it does not change the native source branch. After GitHub finishes publishing, open:

```text
https://suvvarimohit.github.io/chess_2-player/
```

## Controls

- Click a piece, then click one of its highlighted legal destinations.
- Choose a promotion in the popup or press `Q`, `R`, `B`, or `N`.
- `F2`: new game
- `U`: undo one move
- `F`: flip board
- Desktop `Escape`: quit
- Browser `Escape`: clear selection or close the promotion picker; close the tab to quit.
