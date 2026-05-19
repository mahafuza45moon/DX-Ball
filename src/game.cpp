#include "game.h"
#include "sound.h"
#ifdef _WIN32
#  include <windows.h>
#endif
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <cmath>
#include <cstdlib>
#include <string>
#include <algorithm>

static const float PI = 3.14159265f;

// -----------------------------------------------------------------------
// Global game state
// -----------------------------------------------------------------------
static Ball      ball;
static Paddle    paddle;
static Brick     bricks[MAX_BRICKS];
static Drop      drops[MAX_DROPS];
static int       score;
static int       lives;
static int       currentLevel;
static GameState state;
static InputSrc  lastInput     = InputSrc::KEYBOARD;
static float     winW          = WORLD_W;
static float     winH          = WORLD_H;
static bool      specialDown[256] = {};

// Run / timing
static float     elapsedTime;          // seconds spent in PLAYING
static float     bestTime     = -1.0f; // -1 means no record yet (in-memory only)
static float     ballSpeedNow;         // ramped base speed (before multipliers)

// Perk timers (seconds remaining; 0 = inactive)
static float     wideTimer    = 0.0f;
static float     shrinkTimer  = 0.0f;
static float     fastTimer    = 0.0f;
static float     slowTimer    = 0.0f;

// Menu
static int       menuIndex    = 0;     // 0=Start, 1=Help, 2=Exit

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static bool aabbOverlap(float ax, float ay, float aw, float ah,
                        float bx, float by, float bw, float bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

static void drawRect(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

static void drawBevel(float x, float y, float w, float h,
                      float r, float g, float b, float E = 3.0f)
{
    glColor3f(r * 0.45f, g * 0.45f, b * 0.45f);
    drawRect(x, y, w, h);
    glColor3f(std::min(1.0f, r + 0.28f),
              std::min(1.0f, g + 0.28f),
              std::min(1.0f, b + 0.28f));
    drawRect(x, y + E, w - E, h - E);
    glColor3f(r, g, b);
    drawRect(x + E, y + E, w - 2.0f*E, h - 2.0f*E);
}

static void drawText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);
    for (char c : s)
        glutBitmapCharacter(font, c);
}

// Approximate width of a string in HELVETICA_18 pixels for centring.
static float textWidth(const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18)
{
    float w = 0.0f;
    for (char c : s)
        w += (float)glutBitmapWidth(font, (unsigned char)c);
    return w;
}

static void drawTextCentered(float cx, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18)
{
    drawText(cx - textWidth(s, font) * 0.5f, y, s, font);
}

static float randf01()
{
    return (float)std::rand() / (float)RAND_MAX;
}

// Compute current effective ball speed, factoring active multipliers.
static float effectiveBallSpeed()
{
    float s = ballSpeedNow;
    if (fastTimer > 0.0f) s *= 1.4f;
    if (slowTimer > 0.0f) s *= 0.7f;
    return s;
}

// Renormalise the ball velocity to match the given target speed.
static void rescaleBallSpeed(float target)
{
    float mag = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
    if (mag < 0.001f) return;
    ball.vx = ball.vx / mag * target;
    ball.vy = ball.vy / mag * target;
}

// -----------------------------------------------------------------------
// Ball reset / launch
// -----------------------------------------------------------------------
static void resetBall()
{
    ball.size = 12.0f;
    ball.x    = paddle.x + paddle.w / 2.0f - ball.size / 2.0f;
    ball.y    = paddle.y + paddle.h + 1.0f;
    ball.vx   = 0.0f;
    ball.vy   = 0.0f;
    state     = GameState::WAITING;
}

static void launchBall()
{
    if (state != GameState::WAITING) return;
    float angle = 25.0f * PI / 180.0f;
    float s     = effectiveBallSpeed();
    ball.vx = s * std::sin(angle);
    ball.vy = s * std::cos(angle);
    state   = GameState::PLAYING;
}

// -----------------------------------------------------------------------
// Drops
// -----------------------------------------------------------------------
static void clearDrops()
{
    for (int i = 0; i < MAX_DROPS; ++i) drops[i].alive = false;
}

static void spawnDrop(float cx, float cy)
{
    // 25% chance to spawn anything at all
    if (randf01() > 0.25f) return;

    int slot = -1;
    for (int i = 0; i < MAX_DROPS; ++i) {
        if (!drops[i].alive) { slot = i; break; }
    }
    if (slot < 0) return;

    // Pick a perk type. Weighted distribution: positives more common than negatives.
    float r = randf01();
    PerkType t;
    if      (r < 0.20f) t = PerkType::EXTRA_LIFE;     // 20%
    else if (r < 0.45f) t = PerkType::WIDE_PADDLE;    // 25%
    else if (r < 0.65f) t = PerkType::FAST_BALL;      // 20%
    else if (r < 0.85f) t = PerkType::SLOW_BALL;      // 20%
    else                t = PerkType::SHRINK_PADDLE;  // 15%

    Drop& d = drops[slot];
    d.type  = t;
    d.x     = cx - DROP_W / 2.0f;
    d.y     = cy;
    d.vy    = DROP_VY;
    d.alive = true;
}

static void applyPerk(PerkType t)
{
    switch (t) {
        case PerkType::EXTRA_LIFE:
            ++lives;
            break;
        case PerkType::WIDE_PADDLE:
            paddle.w    = PAD_W_WIDE;
            wideTimer   = PERK_DURATION;
            shrinkTimer = 0.0f;
            break;
        case PerkType::SHRINK_PADDLE:
            paddle.w    = PAD_W_SHRINK;
            shrinkTimer = PERK_DURATION;
            wideTimer   = 0.0f;
            break;
        case PerkType::FAST_BALL:
            fastTimer = PERK_DURATION;
            slowTimer = 0.0f;
            rescaleBallSpeed(effectiveBallSpeed());
            break;
        case PerkType::SLOW_BALL:
            slowTimer = PERK_DURATION;
            fastTimer = 0.0f;
            rescaleBallSpeed(effectiveBallSpeed());
            break;
        default: break;
    }
    // Keep paddle on-screen if it just grew/shrunk.
    paddle.x = std::clamp(paddle.x, 0.0f, WORLD_W - paddle.w);
}

// -----------------------------------------------------------------------
// Level loading
// -----------------------------------------------------------------------
void loadLevel(int n)
{
    const Tile* grid      = ALL_LEVELS[n];
    const float SLOT_W    = 80.0f;
    const float SLOT_H    = 24.0f;
    const float BRICK_W   = 76.0f;
    const float BRICK_H   = 20.0f;
    const float BRICK_TOP = 560.0f;

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int    idx  = r * COLS + c;
            Tile   cell = grid[idx];
            Brick& b    = bricks[idx];

            b.x = c * SLOT_W + 2.0f;
            b.y = BRICK_TOP - (float)r * SLOT_H - BRICK_H;
            b.w = BRICK_W;
            b.h = BRICK_H;

            switch (cell) {
                case N:
                    b.type  = BrickType::NORMAL;
                    b.hp    = 1;
                    b.alive = true;
                    break;
                case S:
                    b.type  = BrickType::STRONG;
                    b.hp    = 2;
                    b.alive = true;
                    break;
                default:
                    b.type  = BrickType::NONE;
                    b.hp    = 0;
                    b.alive = false;
                    break;
            }
        }
    }
    clearDrops();
    resetBall();
}

// Reset everything except bestTime / currentLevel to start a fresh run.
static void startNewRun()
{
    paddle.w     = PAD_W_DEFAULT;
    paddle.h     = 15.0f;
    paddle.x     = WORLD_W / 2.0f - paddle.w / 2.0f;
    paddle.y     = 20.0f;

    score        = 0;
    lives        = 3;
    currentLevel = 0;
    elapsedTime  = 0.0f;
    ballSpeedNow = BALL_SPEED;
    wideTimer = shrinkTimer = fastTimer = slowTimer = 0.0f;
    loadLevel(currentLevel);
}

// -----------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------
void gameInit()
{
    soundInit();
    startNewRun();
    state = GameState::MENU;   // override: start at menu instead of WAITING
    menuIndex = 0;
}

// -----------------------------------------------------------------------
// Update helpers
// -----------------------------------------------------------------------
static void updateDrops(float dt)
{
    for (int i = 0; i < MAX_DROPS; ++i) {
        Drop& d = drops[i];
        if (!d.alive) continue;
        d.y += d.vy * dt;
        if (d.y + DROP_H < 0.0f) { d.alive = false; continue; }
        if (aabbOverlap(d.x, d.y, DROP_W, DROP_H,
                        paddle.x, paddle.y, paddle.w, paddle.h)) {
            applyPerk(d.type);
            d.alive = false;
        }
    }
}

static void updatePerkTimers(float dt)
{
    if (wideTimer > 0.0f) {
        wideTimer -= dt;
        if (wideTimer <= 0.0f) {
            wideTimer = 0.0f;
            paddle.w  = PAD_W_DEFAULT;
            paddle.x  = std::clamp(paddle.x, 0.0f, WORLD_W - paddle.w);
        }
    }
    if (shrinkTimer > 0.0f) {
        shrinkTimer -= dt;
        if (shrinkTimer <= 0.0f) {
            shrinkTimer = 0.0f;
            paddle.w    = PAD_W_DEFAULT;
        }
    }
    bool wasFast = fastTimer > 0.0f;
    bool wasSlow = slowTimer > 0.0f;
    if (fastTimer > 0.0f) fastTimer = std::max(0.0f, fastTimer - dt);
    if (slowTimer > 0.0f) slowTimer = std::max(0.0f, slowTimer - dt);
    if ((wasFast && fastTimer == 0.0f) || (wasSlow && slowTimer == 0.0f)) {
        rescaleBallSpeed(effectiveBallSpeed());
    }
}

// -----------------------------------------------------------------------
// Update (called every frame with fixed dt = 16ms)
// -----------------------------------------------------------------------
void gameUpdate(float dt)
{
    // Pre-game states: nothing to simulate.
    if (state == GameState::MENU || state == GameState::HELP ||
        state == GameState::PAUSED || state == GameState::GAME_OVER ||
        state == GameState::YOU_WIN) {
        return;
    }

    // Keyboard paddle movement
    if (lastInput == InputSrc::KEYBOARD) {
        if (specialDown[GLUT_KEY_LEFT])  paddle.x -= PAD_SPEED * dt;
        if (specialDown[GLUT_KEY_RIGHT]) paddle.x += PAD_SPEED * dt;
        paddle.x = std::clamp(paddle.x, 0.0f, WORLD_W - paddle.w);
    }

    // Glue ball to paddle while waiting to launch
    if (state == GameState::WAITING) {
        ball.x = paddle.x + paddle.w / 2.0f - ball.size / 2.0f;
        ball.y = paddle.y + paddle.h + 1.0f;
        updateDrops(dt);        // drops still fall during WAITING
        updatePerkTimers(dt);
        return;
    }

    // -- PLAYING below --
    elapsedTime += dt;

    // Gradual ball speed ramp
    float newBase = std::min(BALL_SPEED_MAX, BALL_SPEED + BALL_RAMP * elapsedTime);
    if (std::fabs(newBase - ballSpeedNow) > 0.001f) {
        ballSpeedNow = newBase;
        rescaleBallSpeed(effectiveBallSpeed());
    }

    updatePerkTimers(dt);
    updateDrops(dt);

    // Move ball
    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    // Wall collisions
    bool wallHit = false;
    if (ball.x < 0.0f) {
        ball.x  = 0.0f;
        ball.vx = std::fabs(ball.vx);
        wallHit = true;
    }
    if (ball.x + ball.size > WORLD_W) {
        ball.x  = WORLD_W - ball.size;
        ball.vx = -std::fabs(ball.vx);
        wallHit = true;
    }
    if (ball.y + ball.size > WORLD_H) {
        ball.y  = WORLD_H - ball.size;
        ball.vy = -std::fabs(ball.vy);
        wallHit = true;
    }
    if (wallHit) soundPlayWallBounce();

    // Ball fell below screen — lose a life
    if (ball.y < 0.0f) {
        soundPlayLifeLost();
        --lives;
        if (lives <= 0)
            state = GameState::GAME_OVER;
        else
            resetBall();
        return;
    }

    // Paddle collision
    if (aabbOverlap(ball.x, ball.y, ball.size, ball.size,
                    paddle.x, paddle.y, paddle.w, paddle.h))
    {
        float ballCX  = ball.x + ball.size / 2.0f;
        float normHit = (ballCX - paddle.x) / paddle.w;
        normHit = std::clamp(normHit, 0.0f, 1.0f);
        float angle = (normHit - 0.5f) * (120.0f * PI / 180.0f);
        float s     = effectiveBallSpeed();
        ball.vx = s * std::sin(angle);
        ball.vy = s * std::cos(angle);
        ball.y  = paddle.y + paddle.h;
        soundPlayPaddleHit();
    }

    // Brick collisions — resolve one brick per frame
    for (int i = 0; i < MAX_BRICKS; ++i) {
        Brick& b = bricks[i];
        if (!b.alive) continue;

        if (!aabbOverlap(ball.x, ball.y, ball.size, ball.size,
                         b.x, b.y, b.w, b.h)) continue;

        float ox = std::min(ball.x + ball.size, b.x + b.w) - std::max(ball.x, b.x);
        float oy = std::min(ball.y + ball.size, b.y + b.h) - std::max(ball.y, b.y);
        if (ox < oy) ball.vx = -ball.vx;
        else         ball.vy = -ball.vy;

        --b.hp;
        if (b.hp <= 0) {
            b.alive = false;
            score  += (b.type == BrickType::NORMAL) ? 10 : 20;
            soundPlayBrickBreak();
            // Maybe spawn a falling perk where the brick was.
            spawnDrop(b.x + b.w / 2.0f, b.y + b.h / 2.0f);
        }
        break;
    }

    // Win check
    bool anyAlive = false;
    for (int i = 0; i < MAX_BRICKS; ++i) {
        if (bricks[i].alive) { anyAlive = true; break; }
    }
    if (!anyAlive) {
        ++currentLevel;
        if (currentLevel >= NUM_LEVELS) {
            state = GameState::YOU_WIN;
            if (bestTime < 0.0f || elapsedTime < bestTime)
                bestTime = elapsedTime;
        } else {
            loadLevel(currentLevel);
        }
    }
}

// -----------------------------------------------------------------------
// Drawing helpers for each non-gameplay screen
// -----------------------------------------------------------------------
static const char* perkLabel(PerkType t)
{
    switch (t) {
        case PerkType::EXTRA_LIFE:    return "+";
        case PerkType::WIDE_PADDLE:   return "W";
        case PerkType::SHRINK_PADDLE: return "X";
        case PerkType::FAST_BALL:     return "F";
        case PerkType::SLOW_BALL:     return "S";
        default:                      return "?";
    }
}

static void perkColor(PerkType t, float& r, float& g, float& b)
{
    switch (t) {
        case PerkType::EXTRA_LIFE:    r=0.20f; g=0.85f; b=0.30f; break; // green
        case PerkType::WIDE_PADDLE:   r=0.20f; g=0.80f; b=0.85f; break; // cyan
        case PerkType::SHRINK_PADDLE: r=0.90f; g=0.20f; b=0.20f; break; // red
        case PerkType::FAST_BALL:     r=0.90f; g=0.30f; b=0.85f; break; // magenta
        case PerkType::SLOW_BALL:     r=0.55f; g=0.70f; b=0.95f; break; // light blue
        default:                      r=g=b=0.7f; break;
    }
}

static void drawDrop(const Drop& d)
{
    float r, g, b;
    perkColor(d.type, r, g, b);
    drawBevel(d.x, d.y, DROP_W, DROP_H, r, g, b, 2.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(d.x + DROP_W * 0.5f - 4.0f, d.y + 2.0f, perkLabel(d.type), GLUT_BITMAP_HELVETICA_12);
}

static std::string formatTime(float t)
{
    int total = (int)t;
    int mm = total / 60;
    int ss = total % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
    return std::string(buf);
}

static void drawMenu()
{
    glColor3f(1.0f, 0.85f, 0.20f);
    drawTextCentered(WORLD_W * 0.5f, 470.0f, "BRICK  BREAKER", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.7f, 0.7f, 0.7f);
    drawTextCentered(WORLD_W * 0.5f, 440.0f, "DX-Ball style — clear all 3 levels as fast as you can");

    const char* items[MENU_COUNT] = { "Start Game", "Help", "Exit" };
    for (int i = 0; i < MENU_COUNT; ++i) {
        float y = 340.0f - (float)i * 40.0f;
        if (i == menuIndex) {
            glColor3f(1.0f, 1.0f, 0.30f);
            drawTextCentered(WORLD_W * 0.5f, y, std::string("> ") + items[i] + " <", GLUT_BITMAP_HELVETICA_18);
        } else {
            glColor3f(0.85f, 0.85f, 0.85f);
            drawTextCentered(WORLD_W * 0.5f, y, items[i]);
        }
    }

    if (bestTime >= 0.0f) {
        glColor3f(0.30f, 1.0f, 0.30f);
        drawTextCentered(WORLD_W * 0.5f, 160.0f, "Best time: " + formatTime(bestTime));
    }
    glColor3f(0.65f, 0.65f, 0.65f);
    drawTextCentered(WORLD_W * 0.5f, 60.0f, "Use Arrow Keys + Enter, or click an item");
    drawTextCentered(WORLD_W * 0.5f, 35.0f, "Esc = quit anytime");
}

static void drawHelp()
{
    glColor3f(1.0f, 0.85f, 0.20f);
    drawTextCentered(WORLD_W * 0.5f, 540.0f, "HELP", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.95f, 0.95f, 0.95f);
    drawText(150.0f, 490.0f, "Controls:");
    glColor3f(0.85f, 0.85f, 0.85f);
    drawText(170.0f, 465.0f, "Left / Right arrows   - move paddle");
    drawText(170.0f, 440.0f, "Mouse                 - paddle follows cursor X");
    drawText(170.0f, 415.0f, "Space / Left click    - launch the ball");
    drawText(170.0f, 390.0f, "P                     - pause / unpause");
    drawText(170.0f, 365.0f, "R                     - restart current run");
    drawText(170.0f, 340.0f, "M                     - back to menu (from pause / end)");
    drawText(170.0f, 315.0f, "Esc                   - quit");

    glColor3f(0.95f, 0.95f, 0.95f);
    drawText(150.0f, 275.0f, "Perks (fall from destroyed bricks - catch with paddle):");

    PerkType perks[] = {
        PerkType::EXTRA_LIFE, PerkType::WIDE_PADDLE, PerkType::FAST_BALL,
        PerkType::SLOW_BALL,  PerkType::SHRINK_PADDLE
    };
    const char* desc[] = {
        "+   Extra life",
        "W   Wider paddle (10 s)",
        "F   Faster ball (10 s)",
        "S   Slower ball (10 s)",
        "X   Shrink paddle (10 s) - bad!"
    };
    for (int i = 0; i < 5; ++i) {
        float y = 245.0f - i * 30.0f;
        float r,g,b; perkColor(perks[i], r,g,b);
        drawBevel(170.0f, y, DROP_W, DROP_H, r,g,b, 2.0f);
        glColor3f(0.0f,0.0f,0.0f);
        drawText(170.0f + DROP_W*0.5f - 4.0f, y + 2.0f, perkLabel(perks[i]), GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.90f, 0.90f, 0.90f);
        drawText(210.0f, y + 1.0f, desc[i]);
    }

    glColor3f(0.65f, 0.65f, 0.65f);
    drawTextCentered(WORLD_W * 0.5f, 50.0f, "Press Esc or M to return to menu");
}

static void drawActivePerksHud()
{
    float x = 10.0f, y = 575.0f;
    auto drawTimer = [&](PerkType t, float remaining) {
        if (remaining <= 0.0f) return;
        float r,g,b; perkColor(t, r,g,b);
        drawBevel(x, y, DROP_W, DROP_H, r,g,b, 2.0f);
        glColor3f(0.0f,0.0f,0.0f);
        drawText(x + DROP_W*0.5f - 4.0f, y + 2.0f, perkLabel(t), GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.95f, 0.95f, 0.95f);
        char buf[16]; std::snprintf(buf, sizeof(buf), "%.0fs", remaining);
        drawText(x + DROP_W + 4.0f, y + 1.0f, buf, GLUT_BITMAP_HELVETICA_12);
        x += DROP_W + 36.0f;
    };
    drawTimer(PerkType::WIDE_PADDLE,   wideTimer);
    drawTimer(PerkType::SHRINK_PADDLE, shrinkTimer);
    drawTimer(PerkType::FAST_BALL,     fastTimer);
    drawTimer(PerkType::SLOW_BALL,     slowTimer);
}

// -----------------------------------------------------------------------
// Draw
// -----------------------------------------------------------------------
void gameDraw()
{
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (state == GameState::MENU) { drawMenu(); glutSwapBuffers(); return; }
    if (state == GameState::HELP) { drawHelp(); glutSwapBuffers(); return; }

    // Bricks
    for (int i = 0; i < MAX_BRICKS; ++i) {
        const Brick& b = bricks[i];
        if (!b.alive) continue;
        if (b.type == BrickType::NORMAL) {
            drawBevel(b.x, b.y, b.w, b.h, 0.25f, 0.45f, 0.95f);
        } else {
            if (b.hp >= 2) drawBevel(b.x, b.y, b.w, b.h, 0.90f, 0.20f, 0.20f);
            else           drawBevel(b.x, b.y, b.w, b.h, 0.95f, 0.55f, 0.10f);
        }
    }

    // Drops
    for (int i = 0; i < MAX_DROPS; ++i)
        if (drops[i].alive) drawDrop(drops[i]);

    // Paddle (tint when shrunk/wide for feedback)
    float pr=0.72f, pg=0.72f, pb=0.82f;
    if (wideTimer   > 0.0f) { pr=0.40f; pg=0.85f; pb=0.95f; }
    if (shrinkTimer > 0.0f) { pr=0.95f; pg=0.45f; pb=0.45f; }
    drawBevel(paddle.x, paddle.y, paddle.w, paddle.h, pr, pg, pb);

    // Ball
    float br=1.0f, bg=0.88f, bb=0.08f;
    if (fastTimer > 0.0f) { br=1.0f; bg=0.40f; bb=0.90f; }
    if (slowTimer > 0.0f) { br=0.55f; bg=0.75f; bb=1.0f; }
    drawBevel(ball.x, ball.y, ball.size, ball.size, br, bg, bb, 2.0f);

    // HUD (top of screen — pushed up to row 575 so it stays away from bricks)
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(10.0f,  8.0f, "Score: " + std::to_string(score));
    drawText(220.0f, 8.0f, "Lives: " + std::to_string(lives));
    drawText(380.0f, 8.0f, "Time: "  + formatTime(elapsedTime));
    drawText(560.0f, 8.0f, "Lvl: "   + std::to_string(currentLevel + 1) + "/" + std::to_string(NUM_LEVELS));
    drawText(670.0f, 8.0f, "Speed: " + std::to_string((int)effectiveBallSpeed()));

    drawActivePerksHud();

    // State overlays
    if (state == GameState::WAITING) {
        glColor3f(0.80f, 0.80f, 0.80f);
        drawTextCentered(WORLD_W * 0.5f, 200.0f, "Press SPACE or click to launch!");
    }
    if (state == GameState::PAUSED) {
        glColor3f(1.0f, 1.0f, 0.20f);
        drawTextCentered(WORLD_W * 0.5f, 330.0f, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.80f, 0.80f, 0.80f);
        drawTextCentered(WORLD_W * 0.5f, 295.0f, "P = continue   M = main menu   Esc = quit");
    }
    if (state == GameState::GAME_OVER) {
        glColor3f(1.0f, 0.20f, 0.20f);
        drawTextCentered(WORLD_W * 0.5f, 340.0f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.80f, 0.80f, 0.80f);
        drawTextCentered(WORLD_W * 0.5f, 305.0f, "R = restart    M = main menu    Esc = quit");
        drawTextCentered(WORLD_W * 0.5f, 280.0f, "Score: " + std::to_string(score) + "    Time: " + formatTime(elapsedTime));
    }
    if (state == GameState::YOU_WIN) {
        glColor3f(0.20f, 1.0f, 0.20f);
        drawTextCentered(WORLD_W * 0.5f, 340.0f, "YOU WIN!", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.80f, 0.80f, 0.80f);
        drawTextCentered(WORLD_W * 0.5f, 305.0f, "Score: " + std::to_string(score) + "    Time: " + formatTime(elapsedTime));
        if (bestTime >= 0.0f)
            drawTextCentered(WORLD_W * 0.5f, 280.0f, "Best time: " + formatTime(bestTime));
        drawTextCentered(WORLD_W * 0.5f, 250.0f, "R = play again    M = main menu    Esc = quit");
    }

    glutSwapBuffers();
}

// -----------------------------------------------------------------------
// Input callbacks
// -----------------------------------------------------------------------
static void activateMenuItem()
{
    switch (menuIndex) {
        case 0:  // Start
            startNewRun();   // resets state to WAITING
            break;
        case 1:  // Help
            state = GameState::HELP;
            break;
        case 2:  // Exit
            std::exit(0);
    }
}

void onKeyboard(unsigned char key, int /*mx*/, int /*my*/)
{
    // Menu input
    if (state == GameState::MENU) {
        if (key == 13 || key == ' ') {        // Enter or Space activates
            activateMenuItem();
        } else if (key == 27) {
            std::exit(0);
        }
        return;
    }
    if (state == GameState::HELP) {
        if (key == 27 || key == 'm' || key == 'M')
            state = GameState::MENU;
        return;
    }

    switch (key) {
        case ' ':
            launchBall();
            break;
        case 'p': case 'P':
            if      (state == GameState::PLAYING) state = GameState::PAUSED;
            else if (state == GameState::PAUSED)  state = GameState::PLAYING;
            break;
        case 'r': case 'R':
            startNewRun();
            break;
        case 'm': case 'M':
            // Allowed from PAUSED / GAME_OVER / YOU_WIN
            if (state == GameState::PAUSED || state == GameState::GAME_OVER ||
                state == GameState::YOU_WIN) {
                state     = GameState::MENU;
                menuIndex = 0;
            }
            break;
        case 27:   // Esc
            std::exit(0);
    }
}

void onSpecial(int key, int /*mx*/, int /*my*/)
{
    if (state == GameState::MENU) {
        if (key == GLUT_KEY_UP)   menuIndex = (menuIndex + MENU_COUNT - 1) % MENU_COUNT;
        if (key == GLUT_KEY_DOWN) menuIndex = (menuIndex + 1) % MENU_COUNT;
        return;
    }
    if (key < 256) specialDown[key] = true;
    lastInput = InputSrc::KEYBOARD;
}

void onSpecialUp(int key, int /*mx*/, int /*my*/)
{
    if (key < 256) specialDown[key] = false;
}

void onMouse(int button, int action, int mx, int my)
{
    if (button != GLUT_LEFT_BUTTON || action != GLUT_DOWN) return;

    if (state == GameState::MENU) {
        // Convert window pixel Y to world Y (origin at bottom-left).
        float worldY = WORLD_H - (float)my / winH * WORLD_H;
        for (int i = 0; i < MENU_COUNT; ++i) {
            float yTop = 340.0f - (float)i * 40.0f + 18.0f;
            float yBot = 340.0f - (float)i * 40.0f - 4.0f;
            if (worldY <= yTop && worldY >= yBot) {
                menuIndex = i;
                activateMenuItem();
                return;
            }
        }
        return;
    }
    if (state == GameState::HELP) {
        state = GameState::MENU;
        return;
    }
    launchBall();
}

void onPassiveMotion(int mx, int /*my*/)
{
    if (state == GameState::MENU || state == GameState::HELP) return;
    lastInput = InputSrc::MOUSE;
    float worldX = (float)mx / winW * WORLD_W;
    paddle.x     = worldX - paddle.w / 2.0f;
    paddle.x     = std::clamp(paddle.x, 0.0f, WORLD_W - paddle.w);
}

void onReshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = (float)w;
    winH = (float)h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, WORLD_W, 0.0, WORLD_H, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
