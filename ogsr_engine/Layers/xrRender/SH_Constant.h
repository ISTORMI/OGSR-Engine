#pragma once

#include "../../xr_3da/WaveForm.h"

class IReader;
class IWriter;

class ECORE_API CConstant : public xr_resource_named
{
public:
    enum
    {
        modeProgrammable = 0,
        modeWaveForm
    };

public:
    Fcolor const_float{0.0f, 0.0f, 0.0f, 0.0f};
    u32 const_dword{0};

    u32 dwFrame{0};
    u32 dwMode{0};
    WaveForm _R;
    WaveForm _G;
    WaveForm _B;
    WaveForm _A;

    IC void set_float(float r, float g, float b, float a)
    {
        const_float.set(r, g, b, a);
        const_dword = const_float.get();
    }
    IC void set_float(Fcolor& c)
    {
        const_float.set(c);
        const_dword = const_float.get();
    }
    IC void set_dword(u32 c)
    {
        const_float.set(c);
        const_dword = c;
    }
    void Calculate();
    IC BOOL Similar(CConstant& C) const
    // comare by modes and params
    {
        if (dwMode != C.dwMode)
            return FALSE;
        if (!_R.Similar(C._R))
            return FALSE;
        if (!_G.Similar(C._G))
            return FALSE;
        if (!_B.Similar(C._B))
            return FALSE;
        if (!_A.Similar(C._A))
            return FALSE;
        return TRUE;
    }
    void Load(IReader* fs);
    void Save(IWriter* fs) const;
};

typedef resptr_core<CConstant, resptr_base<CConstant>> ref_constant_obsolette;
