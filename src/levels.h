#pragma once

constexpr int ROWS       = 6;
constexpr int COLS       = 10;
constexpr int NUM_LEVELS = 3;

// Tile codes used in level grids.  O = empty, N = normal, S = strong.
enum Tile { O=0, N=1, S=2 };

constexpr Tile LEVEL1[ROWS * COLS] = {
    N, N, N, N, N, N, N, N, N, N,
    N, O, O, O, O, O, O, O, O, N,
    O, O, O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O, O, O,
};

constexpr Tile LEVEL2[ROWS * COLS] = {
    S, S, S, S, S, S, S, S, S, S,
    N, N, N, N, N, N, N, N, N, N,
    O, N, O, N, O, N, O, N, O, N,
    O, O, O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O, O, O,
};

constexpr Tile LEVEL3[ROWS * COLS] = {
    S, N, S, N, S, N, S, N, S, N,
    N, S, N, S, N, S, N, S, N, S,
    S, N, S, N, S, N, S, N, S, N,
    N, N, N, N, N, N, N, N, N, N,
    O, O, O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O, O, O,
};

constexpr const Tile* ALL_LEVELS[NUM_LEVELS] = { LEVEL1, LEVEL2, LEVEL3 };
