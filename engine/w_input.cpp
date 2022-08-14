// Keyboard handling stuff.

// This was originally meant to mimic aen's keyboard handler, but has since
// been reversed. While aen's handler "feeds" keys to processes, this one
// simply puts them in a queue for the process to grab on it's own.

// ChangeLog
// + <tSB> 11.05.00 - Initial writing
// + <tSB> 11.07.00 - DirectInput won't compile right.  Re-started, using
// Win32's messaging system to handle keypresses.
// + <tSB> 11.07.00 - This doesn't work either :P Different DirectX libs work
// now. ^_^
// + <tSB> 11.09.00 - woo, another overhaul.  Based on vecna's Winv1/Blackstar
// code.
// + <tSB> 11.16.00 - Mouse code added
// + <tSB> 12.05.00 - Mouse code rehashed, using DInput again.

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <set>
#include <emscripten/html5.h>

#include "w_input.h"

#include "keyboard.h"
#include "verge.h" // for log :P

static const int kup = DIK_UP;
static const int kdown = DIK_DOWN;
static const int kleft = DIK_LEFT;
static const int kright = DIK_RIGHT;
static const int kb1 = DIK_ENTER;
static const int kb2 = DIK_LALT;
static const int kb3 = DIK_ESCAPE;
static const int kb4 = DIK_SPACE;

static byte key_ascii_tbl[128] = {0, 0, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', 8, 9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', 13, 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`',
    0, 92, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
    3, 3, 3, 3, 8, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0,
    127, 0, 0, 92, 3, 3, 0, 0, 0, 0, 0, 0, 0, 13, 0, '/', 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '/', 0, 0, 0, 0, 0};

static byte key_shift_tbl[128] = {0, 0, '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', 126, 126, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O',
    'P', '{', '}', 126, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34,
    '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0,
    1, 127, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 13, 0, '/', 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '/', 0, 0, 0, 0, 0};

static const std::unordered_map<int, int> scanMap = {
    { VK_RIGHT_ALT, DIK_RMENU },
    { VK_LEFT_ALT, DIK_LMENU },
    { VK_LEFT_CONTROL, DIK_LCONTROL },
    { VK_RIGHT_CONTROL, DIK_RCONTROL },
    { VK_LEFT_SHIFT, DIK_LSHIFT },
    { VK_RIGHT_SHIFT, DIK_RSHIFT },
    // { VK_META, DIK_META },
    { VK_BACK_SPACE, DIK_BACK },
    { VK_CAPS_LOCK, DIK_CAPSLOCK },
    // { VK_DELETE, DIK_DELETE },
    { VK_END, DIK_END },
    { VK_ENTER, DIK_ENTER },
    { VK_ESCAPE, DIK_ESCAPE },
    { VK_HOME, DIK_HOME },
    // { VK_NUM_LOCK, DIK_NUM_LOCK },
    // { VK_PAUSE, DIK_PAUSE },
    // { VK_PRINTSCREEN, DIK_PRINTSCREEN },
    // { VK_SCROLL_LOCK, DIK_SCROLL_LOCK },
    { VK_SPACE, DIK_SPACE },
    { VK_TAB, DIK_TAB },
    { VK_LEFT, DIK_LEFT },
    { VK_RIGHT, DIK_RIGHT },
    { VK_UP, DIK_UP },
    { VK_DOWN, DIK_DOWN },
    { VK_PAGE_DOWN, DIK_PGDN },
    { VK_PAGE_UP, DIK_PGUP },
    { VK_F1, DIK_F1 },
    { VK_F2, DIK_F2 },
    { VK_F3, DIK_F3 },
    { VK_F4, DIK_F4 },
    { VK_F5, DIK_F5 },
    { VK_F6, DIK_F6 },
    { VK_F7, DIK_F7 },
    { VK_F8, DIK_F8 },
    { VK_F9, DIK_F9 },
    { VK_F10, DIK_F10 },
    { VK_F11, DIK_F11 },
    { VK_F12, DIK_F12 },
    // { VK_F13, DIK_F13 },
    // { VK_F14, DIK_F14 },
    // { VK_F15, DIK_F15 },
    // { VK_F16, DIK_F16 },
    // { VK_F17, DIK_F17 },
    // { VK_F18, DIK_F18 },
    // { VK_F19, DIK_F19 },
    // { VK_F20, DIK_F20 },
    // { VK_F21, DIK_F21 },
    // { VK_F22, DIK_F22 },
    // { VK_F23, DIK_F23 },
    // { VK_F24, DIK_F24 },
    { VK_TILDE, DIK_GRAVE },

    { VK_0, DIK_0 },
    { VK_1, DIK_1 },
    { VK_2, DIK_2 },
    { VK_3, DIK_3 },
    { VK_4, DIK_4 },
    { VK_5, DIK_5 },
    { VK_6, DIK_6 },
    { VK_7, DIK_7 },
    { VK_8, DIK_8 },
    { VK_9, DIK_9 },
    
    { VK_A, DIK_A },
    { VK_B, DIK_B },
    { VK_C, DIK_C },
    { VK_D, DIK_D },
    { VK_E, DIK_E },
    { VK_F, DIK_F },
    { VK_G, DIK_G },
    { VK_H, DIK_H },
    { VK_I, DIK_I },
    { VK_J, DIK_J },
    { VK_K, DIK_K },
    { VK_L, DIK_L },
    { VK_M, DIK_M },
    { VK_N, DIK_N },
    { VK_O, DIK_O },
    { VK_P, DIK_P },
    { VK_Q, DIK_Q },
    { VK_R, DIK_R },
    { VK_S, DIK_S },
    { VK_T, DIK_T },
    { VK_U, DIK_U },
    { VK_V, DIK_V },
    { VK_W, DIK_W },
    { VK_X, DIK_X },
    { VK_Y, DIK_Y },
    { VK_Z, DIK_Z },
    // ,./ keys.
    { VK_OEM_COMMA, DIK_COMMA },
    { VK_OEM_PERIOD, DIK_PERIOD },
    { VK_OEM_2, DIK_SLASH },
    // ;' keys.
    { VK_OEM_1, DIK_SEMICOLON },
    { VK_OEM_7, DIK_APOSTROPHE },
    // []\ keys.
    { VK_OEM_4, DIK_LBRACKET },
    { VK_OEM_6, DIK_RBRACKET },
    { VK_OEM_5, DIK_BACKSLASH },
    // -= keys.
    { VK_OEM_MINUS, DIK_MINUS },
    { VK_OEM_PLUS, DIK_EQUALS }
};


#ifdef DEBUG_INPUT
#define INPUT_PRINTF printf
#else
template <typename... T> void INPUT_PRINTF(T...) {}
#endif

namespace {
enum class EventType {
    None,
    KeyUp,
    KeyDown,
};

struct InputEvent {
    EventType type;
    int keyCode;
};

std::vector<InputEvent> inputEvents;
std::set<int> connectedGamepads;

const double GAMEPAD_ANALOG_THRESHHOLD = 0.8;

bool shouldStopPropagation(int keyCode) {
    return DIK_TAB == keyCode;
}

EM_BOOL onKeyDown(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    int code = e->keyCode;
    auto it = scanMap.find(code);
    if (it != scanMap.end()) {
        INPUT_PRINTF("onKeyDown: scanMap %d -> %d\n", code, it->second);
        code = it->second;
    }

    INPUT_PRINTF("Key down %d %s\n", code, e->key);
    inputEvents.push_back(InputEvent{EventType::KeyDown, code});
    return shouldStopPropagation(code);
}

EM_BOOL onKeyUp(
    int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
    int code = e->keyCode;
    auto it = scanMap.find(code);
    if (it != scanMap.end()) {
        INPUT_PRINTF("onKeyUp: scanMap %d -> %d\n", code, it->second);
        code = it->second;
    }

    INPUT_PRINTF("Key up %d %s\n", code, e->key);
    inputEvents.push_back(InputEvent{EventType::KeyUp, code});
    return shouldStopPropagation(code);
}

EM_BOOL onGamepadConnected(
    int eventType, const EmscriptenGamepadEvent* gamepadEvent, void* userData) {
    INPUT_PRINTF("Gamepad connected idx='%s' mapping='%s' index=%ld\n",
        gamepadEvent->id, gamepadEvent->mapping, gamepadEvent->index);
    connectedGamepads.insert(gamepadEvent->index);
    return true;
}

EM_BOOL onGamepadDisonnected(
    int eventType, const EmscriptenGamepadEvent* gamepadEvent, void* userData) {
    INPUT_PRINTF("Gamepad disconnected\n");
    connectedGamepads.erase(gamepadEvent->index);
    return true;
}

EM_BOOL onMouseDown(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData) {
    // INPUT_PRINTF("onMouseDown %ld %ld %d\n", mouseEvent->clientX, mouseEvent->clientY, mouseEvent->button);
    Input* input = reinterpret_cast<Input*>(userData);

    input->mousex = mouseEvent->clientX;
    input->mousey = mouseEvent->clientY;
    switch (mouseEvent->button) {
        case 0: input->mouseb |= 1; break;
        case 1: input->mouseb |= 2; break;
        case 2: input->mouseb |= 4; break;
    }

    return true;
}

EM_BOOL onMouseUp(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData) {
    // printf("onMouseUp %ld %ld %d\n", mouseEvent->clientX, mouseEvent->clientY, mouseEvent->button);
    Input* input = reinterpret_cast<Input*>(userData);

    input->mousex = mouseEvent->clientX;
    input->mousey = mouseEvent->clientY;
    switch (mouseEvent->button) {
    case 0: input->mouseb &= ~1; break;
    case 1: input->mouseb &= ~2; break;
    case 2: input->mouseb &= ~4; break;
    }

    return true;
}

EM_BOOL onMouseMove(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData) {
    // printf("onMouseMove %ld %ld %d\n", mouseEvent->clientX, mouseEvent->clientY, mouseEvent->button);
    Input* input = reinterpret_cast<Input*>(userData);

    input->mousex = mouseEvent->clientX;
    input->mousey = mouseEvent->clientY;

    return true;
}

}

Input::Input() {}

Input::~Input() {}

int Input::Init() {
    kb_start = kb_end = 0;

    // mclip.top = mclip.left = 0;
    // mclip.right = 320;
    // mclip.bottom = 200;

    EMSCRIPTEN_RESULT result;
    result = emscripten_set_keydown_callback("body", nullptr, true, &onKeyDown);
    // TEST_RESULT(result);

    result = emscripten_set_keyup_callback("body", nullptr, true, &onKeyUp);
    // TEST_RESULT(result);

    auto res = emscripten_sample_gamepad_data();
    if (res == EMSCRIPTEN_RESULT_NOT_SUPPORTED) {
        printf("no gamepad support?\n");
    }
    // EMSCRIPTEN_RESULT_NOT_SUPPORTED means that the browser does not support gamepads at all

    emscripten_set_gamepadconnected_callback(0, true, &onGamepadConnected);
    emscripten_set_gamepaddisconnected_callback(0, true, &onGamepadDisonnected);

    emscripten_set_mousedown_callback("#vergeCanvas", this, true, &onMouseDown);
    emscripten_set_mouseup_callback("#vergeCanvas", this, true, &onMouseUp);
    emscripten_set_mousemove_callback("#vergeCanvas", this, true, &onMouseMove);

    return 1;
}

void Input::ShutDown() {}

void Input::Poll() // updates the key[] array.  This is called in winproc in
                   // response to WM_KEYDOWN and WM_KEYUP
{
    unsigned int i, k; // loop counter, key index
    bool kdown;        // is the key down?
    auto events = std::move(inputEvents);
    for (const InputEvent& evt : events) {
        k = evt.keyCode;
        kdown = evt.type == EventType::KeyDown;

        // First off, DX has separate codes for the control keys, and alt keys,
        // etc...
        // We don't want that.  Convert 'em.
        if (k == DIK_RCONTROL)
            k = DIK_LCONTROL;
        if (k == DIK_RMENU)
            k = DIK_LMENU;

        if (unpress[k]) {
            if (kdown)
                return;
            else
                unpress[k] = 0;
        }

        key[k] = kdown ? 1 : 0; // 1 if it's down, 0 if it isn't. (high bit of
                                // dwData is set if the key is down)
        last_pressed = k;       // this is kinda handy

        if (kdown && kb_end != kb_start + 1) // only if the buffer isn't full
            key_buffer[kb_end++] =
                k; // add it to the queue, if the key was pushed
                   // TODO: key repeating?
    }

    // -------------mouse-------------
}

void Input::Update() // updates the direction variables and the virtual buttons
                     // (b1, b2, etc..)
{
    Poll();

    up = key[kup];
    down = key[kdown];
    left = key[kleft];
    right = key[kright];

    b1 = key[kb1];                 // TODO: make these customizable
    b2 = key[kb2] | key[DIK_RALT]; // hack
    b3 = key[kb3];
    b4 = key[kb4];

    emscripten_sample_gamepad_data();
    // int count = emscripten_get_num_gamepads();

    EmscriptenGamepadEvent state;

    for (int i : connectedGamepads) {
        emscripten_get_gamepad_status(i, &state);
        b1 |= state.digitalButton[0];
        b2 |= state.digitalButton[1];
        b3 |= state.digitalButton[9];
        b4 |= state.digitalButton[3];

        up |= state.digitalButton[12];
        down |= state.digitalButton[13];
        left |= state.digitalButton[14];
        right |= state.digitalButton[15];

        if (state.axis[0] < -GAMEPAD_ANALOG_THRESHHOLD) {
            left = 1;
        }
        if (state.axis[0] > GAMEPAD_ANALOG_THRESHHOLD) {
            right = 1;
        }
        if (state.axis[1] < -GAMEPAD_ANALOG_THRESHHOLD) {
            up = 1;
        }
        if (state.axis[1] > GAMEPAD_ANALOG_THRESHHOLD) {
            down = 1;
        }
    }

    if (unpress[1]) {
        if (b1)
            b1 = 0;
        else
            unpress[1] = 0;
    }
    if (unpress[2]) {
        if (b2)
            b2 = 0;
        else
            unpress[2] = 0;
    }
    if (unpress[3]) {
        if (b3)
            b3 = 0;
        else
            unpress[3] = 0;
    }
    if (unpress[4]) {
        if (b4)
            b4 = 0;
        else
            unpress[4] = 0;
    }

    if (unpress[5]) {
        if (up)
            up = 0;
        else
            unpress[5] = 0;
    }
    if (unpress[6]) {
        if (down)
            down = 0;
        else
            unpress[6] = 0;
    }
    if (unpress[7]) {
        if (left)
            left = 0;
        else
            unpress[7] = 0;
    }
    if (unpress[8]) {
        if (right)
            right = 0;
        else
            unpress[8] = 0;
    }

    UpdateMouse();
}

int Input::GetKey()
// gets the next key from the buffer, or 0 if there isn't one
{
    if (kb_start == kb_end)
        return 0; // nope!  nuthin here

    return key_buffer[kb_start++];
}

void Input::ClearKeys()
// clears the keyboard buffer (duh!)
{
    kb_end = kb_start = 0;
}

void Input::UnPress(int control) {
    auto u = [&](int c, char& k, int keycode) {
        if (k) {
            unpress[c] = 1;
            k = 0;
            key[keycode] = 0;
        }
    };

    switch (control) {
    // GROSS!
    case 0:
        for (int i = 1; i <= 8; ++i)
            UnPress(i);
        break;
    case 1:
        u(control, b1, kb1);
        break;
    case 2:
        u(control, b2, kb2);
        break;
    case 3:
        u(control, b3, kb3);
        break;
    case 4:
        u(control, b4, kb4);
        break;

    case 5:
        u(control, up, kup);
        break;
    case 6:
        u(control, down, kdown);
        break;
    case 7:
        u(control, left, kleft);
        break;
    case 8:
        u(control, right, kright);
        break;

        // default:
        //     u[control] = 1;
        //     break;
    }
}

char Input::Scan2ASCII(int scancode) {
    if (key[DIK_LSHIFT] || key[DIK_RSHIFT])
        return key_shift_tbl[scancode];
    else
        return key_ascii_tbl[scancode];
}

void Input::MoveMouse(int x, int y) {
    mousex = x;
    mousey = y;
}

void Input::UpdateMouse() {
    // mousex += dims.lX;
    // mousey += dims.lY;

    if (mousex < mclip.left)
        mousex = mclip.left;
    if (mousex > mclip.right)
        mousex = mclip.right;
    if (mousey < mclip.top)
        mousey = mclip.top;
    if (mousey > mclip.bottom)
        mousey = mclip.bottom;

    mouseb = 0;
    // if (dims.rgbButtons[0] & 0x80)
    //     mouseb |= 1;
    // if (dims.rgbButtons[1] & 0x80)
    //     mouseb |= 2;
    // if (dims.rgbButtons[2] & 0x80)
    //     mouseb |= 4;
    // if (dims.rgbButtons[3] & 0x80)
    //     mouseb |= 8;
}

void Input::ClipMouse(int x1, int y1, int x2, int y2) {
    mclip.left = x1;
    mclip.right = x2;
    mclip.top = y1;
    mclip.bottom = y2;
}
