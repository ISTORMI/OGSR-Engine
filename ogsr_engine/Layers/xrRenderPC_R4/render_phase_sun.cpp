#include "stdafx.h"

#include "render_phase_sun.h"
#include "r4_R_sun_support.h"

using namespace DirectX;

void render_sun::init()
{
    const float fBias = -0.0000025f;

    m_sun_cascades[0].reset_chain = true;
    m_sun_cascades[0].size = ps_ssfx_shadow_cascades.x;
    m_sun_cascades[0].bias = m_sun_cascades[0].size * fBias;

    m_sun_cascades[1].size = ps_ssfx_shadow_cascades.y;
    m_sun_cascades[1].bias = m_sun_cascades[1].size * fBias;

    m_sun_cascades[2].size = ps_ssfx_shadow_cascades.z;
    m_sun_cascades[2].bias = m_sun_cascades[2].size * fBias;
    
    sun = (light*)RImplementation.Lights.sun_adapted._get(); // fuckingsun

    const Fcolor sun_color = sun->color;
    o.active = ps_r2_ls_flags.test(R2FLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);

    if (!o.active)
        return;

    // pre-allocate contexts
    for (unsigned int& contexts_id : contexts_ids)
    {
        contexts_id = RImplementation.alloc_context();
        R_ASSERT(contexts_id != CHW::INVALID_CONTEXT_ID);
    }

    o.mt_calc_enabled = ps_r2_ls_flags.test(R2FLAG_EXP_MT_SUN);
    o.mt_draw_enabled = ps_r2_ls_flags.test(R2FLAG_EXP_MT_SUN);
}

void render_sun::calculate()
{
    ZoneScoped;

    need_to_render_sunshafts = RImplementation.Target->need_to_render_sunshafts();
    last_cascade_chain_mode = m_sun_cascades[R__NUM_SUN_CASCADES - 1].reset_chain;

    if (need_to_render_sunshafts)
        m_sun_cascades[R__NUM_SUN_CASCADES - 1].reset_chain = true;

    // Lets begin from base frustum
    Fmatrix fullxform_inv = Device.mInvFullTransform;

    // Create approximate ortho-xform
    // view: auto find 'up' and 'right' vectors
    Fmatrix mdir_View, mdir_Project;
    Fvector L_dir, L_up, L_right, L_pos;
    L_pos.set(sun->position);
    L_dir.set(sun->direction).normalize();
    L_right.set(1, 0, 0);
    if (_abs(L_right.dotproduct(L_dir)) > .99f)
        L_right.set(0, 0, 1);
    L_up.crossproduct(L_dir, L_right).normalize();
    L_right.crossproduct(L_up, L_dir).normalize();
    mdir_View.build_camera_dir(L_pos, L_dir, L_up);

    // THIS NEED TO BE A CONSTATNT
    Fplane light_top_plane;
    light_top_plane.build_unit_normal(L_pos, L_dir);
    float dist = light_top_plane.classify(Device.vCameraPosition);

    // build viewport xform
    float view_dim = float(RImplementation.o.smapsize);
    Fmatrix m_viewport = {
        view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f,
        -view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        view_dim / 2.f, view_dim / 2.f, 0.0f, 1.0f};
    Fmatrix m_viewport_inv;

    XMStoreFloat4x4((XMFLOAT4X4*)&m_viewport_inv, XMMatrixInverse(nullptr, XMLoadFloat4x4((XMFLOAT4X4*)&m_viewport)));

    // Compute volume(s) - something like a frustum for infinite directional light
    // Also compute virtual light position and sector it is inside
    xr_vector<Fplane> cull_planes;

    CFrustum cull_frustum[R__NUM_SUN_CASCADES];
    Fvector3 cull_COP[R__NUM_SUN_CASCADES];
    Fmatrix cull_xform[R__NUM_SUN_CASCADES];

    for (int cascade_ind = 0; cascade_ind < R__NUM_SUN_CASCADES; ++cascade_ind)
    {
        cull_planes.clear();

        //******************************* Need to be placed after cuboid built **************************
        // COP - 100 km away
        cull_COP[cascade_ind].mad(Device.vCameraPosition, sun->direction, -tweak_COP_initial_offs);

#ifdef DEBUG
        typedef FixedConvexVolume<true> t_cuboid;
#else
        typedef FixedConvexVolume<false> t_cuboid;
#endif

        t_cuboid light_cuboid;
        {
            // Initialize the first cascade rays, then each cascade will initialize rays for next one.
            if (cascade_ind == 0 || m_sun_cascades[cascade_ind].reset_chain)
            {
                Fvector3 near_p, edge_vec;
                light_cuboid.view_frustum_rays.reserve(4);
                for (int p = 0; p < 4; p++)
                {
                    near_p = wform(fullxform_inv, sun::corners[sun::facetable[4][p]]);

                    edge_vec = wform(fullxform_inv, sun::corners[sun::facetable[5][p]]);
                    edge_vec.sub(near_p);
                    edge_vec.normalize();

                    light_cuboid.view_frustum_rays.emplace_back(near_p, edge_vec);
                }
            }
            else
                light_cuboid.view_frustum_rays = m_sun_cascades[cascade_ind].rays;

            light_cuboid.view_ray.P = Device.vCameraPosition;
            light_cuboid.view_ray.D = Device.vCameraDirection;
            light_cuboid.light_ray.P = L_pos;
            light_cuboid.light_ray.D = L_dir;
        }

        float map_size = m_sun_cascades[cascade_ind].size;

        XMStoreFloat4x4((XMFLOAT4X4*)&mdir_Project,
                        XMMatrixOrthographicOffCenterLH(-map_size * 0.5f, map_size * 0.5f, -map_size * 0.5f, map_size * 0.5f, 0.1f,
                                                        dist + /*sqrt(2)*/ 1.41421f * map_size));
        // snap view-position to pixel
        cull_xform[cascade_ind].mul(mdir_Project, mdir_View);
        Fmatrix cull_xform_inv;
        cull_xform_inv.invert(cull_xform[cascade_ind]);

        for (int p = 0; p < 8; p++)
        {
            Fvector3 xf = wform(cull_xform_inv, sun::corners[p]);
            light_cuboid.light_cuboid_points[p] = xf;
        }

        // only side planes
        for (int plane = 0; plane < 4; plane++)
        {
            for (int pt = 0; pt < 4; pt++)
            {
                int asd = sun::facetable[plane][pt];
                light_cuboid.light_cuboid_polys[plane].points[pt] = asd;
            }
        }

        Fvector lightXZshift;
        light_cuboid.compute_caster_model_fixed(cull_planes, lightXZshift, m_sun_cascades[cascade_ind].size, m_sun_cascades[cascade_ind].reset_chain);

        // Initialize rays for the next cascade
        if (cascade_ind < R__NUM_SUN_CASCADES - 1)
            m_sun_cascades[cascade_ind + 1].rays = light_cuboid.view_frustum_rays;

#ifdef DEBUG
        static bool draw_debug = false;
        if (draw_debug && cascade_ind == 0)
            for (u32 it = 0; it < cull_planes.size(); it++)
                RImplementation.Target->dbg_addplane(cull_planes[it], it * 0xFFF);
#endif

        Fvector cam_shifted = L_pos;
        cam_shifted.add(lightXZshift);

        // rebuild the view transform with the shift.
        mdir_View.identity();
        mdir_View.build_camera_dir(cam_shifted, L_dir, L_up);
        cull_xform[cascade_ind].identity();
        cull_xform[cascade_ind].mul(mdir_Project, mdir_View);
        cull_xform_inv.invert(cull_xform[cascade_ind]);

        // Create frustum for query
        cull_frustum[cascade_ind]._clear();
        for (auto& cull_plane : cull_planes)
            cull_frustum[cascade_ind]._add(cull_plane);

        {
            Fvector cam_proj = Device.vCameraPosition;
            const float align_aim_step_coef = 4.f;
            cam_proj.set(floorf(cam_proj.x / align_aim_step_coef) + align_aim_step_coef / 2, 
                         floorf(cam_proj.y / align_aim_step_coef) + align_aim_step_coef / 2,
                         floorf(cam_proj.z / align_aim_step_coef) + align_aim_step_coef / 2);
            cam_proj.mul(align_aim_step_coef);
            Fvector cam_pixel = wform(cull_xform[cascade_ind], cam_proj);
            cam_pixel = wform(m_viewport, cam_pixel);
            Fvector shift_proj = lightXZshift;
            cull_xform[cascade_ind].transform_dir(shift_proj);
            m_viewport.transform_dir(shift_proj);

            const float align_granularity = 4.f;
            shift_proj.x = shift_proj.x > 0 ? align_granularity : -align_granularity;
            shift_proj.y = shift_proj.y > 0 ? align_granularity : -align_granularity;
            shift_proj.z = 0;

            cam_pixel.x = cam_pixel.x / align_granularity - floorf(cam_pixel.x / align_granularity);
            cam_pixel.y = cam_pixel.y / align_granularity - floorf(cam_pixel.y / align_granularity);
            cam_pixel.x *= align_granularity;
            cam_pixel.y *= align_granularity;
            cam_pixel.z = 0;

            cam_pixel.sub(shift_proj);

            m_viewport_inv.transform_dir(cam_pixel);
            cull_xform_inv.transform_dir(cam_pixel);
            Fvector diff = cam_pixel;
            constexpr float sign_test = -1.f;
            diff.mul(sign_test);
            Fmatrix adjust;
            adjust.translate(diff);
            cull_xform[cascade_ind].mulB_44(adjust);
        }

        m_sun_cascades[cascade_ind].cull_xform = cull_xform[cascade_ind];

        s32 limit = RImplementation.o.smapsize - 1;
        sun->X.D[cascade_ind].minX = 0;
        sun->X.D[cascade_ind].maxX = limit;
        sun->X.D[cascade_ind].minY = 0;
        sun->X.D[cascade_ind].maxY = limit;
        sun->X.D[cascade_ind].combine = cull_xform[cascade_ind];

        // full-xform
    }

    const auto process_cascade = [&](const u32 cascade_ind) {
        // Begin SMAP-render
        auto& dsgraph = RImplementation.get_context(contexts_ids[cascade_ind]);
        {
            dsgraph.phase = CRender::PHASE_SMAP;
            dsgraph.r_pmask(true, false);

            IRender_Sector::sector_id_t id = RImplementation.get_largest_sector();

            // Fill the database
            dsgraph.build_subspace(id, &cull_frustum[cascade_ind], cull_xform[cascade_ind], cull_COP[cascade_ind], TRUE);
        }
    };

    if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_SUN))
    {
        for (int cascade_ind = 0; cascade_ind < R__NUM_SUN_CASCADES; ++cascade_ind)
        {
            sun_cascade_pool.submit_detach(process_cascade, cascade_ind);
        }

        sun_cascade_pool.wait_for_tasks();
    }
    else
    {
        for (int cascade_ind = 0; cascade_ind < R__NUM_SUN_CASCADES; ++cascade_ind)
        {
            process_cascade(cascade_ind);
        }
    }
}

void render_sun::render()
{
    if (!o.active)
        return;

    if (need_to_render_sunshafts)
        m_sun_cascades[R__NUM_SUN_CASCADES - 1].reset_chain = last_cascade_chain_mode;

    // Render shadow-map
    const auto render_cascade = [&](const u32 cascade_ind) {
        // TracyD3D11Zone(HW.profiler_ctx, "render_sun::render_cascade");

        auto& dsgraph = RImplementation.get_context(contexts_ids[cascade_ind]);

        const bool bNormal = !dsgraph.mapNormalPasses[0][0].empty() || !dsgraph.mapMatrixPasses[0][0].empty();
        const bool bSpecial = !dsgraph.mapNormalPasses[1][0].empty() || !dsgraph.mapMatrixPasses[1][0].empty() || !dsgraph.mapSorted.empty();

        if (bNormal || bSpecial)
        {
            RImplementation.Target->phase_smap_direct(dsgraph.cmd_list, sun, cascade_ind);
            dsgraph.cmd_list.set_xform_world(Fidentity);
            dsgraph.cmd_list.set_xform_view(Fidentity);
            dsgraph.cmd_list.set_xform_project(sun->X.D[cascade_ind].combine);
            dsgraph.r_dsgraph_render_graph(0);

            // иногда тут творится что то странное с ctable. попробуем пропускать рендеры травы для каскадов на префетче
            if (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS) && !Device.dwPrecacheFrame)
            {
                if (cascade_ind <= u32(ps_ssfx_grass_shadows.x)) // check cascade
                {
                    RImplementation.Details->Render(dsgraph.cmd_list, true);
                }
            }

            sun->X.D[cascade_ind].transluent = FALSE;

            if (bSpecial)
            {
                sun->X.D[cascade_ind].transluent = TRUE;
                RImplementation.Target->phase_smap_direct_tsh(dsgraph.cmd_list, sun, cascade_ind);
                dsgraph.r_dsgraph_render_graph(1); // normal level, secondary priority
                dsgraph.r_dsgraph_render_sorted(); // strict-sorted geoms
            }
        }
    };

    if (ps_r2_ls_flags.test(R2FLAG_EXP_MT_SUN))
    {
        for (int cascade_ind = 0; cascade_ind < R__NUM_SUN_CASCADES; ++cascade_ind)
        {
            sun_cascade_pool.submit_detach(render_cascade, cascade_ind);
        }

        sun_cascade_pool.wait_for_tasks();
    }
    else
    {
        for (int cascade_ind = 0; cascade_ind < R__NUM_SUN_CASCADES; ++cascade_ind)
        {
            render_cascade(cascade_ind);
        }
    }
}

void render_sun::flush()
{
    if (!o.active)
        return;

    for (int cascade_ind = 0; cascade_ind < R__NUM_SUN_CASCADES; ++cascade_ind)
    {
        accumulate_cascade(cascade_ind);
    }

    auto& cmd_list_imm = RImplementation.get_imm_context().cmd_list;
    cmd_list_imm.Invalidate();

    // Restore XForms
    cmd_list_imm.set_xform_world(Fidentity);
    cmd_list_imm.set_xform_view(Device.mView);
    cmd_list_imm.set_xform_project(Device.mProject);
}

void render_sun::accumulate_cascade(u32 cascade_ind)
{
    // TracyD3D11Zone(HW.profiler_ctx, "render_sun::accumulate_cascade");

    auto& dsgraph = RImplementation.get_context(contexts_ids[cascade_ind]);

    // Accumulate
    RImplementation.Target->rt_smap_depth->set_slice_read(contexts_ids[cascade_ind]);

    {
        if (cascade_ind == 0)
        {
            PIX_EVENT_CTX(dsgraph.cmd_list, SE_SUN_NEAR);
            RImplementation.Target->accum_direct_cascade(dsgraph.cmd_list, SE_SUN_NEAR, m_sun_cascades[cascade_ind].cull_xform
                , m_sun_cascades[cascade_ind].cull_xform
                , m_sun_cascades[cascade_ind].bias);
        }
        else if (cascade_ind == 1)
        {
            PIX_EVENT_CTX(dsgraph.cmd_list, SE_SUN_MIDDLE);
            RImplementation.Target->accum_direct_cascade(dsgraph.cmd_list, SE_SUN_MIDDLE, m_sun_cascades[cascade_ind].cull_xform
                , m_sun_cascades[cascade_ind - 1].cull_xform
                , m_sun_cascades[cascade_ind].bias);
        }
        else if (cascade_ind == 2)
        {
            PIX_EVENT_CTX(dsgraph.cmd_list, SE_SUN_FAR);
            RImplementation.Target->accum_direct_cascade(dsgraph.cmd_list, SE_SUN_FAR, m_sun_cascades[cascade_ind].cull_xform
                , m_sun_cascades[cascade_ind - 1].cull_xform
                , m_sun_cascades[cascade_ind].bias);
        }
    }

    dsgraph.cmd_list.submit(); // TODO: move into release (rename to submit?)
    RImplementation.release_context(dsgraph.context_id);
}
