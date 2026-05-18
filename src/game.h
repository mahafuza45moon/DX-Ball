#pragma once
#include "levels.h"
#include <string>

constexpr float WORLD_W        = 800.0f;
constexpr float WORLD_H        = 600.0f;
constexpr float BALL_SPEED     = 300.0f;     // base / starting ball speed
constexpr float BALL_SPEED_MAX = 600.0f;     // cap on ramped speed
constexpr float BALL_RAMP      = 6.0f;       // u/s added per second elapsed
constexpr float PAD_SPEED      = 400.0f;
constexpr float PAD_W_DEFAULT  = 100.0f;
constexpr float PAD_W_WIDE     = 150.0f;
constexpr float PAD_W_SHRINK   = 60.0f;
constexpr float PERK_DURATION  = 10.0f;      // seconds for timed perks
constexpr float DROP_VY        = -150.0f;    // pixels/sec (falling)
constexpr float DROP_W         = 24.0f;
constexpr float DROP_H         = 14.0f;
constexpr int   MAX_BRICKS     = ROWS * COLS;
constexpr int   MAX_DROPS      = 16;
constexpr int   MENU_COUNT     = 3;          // Start / Help / Exit

enum class BrickType { NONE, NORMAL, STRONG };
enum class GameState { MENU, HELP, WAITING, PLAYING, PAUSED, GAME_OVER, YOU_WIN };
enum class InputSrc  { KEYBOARD, MOUSE };
enum class PerkType  { NONE, EXTRA_LIFE, WIDE_PADDLE, FAST_BALL, SLOW_BALL, SHRINK_PADDLE };

struct Ball   { float x, y, vx, vy, size; };
struct Paddle { float x, y, w, h; };
struct Brick  { BrickType type; int hp; bool alive; float x, y, w, h; };
struct Drop   { PerkType type; float x, y, vy; bool alive; };

void gameInit();
void gameUpdate(float dt);
void gameDraw();
void loadLevel(int n);

// GLUT callbacks registered in main.cpp
void onKeyboard(unsigned char key, int mx, int my);
void onSpecial(int key, int mx, int my);
void onSpecialUp(int key, int mx, int my);
void onMouse(int button, int action, int mx, int my);
void onPassiveMotion(int mx, int my);
void onReshape(int w, int h);
