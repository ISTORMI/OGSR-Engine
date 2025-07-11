// occRasterizer.cpp: implementation of the occRasterizer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "occRasterizer.h"

#if DEBUG
#include "dxRenderDeviceRender.h"
#include "xrRender_console.h"
#endif

occRasterizer Raster;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

occRasterizer::occRasterizer()
    : bufFrame{}, bufDepth{}, bufDepth_0{}
#if DEBUG
      ,
      dbg_HOM_draw_initialized(false)
#endif
{}

template<typename T>
static inline void MemFill(void* dst, const T value, const size_t dstSize)
{
    T* ptr = reinterpret_cast<T*>(dst);
    T* end = ptr + dstSize;
    while (ptr != end)
        *ptr++ = value;
}

void occRasterizer::clear()
{
    MemFill<ptrdiff_t>(bufFrame, 0, sizeof bufFrame / sizeof(occTri*));
    MemFill<float>(bufDepth, 1.f, sizeof bufDepth / sizeof(float));
}

static inline bool shared(occTri* T1, occTri* T2)
{
    if (T1 == T2)
        return true;
    if (T1->adjacent[0] == T2)
        return true;
    if (T1->adjacent[1] == T2)
        return true;
    if (T1->adjacent[2] == T2)
        return true;
    return false;
}

void occRasterizer::propagade()
{
    ZoneScoped;

    // Clip-and-propagade zero level
    occTri** pFrame = get_frame();
    float* pDepth = get_depth();
    for (int y = 0; y < occ_dim_0; y++)
    {
        for (int x = 0; x < occ_dim_0; x++)
        {
            int ox = x + 2, oy = y + 2;

            // Y2-connect
            const int pos = oy * occ_dim + ox;
            const int pos_up = pos - occ_dim;
            const int pos_down = pos + occ_dim;
            const int pos_down2 = pos_down + occ_dim;

            occTri* Tu1 = pFrame[pos_up];
            if (Tu1)
            {
                // We has pixel 1scan up
                if (shared(Tu1, pFrame[pos_down]))
                {
                    // We has pixel 1scan down
                    const float ZR = (pDepth[pos_up] + pDepth[pos_down]) / 2;
                    if (ZR < pDepth[pos])
                    {
                        pFrame[pos] = Tu1;
                        pDepth[pos] = ZR;
                    }
                }
                else if (shared(Tu1, pFrame[pos_down2]))
                {
                    // We has pixel 2scan down
                    const float ZR = (pDepth[pos_up] + pDepth[pos_down2]) / 2;
                    if (ZR < pDepth[pos])
                    {
                        pFrame[pos] = Tu1;
                        pDepth[pos] = ZR;
                    }
                }
            }

            //
            float d = pDepth[pos];
            clamp(d, -1.99f, 1.99f);
            bufDepth_0[y][x] = df_2_s32(d);
        }
    }
}

void occRasterizer::on_dbg_render()
{
#if DEBUG
    if (!ps_r2_ls_flags_ext.is(R2FLAGEXT_HOM_DEPTH_DRAW))
    {
        dbg_HOM_draw_initialized = false;
        return;
    }

    for (int i = 0; i < occ_dim_0; ++i)
    {
        for (int j = 0; j < occ_dim_0; ++j)
        {
            if (bDebug)
            {
                Fvector quad, left_top, right_bottom, box_center, box_r;
                quad.set((float)j - occ_dim_0 / 2.f, -((float)i - occ_dim_0 / 2.f), (float)bufDepth_0[i][j] / occQ_s32);
                Device.mProject;

                float z = -Device.mProject._43 / (float)(Device.mProject._33 - quad.z);
                left_top.set(quad.x * z / Device.mProject._11 / (occ_dim_0 / 2.f), quad.y * z / Device.mProject._22 / (occ_dim_0 / 2.f), z);
                right_bottom.set((quad.x + 1) * z / Device.mProject._11 / (occ_dim_0 / 2.f), (quad.y + 1) * z / Device.mProject._22 / (occ_dim_0 / 2.f), z);

                box_center.set((right_bottom.x + left_top.x) / 2, (right_bottom.y + left_top.y) / 2, z);
                box_r = right_bottom;
                box_r.sub(box_center);

                Device.mInvView.transform(box_center);
                Device.mInvView.transform_dir(box_r);

                pixel_box& tmp = dbg_pixel_boxes[i * occ_dim_0 + j];
                tmp.center = box_center;
                tmp.radius = box_r;
                tmp.z = quad.z;
                dbg_HOM_draw_initialized = true;
            }

            if (!dbg_HOM_draw_initialized)
                return;

            pixel_box& tmp = dbg_pixel_boxes[i * occ_dim_0 + j];
            Fmatrix Transform;
            Transform.identity();
            Transform.translate(tmp.center);

            // draw wire
            Device.SetNearer(TRUE);

            cmd_list.set_Shader(dxRenderDeviceRender::Instance().m_SelectionShader);
            cmd_list.dbg_DrawOBB(Transform, tmp.radius, D3DCOLOR_XRGB(u32(255 * pow(tmp.z, 20.f)), u32(255 * (1 - pow(tmp.z, 20.f))), 0));
            Device.SetNearer(FALSE);
        }
    }
#endif
}

IC BOOL test_Level(occD* depth, int dim, float _x0, float _y0, float _x1, float _y1, occD z)
{
    int x0 = iFloor(_x0 * dim + .5f);
    clamp(x0, 0, dim - 1);
    int x1 = iFloor(_x1 * dim + .5f);
    clamp(x1, x0, dim - 1);
    int y0 = iFloor(_y0 * dim + .5f);
    clamp(y0, 0, dim - 1);
    int y1 = iFloor(_y1 * dim + .5f);
    clamp(y1, y0, dim - 1);

    for (int y = y0; y <= y1; y++)
    {
        occD* base = depth + y * dim;
        const occD* it = base + x0;
        const occD* end = base + x1;
        for (; it <= end; it++)
            if (z < *it)
                return TRUE;
    }
    return FALSE;
}

BOOL occRasterizer::test(float _x0, float _y0, float _x1, float _y1, float _z)
{
    const occD z = df_2_s32up(_z) + 1;
    return test_Level(get_depth_level(), occ_dim_0, _x0, _y0, _x1, _y1, z);
    /*
    if	(test_Level(get_depth_level(2),occ_dim_2,_x0,_y0,_x1,_y1,z))
    {
        // Visbible on level 2 - test level 0
        return test_Level(get_depth_level(0),occ_dim_0,_x0,_y0,_x1,_y1,z);
    }
    return FALSE;
    */
}
