// Input module for v2.6 by the Speed Bump
// This stuff is all GPL.
// In short: there is NO WARRENTY on this, you can't keep it secret, and you
// can't steal credit from me.

#ifndef w_input_h
#define w_input_h

#include <dinput.h>
#include <windows.h>

class Input {
   private:
    // handles
    HINSTANCE hInst;  // app handle
    HWND hWnd;        // window handle

    // DirectInput objects
    LPDIRECTINPUT lpdi;
    LPDIRECTINPUTDEVICE keybd;
    LPDIRECTINPUTDEVICE mouse;
    // LPDIRECTINPUTDEVICE joystick;

    // key buffer
    unsigned char key_buffer[256];   // remember up to 256 events
    unsigned char kb_start, kb_end;  // start/end of keyboard buffer.  Since
                                     // they're bytes, they automaticly wrap
                                     // around.

    RECT mclip;

    // for UnPress
    byte unpress[9];

    int Test(HRESULT result, char *errmsg);

   public:
    unsigned char key[256];  // set to 1 if the key is down, 0 otherwise
    char last_pressed;       // last key event here

    // mouse stuff
    int mousex, mousey;  // mouse coordinates
    int mouseb;

    char up, down, left, right;  // directional controls
    char b1, b2, b3, b4;         // virtual button thingies

    Input();   // constructor
    ~Input();  // destructor

    int Init(HINSTANCE hinst, HWND hwnd);
    void ShutDown();

    void Poll();    // updates key queue and key array
    void Update();  // updates left, down, etc...
    void UnPress(int control);

    // Keyboard stuff
    int GetKey();
    void ClearKeys();
    char Scan2ASCII(int scancode);  // returns the ASCII code. ;)

    void MoveMouse(int x, int y);
    void UpdateMouse();
    void ClipMouse(int x1, int y1, int x2, int y2);
};

#endif
