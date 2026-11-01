# Chess — Local Two-Player

A graphical, local player-versus-player chess game in C++20 using SFML 3.1. Both players take turns at the same board. There is no computer opponent or network dependency.

## Features

- Click-to-move with legal destination and capture highlights
- Castling, en passant, and promotion to queen, rook, bishop, or knight
- Check highlighting, checkmate and stalemate detection
- Draw detection for threefold repetition, the 50-move rule, and insufficient material
- Algebraic move history and captured-piece display
- Undo one move, start a new game, and flip the board
- Transparent piece sprites based on the reference design sheet, colored for both sides
- Bundled Adwaita Sans font, with its SIL Open Font License

## Requirements

- C++20 compiler
- CMake 3.20 or newer
- SFML 3.1 with Graphics, Window, and System modules

Arch Linux setup:

```sh
sudo pacman -S --needed cmake sfml
```

## Build and run

From this repository directory:

```sh
cmake -S . -B build
cmake --build build
./build/chess
```

## Controls

- Click a piece, then click one of its highlighted legal destinations.
- Choose a promotion in the popup or press `Q`, `R`, `B`, or `N`.
- `F2`: new game
- `U`: undo one move
- `F`: flip board
- `Escape`: quit

