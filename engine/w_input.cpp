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

#define DIRECTINPUT_VERSION 0X0500
#include "w_input.h"  // woo!  no dependencies! :D
#include <dinput.h>

#include "verge.h"  // for log :P

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

Input::Input() {
    lpdi = NULL;
    keybd = NULL;
    mouse = NULL;
}

Input::~Input() {
    if (lpdi != NULL) ShutDown();
}

inline int Input::Test(HRESULT result, char *errmsg) {
    if (result != DI_OK) {
        Log(errmsg);
        return 0;
    }
    return 1;
}

int Input::Init(HINSTANCE hinst, HWND hwnd) {
    HRESULT result;
    DIPROPDWORD dipdw;

    hInst = hinst;
    hWnd = hwnd;

    result = DirectInputCreate(hinst, DIRECTINPUT_VERSION, &lpdi, NULL);
    if (!Test(result, "DI:DInputCreate")) return 0;
    /* if (result!=DI_OK)
      {
       Log("DI:DInputcreate");
       return 0;
      }*/

    // -------------keyboard initizlization------------
    result = lpdi->CreateDevice(GUID_SysKeyboard, &keybd, NULL);
    if (result != DI_OK) {
        Log("DI:CreateDevice");
        ShutDown();
        return 0;
    }

    result = keybd->SetDataFormat(&c_dfDIKeyboard);
    if (result != DI_OK) {
        Log("DI:SetDataFormat");
        ShutDown();
        return 0;
    }

    result =
        keybd->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (result != DI_OK) {
        Log("DI:SetCo-opLevel");
        ShutDown();
        return 0;
    }

    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = 128;
    result = keybd->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
    if (result != DI_OK) {
        Log("DI:SetProperty");
        ShutDown();
        return 0;
    }

    keybd->Acquire();
    kb_start = kb_end = 0;

    // ---------------mouse-----------------
    result = lpdi->CreateDevice(GUID_SysMouse, &mouse, NULL);
    if (!Test(result, "DI:CreateMouseDevice")) return 0;

    result = mouse->SetDataFormat(&c_dfDIMouse);
    if (!Test(result, "DI:SetMouseDataFormat")) return 0;

    result =
        mouse->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (!Test(result, "DI:SetMouseCoOpLevel")) return 0;

    mclip.top = mclip.left = 0;
    mclip.right = 320;
    mclip.bottom = 200;

    return 1;
}

void Input::ShutDown() {
    if (lpdi == NULL) return;
    if (keybd != NULL) {
        keybd->Unacquire();
        keybd->Release();
        keybd = NULL;
    }
    if (mouse != NULL) {
        mouse->Unacquire();
        mouse->Release();
        mouse = NULL;
    }
    // etc... for mouse/joystick/etc...
    lpdi->Release();
    lpdi = NULL;
}

void Input::Poll()  // updates the key[] array.  This is called in winproc in
                    // response to WM_KEYDOWN and WM_KEYUP
{
    HRESULT result;
    DIDEVICEOBJECTDATA didata[128];
    DWORD numentries;

    numentries = 128;
    // read from the keyboard (buffered mode this time!
    result = keybd->GetDeviceData(
        sizeof(DIDEVICEOBJECTDATA), didata, &numentries, 0);
    if (result != DI_OK && result != DI_BUFFEROVERFLOW)  // HEY! D:<
    {
        keybd->Acquire();  // re-acquire it
        result = keybd->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), didata,
            &numentries, 0);  // and try again!
    }

    if (!numentries || result == DIERR_OTHERAPPHASPRIO)
        return;  // TODO: joystick?

    unsigned int i, k;  // loop counter, key index
    bool kdown;         // is the key down?
    for (i = 0; i < numentries; i++) {
        k = didata[i].dwOfs;
        kdown = didata[i].dwData & 0x80 ? true : false;

        // First off, DX has separate codes for the control keys, and alt keys,
        // etc...
        // We don't want that.  Convert 'em.
        if (k == DIK_RCONTROL) k = DIK_LCONTROL;
        if (k == DIK_RMENU) k = DIK_LMENU;

        if (unpress[k]) {
            if (kdown)
                return;
            else
                unpress[k] = 0;
        }

        key[k] = kdown ? 1 : 0;  // 1 if it's down, 0 if it isn't. (high bit of
                                 // dwData is set if the key is down)
        last_pressed = k;        // this is kinda handy

        if (kdown && kb_end != kb_start + 1)  // only if the buffer isn't full
            key_buffer[kb_end++] =
                k;  // add it to the queue, if the key was pushed
                    // TODO: key repeating?
    }

    if (key[DIK_F11]) {
        Message_Send("Screenshot taken", 100);
        ScreenShot();
    }

    // -------------mouse-------------
}

void Input::Update()  // updates the direction variables and the virtual buttons
                      // (b1, b2, etc..)
{
    Poll();

    up = key[DIK_UP];
    down = key[DIK_DOWN];
    left = key[DIK_LEFT];
    right = key[DIK_RIGHT];

    b1 = key[DIK_ENTER];  // TODO: make these customizable
    b2 = key[DIK_LALT] | key[DIK_RALT];
    b3 = key[DIK_ESCAPE];
    b4 = key[DIK_SPACE];

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
    if (kb_start == kb_end) return 0;  // nope!  nuthin here

    return key_buffer[kb_start++];
}

void Input::ClearKeys()
// clears the keyboard buffer (duh!)
{
    kb_end = kb_start = 0;
}

void Input::UnPress(int control) {
    switch (control) {
    // GROSS!
    case 0:
        if (b1) unpress[1] = 1;
        if (b2) unpress[2] = 1;
        if (b3) unpress[3] = 1;
        if (b4) unpress[4] = 1;

        if (up) unpress[5] = 1;
        if (down) unpress[6] = 1;
        if (left) unpress[7] = 1;
        if (right) unpress[8] = 1;
        break;
    case 1:
        if (b1) {
            unpress[1] = 1;
            b1 = 0;
        }
        break;
    case 2:
        if (b2) {
            unpress[2] = 1;
            b2 = 0;
        }
        break;
    case 3:
        if (b3) {
            unpress[3] = 1;
            b3 = 0;
        }
        break;
    case 4:
        if (b4) {
            unpress[4] = 1;
            b4 = 0;
        }
        break;

    case 5:
        if (up) unpress[5] = 1;
        break;
    case 6:
        if (down) unpress[6] = 1;
        break;
    case 7:
        if (left) unpress[7] = 1;
        break;
    case 8:
        if (right) unpress[8] = 1;
        break;
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
    DIMOUSESTATE dims;
    HRESULT result;

    mouse->Acquire();
    result = mouse->GetDeviceState(sizeof(dims), &dims);
    if (!Test(result, "DirectInput Error: Error reading the mouse")) return;

    mousex += dims.lX;
    mousey += dims.lY;

    if (mousex < mclip.left) mousex = mclip.left;
    if (mousex > mclip.right) mousex = mclip.right;
    if (mousey < mclip.top) mousey = mclip.top;
    if (mousey > mclip.bottom) mousey = mclip.bottom;

    mouseb = 0;
    if (dims.rgbButtons[0] & 0x80) mouseb |= 1;
    if (dims.rgbButtons[1] & 0x80) mouseb |= 2;
    if (dims.rgbButtons[2] & 0x80) mouseb |= 4;
    if (dims.rgbButtons[3] & 0x80) mouseb |= 8;
}

void Input::ClipMouse(int x1, int y1, int x2, int y2) {
    mclip.left = x1;
    mclip.right = x2;
    mclip.top = y1;
    mclip.bottom = y2;
}
