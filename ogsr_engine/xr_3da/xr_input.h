#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#define MOUSE_1 (0xED + 100)
#define MOUSE_8 (0xED + 107)

class ENGINE_API IInputReceiver;

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

const int mouse_device_key = 1;
const int keyboard_device_key = 2;
const int all_device_key = mouse_device_key | keyboard_device_key;
const int default_key = mouse_device_key | keyboard_device_key;

class ENGINE_API CInput : public pureFrame, public pureAppActivate, public pureAppDeactivate
{
public:
    enum
    {
        COUNT_MOUSE_BUTTONS = 8,
        COUNT_MOUSE_AXIS = 3,
        COUNT_KB_BUTTONS = 256
    };
    struct sxr_mouse
    {
        DIDEVCAPS capabilities;
        DIDEVICEINSTANCE deviceInfo;
        DIDEVICEOBJECTINSTANCE objectInfo;
        u32 mouse_dt;
    };
    struct sxr_key
    {
        DIDEVCAPS capabilities;
        DIDEVICEINSTANCE deviceInfo;
        DIDEVICEOBJECTINSTANCE objectInfo;
    };

private:
    LPDIRECTINPUT8 pDI{}; // The DInput object
    LPDIRECTINPUTDEVICE8 pMouse{}; // The DIDevice7 interface
    LPDIRECTINPUTDEVICE8 pKeyboard{}; // The DIDevice7 interface

    u32 timeStamp[COUNT_MOUSE_AXIS]{};
    u32 timeSave[COUNT_MOUSE_AXIS]{};
    int offs[COUNT_MOUSE_AXIS]{};
    BOOL mouseState[COUNT_MOUSE_BUTTONS]{};
    u32 mouseTime[COUNT_MOUSE_BUTTONS]{};
    uint8_t KBState[COUNT_KB_BUTTONS]{};

    HRESULT CreateInputDevice(LPDIRECTINPUTDEVICE8* device, GUID guidDevice, const DIDATAFORMAT* pdidDataFormat, u32 dwFlags, u32 buf_size) const;

    xr_vector<IInputReceiver*> cbStack;

    void MouseUpdate();
    void KeyUpdate();
    bool is_exclusive_mode;

    BOOL iGetAsyncBtnState(int btn, bool hold = false) const;

    sxr_mouse mouse_property;
    sxr_key key_property;
    u32 dwCurTime;

    void SetAllAcquire(BOOL bAcquire = TRUE) const;

public:

    CInput(bool bExclusive = true, int deviceForInit = default_key);
    ~CInput();

    void iCapture(IInputReceiver* pc);
    void iRelease(IInputReceiver* pc);
    BOOL iGetAsyncKeyState(int dik, bool hold = false) const;

    virtual void OnFrame(void);
    virtual void OnAppActivate(void);
    virtual void OnAppDeactivate(void);

    IInputReceiver* CurrentIR() const;

    void exclusive_mode(const bool exclusive);
    bool exclusive_mode() const { return is_exclusive_mode; }

public:
    // Возвращает символ по коду клавиши. Учитывается переключение языка, зажатый shift и caps lock
    // ( caps lock учитывается только в неэксклюзивном режиме, из-за его особенностей )
    // В случае неудачи функция возвращает 0.
    u16 DikToChar(const int dik, const bool utf) const;
    
    bool get_dik_name(int dik, LPSTR dest, int dest_sz) const;
    static void clip_cursor(bool clip);
};

extern ENGINE_API CInput* pInput;
