#include "TouchControls.hpp"
#include <cstring>
#include <cmath>

TouchControls::TouchControls() : visible(true), scale(4) {
    std::memset(touchToButton, -1, sizeof(touchToButton));
    std::memset(lastPressed, 0, sizeof(lastPressed));
    recalculateLayout();
}

void TouchControls::recalculateLayout() {
    int s = scale;

    int halfArm = 10 * s / 4;
    int gap = 1;
    int cx = 38;
    int cy = 130;

    // D-pad: 4 directional arms
    // Up
    buttons[0].rect = {cx - halfArm, cy - halfArm * 2 - gap, halfArm * 2, halfArm};
    buttons[0].nesButton = BUTTON_UP;
    buttons[0].color = 0x30FFFFFF;
    buttons[0].pressedColor = 0x70FFFFFF;
    buttons[0].label = "U";
    // Down
    buttons[1].rect = {cx - halfArm, cy + halfArm + gap, halfArm * 2, halfArm};
    buttons[1].nesButton = BUTTON_DOWN;
    buttons[1].color = 0x30FFFFFF;
    buttons[1].pressedColor = 0x70FFFFFF;
    buttons[1].label = "D";
    // Left
    buttons[2].rect = {cx - halfArm * 2 - gap, cy - halfArm, halfArm, halfArm * 2};
    buttons[2].nesButton = BUTTON_LEFT;
    buttons[2].color = 0x30FFFFFF;
    buttons[2].pressedColor = 0x70FFFFFF;
    buttons[2].label = "L";
    // Right
    buttons[3].rect = {cx + halfArm + gap, cy - halfArm, halfArm, halfArm * 2};
    buttons[3].nesButton = BUTTON_RIGHT;
    buttons[3].color = 0x30FFFFFF;
    buttons[3].pressedColor = 0x70FFFFFF;
    buttons[3].label = "R";
    // Center (dead zone)
    buttons[4].rect = {cx - halfArm, cy - halfArm, halfArm * 2, halfArm * 2};
    buttons[4].nesButton = BUTTON_A;
    buttons[4].color = 0x10FFFFFF;
    buttons[4].pressedColor = 0x10FFFFFF;
    buttons[4].label = "";

    // B (left)
    int bSize = 9 * s / 4;
    int bCX = 215;
    int bCY = 140;
    buttons[5].rect = {bCX - bSize - 8, bCY - bSize, bSize * 2, bSize * 2};
    buttons[5].nesButton = BUTTON_B;
    buttons[5].color = 0x30FFAAAA;
    buttons[5].pressedColor = 0x80FF5555;
    buttons[5].label = "B";

    // A (right)
    buttons[6].rect = {bCX + bSize - 8, bCY - bSize, bSize * 2, bSize * 2};
    buttons[6].nesButton = BUTTON_A;
    buttons[6].color = 0x30AAFFAA;
    buttons[6].pressedColor = 0x8055FF55;
    buttons[6].label = "A";

    // Select
    int ssW = 14 * s / 4;
    int ssH = 5 * s / 4;
    int ssY = 228;
    buttons[7].rect = {128 - ssW - 3, ssY, ssW, ssH};
    buttons[7].nesButton = BUTTON_SELECT;
    buttons[7].color = 0x30CCCFFF;
    buttons[7].pressedColor = 0x7099AAFF;
    buttons[7].label = "SE";

    // Start
    buttons[8].rect = {128 + 3, ssY, ssW, ssH};
    buttons[8].nesButton = BUTTON_START;
    buttons[8].color = 0x30CCCFFF;
    buttons[8].pressedColor = 0x7099AAFF;
    buttons[8].label = "ST";

    // Settings gear (top right)
    int gSize = 8 * s / 4;
    buttons[9].rect = {248 - gSize, 4, gSize * 2, gSize * 2};
    buttons[9].nesButton = BUTTON_START;
    buttons[9].color = 0x40CCCCCC;
    buttons[9].pressedColor = 0x80FFFFFF;
    buttons[9].label = "O";
}

void TouchControls::handleEvent(const SDL_Event& event, Controller& controller1) {
    if (!visible) return;

    if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP ||
        event.type == SDL_FINGERMOTION) {

        float tx = event.tfinger.x;
        float ty = event.tfinger.y;
        int nx = (int)(tx * RENDER_WIDTH);
        int ny = (int)(ty * RENDER_HEIGHT);
        int fingerId = event.tfinger.fingerId;
        int action = event.type;

        if (action == SDL_FINGERDOWN || action == SDL_FINGERMOTION) {
            int matchedButton = -1;
            for (int i = 0; i < NUM_BUTTONS; i++) {
                SDL_Rect& r = buttons[i].rect;
                if (nx >= r.x && nx < r.x + r.w && ny >= r.y && ny < r.y + r.h) {
                    matchedButton = i;
                    break;
                }
            }

            if (action == SDL_FINGERDOWN) {
                if (matchedButton >= 0) {
                    touchToButton[fingerId % 16] = matchedButton;
                    buttons[matchedButton].pressed = true;
                    controller1.setButtonState(buttons[matchedButton].nesButton, true);
                    if (!lastPressed[matchedButton]) {
                        lastPressed[matchedButton] = true;
                        vibrateFeedback();
                    }
                }
            } else {
                int prevButton = touchToButton[fingerId % 16];
                if (prevButton >= 0 && prevButton != matchedButton) {
                    buttons[prevButton].pressed = false;
                    controller1.setButtonState(buttons[prevButton].nesButton, false);
                    touchToButton[fingerId % 16] = -1;
                }
                if (matchedButton >= 0 && prevButton != matchedButton) {
                    touchToButton[fingerId % 16] = matchedButton;
                    buttons[matchedButton].pressed = true;
                    controller1.setButtonState(buttons[matchedButton].nesButton, true);
                    if (!lastPressed[matchedButton]) {
                        lastPressed[matchedButton] = true;
                        vibrateFeedback();
                    }
                }
            }
        } else if (action == SDL_FINGERUP) {
            int prevButton = touchToButton[fingerId % 16];
            if (prevButton >= 0) {
                buttons[prevButton].pressed = false;
                controller1.setButtonState(buttons[prevButton].nesButton, false);
                touchToButton[fingerId % 16] = -1;
            }
        }
    }
}

void TouchControls::vibrateFeedback() {
    extern void androidVibrate(int ms);
    androidVibrate(15);
}

uint32_t TouchControls::blend(uint32_t bg, uint32_t fg) {
    uint32_t bgA = (bg >> 24) & 0xFF;
    uint32_t bgR = (bg >> 16) & 0xFF;
    uint32_t bgG = (bg >> 8) & 0xFF;
    uint32_t bgB = bg & 0xFF;

    uint32_t fgA = (fg >> 24) & 0xFF;
    uint32_t fgR = (fg >> 16) & 0xFF;
    uint32_t fgG = (fg >> 8) & 0xFF;
    uint32_t fgB = fg & 0xFF;

    uint32_t outA = 255;
    uint32_t outR = (fgR * fgA + bgR * (255 - fgA)) / 255;
    uint32_t outG = (fgG * fgA + bgG * (255 - fgA)) / 255;
    uint32_t outB = (fgB * fgA + bgB * (255 - fgA)) / 255;

    return (outA << 24) | (outR << 16) | (outG << 8) | outB;
}

void TouchControls::drawRoundRect(uint32_t* buffer, int x, int y, int w, int h, int r, uint32_t color) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            if (px < 0 || px >= RENDER_WIDTH || py < 0 || py >= RENDER_HEIGHT) continue;

            bool inCircle = false;
            if (px < x + r && py < y + r) {
                int dx = (x + r - 1) - px;
                int dy = (y + r - 1) - py;
                inCircle = (dx * dx + dy * dy <= r * r);
            } else if (px >= x + w - r && py < y + r) {
                int dx = px - (x + w - r);
                int dy = (y + r - 1) - py;
                inCircle = (dx * dx + dy * dy <= r * r);
            } else if (px < x + r && py >= y + h - r) {
                int dx = (x + r - 1) - px;
                int dy = py - (y + h - r);
                inCircle = (dx * dx + dy * dy <= r * r);
            } else if (px >= x + w - r && py >= y + h - r) {
                int dx = px - (x + w - r);
                int dy = py - (y + h - r);
                inCircle = (dx * dx + dy * dy <= r * r);
            } else {
                inCircle = true;
            }

            if (inCircle) {
                buffer[py * RENDER_WIDTH + px] = blend(buffer[py * RENDER_WIDTH + px], color);
            }
        }
    }
}

void TouchControls::drawTriangle(uint32_t* buffer, int cx, int cy, int size, int direction, uint32_t color) {
    for (int py = cy - size; py <= cy + size; py++) {
        for (int px = cx - size; px <= cx + size; px++) {
            if (px < 0 || px >= RENDER_WIDTH || py < 0 || py >= RENDER_HEIGHT) continue;

            bool inside = false;
            switch (direction) {
                case 0: // Up
                    inside = (py <= cy && px >= cx - (cy - py) && px <= cx + (cy - py));
                    break;
                case 1: // Down
                    inside = (py >= cy && px >= cx - (py - cy) && px <= cx + (py - cy));
                    break;
                case 2: // Left
                    inside = (px <= cx && py >= cy - (cx - px) && py <= cy + (cx - px));
                    break;
                case 3: // Right
                    inside = (px >= cx && py >= cy - (px - cx) && py <= cy + (px - cx));
                    break;
            }
            if (inside) {
                buffer[py * RENDER_WIDTH + px] = blend(buffer[py * RENDER_WIDTH + px], color);
            }
        }
    }
}

void TouchControls::render(uint32_t* buffer) {
    if (!visible) return;

    for (int i = 0; i < NUM_BUTTONS; i++) {
        TouchButton& btn = buttons[i];
        SDL_Rect& r = btn.rect;
        uint32_t col = btn.pressed ? btn.pressedColor : btn.color;

        if (i < 4) {
            // D-pad arms: rounded rect with triangle arrow
            int armLen = (r.w > r.h) ? r.w : r.h;
            int arrowSize = 2 * scale / 4;
            if (arrowSize < 2) arrowSize = 2;

            drawRoundRect(buffer, r.x, r.y, r.w, r.h, 2, col);

            int acx = r.x + r.w / 2;
            int acy = r.y + r.h / 2;
            drawTriangle(buffer, acx, acy, arrowSize, i, 0x80FFFFFF);
        } else if (i >= 5 && i <= 6) {
            // A/B circles
            drawRoundRect(buffer, r.x, r.y, r.w, r.h, r.w / 2, col);
            // Draw letter via simple pattern
            int lx = r.x + r.w / 2 - 2;
            int ly = r.y + r.h / 2 - 3;
            uint32_t lcol = 0xC0FFFFFF;
            // Simple letters - just a few pixels for A and B
            if (btn.label[0] == 'A') {
                // A: triangle shape at small scale
                for (int py = 0; py < 5; py++) {
                    for (int px = 0; px < 3; px++) {
                        int dx = lx + px;
                        int dy = ly + py;
                        if (dx >= 0 && dx < RENDER_WIDTH && dy >= 0 && dy < RENDER_HEIGHT) {
                            bool on = (px == 0 && py > 1) || (px == 2 && py > 1) || (px == 1 && py == 1) || (px == 1 && py == 3);
                            if (on) buffer[dy * RENDER_WIDTH + dx] = blend(buffer[dy * RENDER_WIDTH + dx], lcol);
                        }
                    }
                }
            } else if (btn.label[0] == 'B') {
                // B: vertical line + bumps
                for (int py = 0; py < 5; py++) {
                    for (int px = 0; px < 3; px++) {
                        int dx = lx + px;
                        int dy = ly + py;
                        if (dx >= 0 && dx < RENDER_WIDTH && dy >= 0 && dy < RENDER_HEIGHT) {
                            bool on = (px == 0) || (py == 0 && px > 0) || (py == 2 && px > 0) || (py == 4 && px > 0) || (px == 2 && (py == 1 || py == 3));
                            if (on) buffer[dy * RENDER_WIDTH + dx] = blend(buffer[dy * RENDER_WIDTH + dx], lcol);
                        }
                    }
                }
            }
        } else if (i >= 7 && i <= 8) {
            // Start/Select pills
            drawRoundRect(buffer, r.x, r.y, r.w, r.h, r.h / 2, col);
        } else if (i == 9) {
            // Settings gear
            drawRoundRect(buffer, r.x, r.y, r.w, r.h, 2, col);
            // Cross inside gear
            uint32_t gcol = 0x80FFFFFF;
            int gcx = r.x + r.w / 2;
            int gcy = r.y + r.h / 2;
            int gs = scale / 2;
            if (gs < 2) gs = 2;
            for (int py = gcy - gs; py <= gcy + gs; py++) {
                for (int px = gcx - gs; px <= gcx + gs; px++) {
                    if (px >= 0 && px < RENDER_WIDTH && py >= 0 && py < RENDER_HEIGHT) {
                        buffer[py * RENDER_WIDTH + px] = blend(buffer[py * RENDER_WIDTH + px], gcol);
                    }
                }
            }
        }
    }
}
