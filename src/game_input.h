#pragma once

#include "types.h"

namespace xjar {

struct ButtonState {
    b32 pressed;
    u32 transitions;
};

constexpr int BUTTON_COUNT = 12;

enum GameMouseInput {
    GameMouseInput_Left = 0,
    GameMouseInput_Middle,
    GameMouseInput_Right,
    GameMouseInput_Extended1,
    GameMouseInput_Extended2,
    GameMouseInput_Count
};

struct GameInput {
    union {
        ButtonState buttons[BUTTON_COUNT];
        struct {
            ButtonState actionUp;
            ButtonState actionDown;
            ButtonState actionLeft;
            ButtonState actionRight;
            ButtonState actionAccelerate;
            ButtonState buttonH;
            ButtonState buttonJ;
            ButtonState buttonK;
            ButtonState buttonL;
            ButtonState button1;
            ButtonState button2;
            ButtonState button3;
        };
    };

    ButtonState mouseButtons[GameMouseInput_Count];
    f32         mouseX;
    f32         mouseY;
};


}
