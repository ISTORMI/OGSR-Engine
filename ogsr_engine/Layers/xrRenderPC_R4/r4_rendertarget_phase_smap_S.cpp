#include "stdafx.h"

void CRenderTarget::phase_smap_spot_clear(CBackend& cmd_list)
{
    ZoneScoped;

    rt_smap_depth->set_slice_write(cmd_list.context_id, cmd_list.context_id);
    cmd_list.set_pass_targets(nullptr, nullptr, nullptr, nullptr, rt_smap_depth);
    cmd_list.ClearZB(rt_smap_depth, 1.0f);
}

void CRenderTarget::phase_smap_spot(CBackend& cmd_list, light* L) const
{
    // TODO: it is possible to increase lights batch size
    // by rendering into different smap array slices in parallel
    rt_smap_depth->set_slice_write(cmd_list.context_id, cmd_list.context_id);                                                             
    cmd_list.set_pass_targets(nullptr, nullptr, nullptr, nullptr, rt_smap_depth);

    const D3D_VIEWPORT viewport = {L->X.S.posX, L->X.S.posY, L->X.S.size, L->X.S.size, 0.f, 1.f};
    cmd_list.SetViewport(viewport);

    // Misc		- draw only front-faces //back-faces
    cmd_list.set_CullMode(CULL_CCW);
    cmd_list.set_Stencil(FALSE);
    // no transparency
#pragma todo("can optimize for multi-lights covering more than say 50%...")
    cmd_list.set_ColorWriteEnable(FALSE);
}

void CRenderTarget::phase_smap_spot_tsh(CBackend& cmd_list, light* L)
{
    VERIFY(!"Implement clear of the buffer for tsh!");

    cmd_list.set_ColorWriteEnable();
    if (IRender_Light::OMNIPART == L->flags.type)
    {
        // omni-part
        cmd_list.ClearRT(cmd_list.get_RT(), {1.0f, 1.0f, 1.0f, 1.0f});
    }
    else
    {
        // real-spot
        // Select color-mask
        ref_shader shader = L->s_spot;
        if (!shader)
            shader = s_accum_spot;
        cmd_list.set_Element(shader->E[SE_L_FILL]);

        // Fill vertex buffer
        Fvector2 p0, p1;
        u32 Offset;
        const u32 C = color_rgba(255, 255, 255, 255);
        const float _w = float(L->X.S.size);
        const float _h = float(L->X.S.size);
        const float d_Z = EPS_S;
        const float d_W = 1.f;
        p0.set(.5f / _w, .5f / _h);
        p1.set((_w + .5f) / _w, (_h + .5f) / _h);

        FVF::TL* pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);

        pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
        pv++;
        pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
        pv++;
        pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
        pv++;
        pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
        pv++;

        RImplementation.Vertex.Unlock(4, g_combine->vb_stride);
        cmd_list.set_Geometry(g_combine);

        // draw
        cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }
}
