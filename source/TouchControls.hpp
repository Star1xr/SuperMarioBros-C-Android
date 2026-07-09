#ifndef TOUCHCONTROLS_HPP
#define TOUCHCONTROLS_HPP

#include <cstdint>
#include <SDL.h>
#include "Constants.hpp"
#include "Emulation/Controller.hpp"

struct TouchButton {
    SDL_Rect rect;
    ControllerButton nesButton;
    bool pressed;
    uint32_t color;
    uint32_t pressedColor;
    const char* label;
};

class TouchControls {
public:
    TouchControls();

    void handleEvent(const SDL_Event& event, Controller& controller1);
    void render(uint32_t* buffer);
    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }
    void setScale(int s) { scale = s; }
    void vibrateFeedback();

private:
    bool visible;
    int scale;
    bool lastPressed[8];
    int touchToButton[16];
    static constexpr int NUM_BUTTONS = 10;

    TouchButton buttons[NUM_BUTTONS];
    void recalculateLayout();
    void drawRoundRect(uint32_t* buffer, int x, int y, int w, int h, int r, uint32_t color);
    void drawDpad(uint32_t* buffer, int cx, int cy, int size, uint32_t color);
    void drawTriangle(uint32_t* buffer, int cx, int cy, int size, int direction, uint32_t color);
    uint32_t blend(uint32_t bg, uint32_t fg);
};

#endif
