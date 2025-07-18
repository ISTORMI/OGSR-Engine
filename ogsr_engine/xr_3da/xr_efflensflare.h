#pragma once


#include "../xrcdb/xr_collide_defs.h"

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/LensFlareRender.h"

class ENGINE_API CInifile;
class ENGINE_API CEnvironment;

class ENGINE_API CLensFlareDescriptor
{
public:
    struct SFlare
    {
        float fOpacity;
        float fRadius;
        float fPosition;
        shared_str texture;
        shared_str shader;
        FactoryPtr<IFlareRender> m_pRender;

        SFlare() { fOpacity = fRadius = fPosition = 0; }
    };
    struct SSource : public SFlare
    {
        BOOL ignore_color;
    };
    DEFINE_VECTOR(SFlare, FlareVec, FlareIt);
    FlareVec m_Flares;

    enum
    {
        flFlare = (1 << 0),
        flSource = (1 << 1),
        flGradient = (1 << 2)
    };
    Flags32 m_Flags;

    // source
    SSource m_Source;

    // gradient
    SFlare m_Gradient;

    float m_StateBlendUpSpeed;
    float m_StateBlendDnSpeed;

    shared_str section;

private:
    void SetGradient(float fMaxRadius, float fOpacity, LPCSTR tex_name, LPCSTR sh_name);
    void SetSource(float fRadius, BOOL ign_color, LPCSTR tex_name, LPCSTR sh_name);
    void AddFlare(float fRadius, float fOpacity, float fPosition, LPCSTR tex_name, LPCSTR sh_name);

public:
    CLensFlareDescriptor()
    {
        m_Flags.zero();
        section = nullptr;
        m_StateBlendUpSpeed = m_StateBlendDnSpeed = 0.1f;
    }
    void load(CInifile* pIni, LPCSTR section);
    void OnDeviceCreate();
    void OnDeviceDestroy();
};
DEFINE_VECTOR(CLensFlareDescriptor*, LensFlareDescVec, LensFlareDescIt);

class ENGINE_API CLensFlare
{
    friend class dxLensFlareRender;

public:
    enum
    {
        MAX_RAYS = 5
    };

private:
    collide::rq_results r_dest;
    collide::ray_cache m_ray_cache[MAX_RAYS];

protected:
    float fBlend;
    u32 dwFrame;

    Fvector vSunDir;
    Fvector vecLight;
    Fvector vecX, vecY, vecDir, vecAxis, vecCenter;
    BOOL bRender;

    // variable
    Fcolor LightColor;
    float fGradientValue;

    FactoryPtr<ILensFlareRender> m_pRender;

    LensFlareDescVec m_Palette;
    CLensFlareDescriptor* m_Current;

    //. #ifdef DEBUG
public:
    enum LFState
    {
        lfsNone,
        lfsIdle,
        lfsHide,
        lfsShow,
    };
    //. #endif // DEBUG

protected:
    LFState m_State;
    float m_StateBlend;

public:
    CLensFlare();
    virtual ~CLensFlare();

    void OnFrame(shared_str id);
    void Render(CBackend& cmd_list, BOOL bSun, BOOL bFlares, BOOL bGradient);

    void OnDeviceCreate();
    void OnDeviceDestroy();

    shared_str AppendDef(CEnvironment& environment, LPCSTR sect);

    void Invalidate() { m_State = lfsNone; }
};

IC void blend_lerp(float& cur, float tgt, float speed, float dt)
{
    float diff = tgt - cur;
    float diff_a = _abs(diff);
    if (diff_a < EPS_S)
        return;
    float mot = speed * dt;
    if (mot > diff_a)
        mot = diff_a;
    cur += (diff / diff_a) * mot;
}
