#include "stdafx.h"
#include "WeaponRG6.h"
#include "entity.h"
#include "explosiveRocket.h"
#include "level.h"
#include "clsid_game.h"

#include "MathUtils.h"
#ifdef DEBUG
#include "phdebug.h"
#endif

CWeaponRG6::~CWeaponRG6() {}

BOOL CWeaponRG6::net_Spawn(CSE_Abstract* DC)
{
    BOOL l_res = inheritedSG::net_Spawn(DC);
    if (!l_res)
        return l_res;

    if (iAmmoElapsed && !getCurrentRocket())
    {
        shared_str grenade_name = m_ammoTypes[0];
        shared_str fake_grenade_name = pSettings->r_string(grenade_name, "fake_grenade_name");

        if (fake_grenade_name.size())
        {
            int k = iAmmoElapsed;
            while (k)
            {
                k--;
                inheritedRL::SpawnRocket(*fake_grenade_name, this);
            }
        }
        //			inheritedRL::SpawnRocket(*fake_grenade_name, this);
    }

    return l_res;
};

void CWeaponRG6::Load(LPCSTR section)
{
    inheritedRL::Load(section);
    inheritedSG::Load(section);
}

#include "inventory.h"
#include "inventoryOwner.h"

void CWeaponRG6::LaunchGrenade(const Fvector& p1, const Fvector& d1)
{
    if (getRocketCount())
    {
        Fvector p, d;
        p = p1;
        d = d1;

        CEntity* E = smart_cast<CEntity*>(H_Parent());
        if (E)
        {
#ifdef DEBUG
            CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
            if (NULL == io->inventory().ActiveItem())
            {
                Log("current_state", GetState());
                Log("next_state", GetNextState());
                Log("state_time", m_dwStateTime);
                Log("item_sect", cNameSect().c_str());
                Log("H_Parent", H_Parent()->cNameSect().c_str());
            }
#endif
            // E->g_fireParams (this, p1,d);
        }

        Fmatrix launch_matrix;
        launch_matrix.identity();
        launch_matrix.k.set(d);
        Fvector::generate_orthonormal_basis(launch_matrix.k, launch_matrix.j, launch_matrix.i);
        launch_matrix.c.set(p);

        if (IsZoomed() && H_Parent()->CLS_ID == CLSID_OBJECT_ACTOR)
        {
            H_Parent()->setEnabled(FALSE);
            setEnabled(FALSE);

            collide::rq_result RQ;
            BOOL HasPick = Level().ObjectSpace.RayPick(p, d, 300.0f, collide::rqtStatic, RQ, this);

            setEnabled(TRUE);
            H_Parent()->setEnabled(TRUE);

            if (HasPick)
            {
                //			collide::rq_result& RQ = HUD().GetCurrentRayQuery();
                Fvector Transference;
                // Transference.add(p1, Fvector().mul(d, RQ.range));
                Transference.mul(d, RQ.range);
                Fvector res[2];
#ifdef DEBUG
                DBG_OpenCashedDraw();
                DBG_DrawLine(p1, Fvector().add(p1, d), D3DCOLOR_XRGB(255, 0, 0));
#endif
                u8 canfire0 = TransferenceAndThrowVelToThrowDir(Transference, CRocketLauncher::m_fLaunchSpeed, EffectiveGravity(), res);
#ifdef DEBUG
                if (canfire0 > 0)
                    DBG_DrawLine(p1, Fvector().add(p1, res[0]), D3DCOLOR_XRGB(0, 255, 0));
                if (canfire0 > 1)
                    DBG_DrawLine(p1, Fvector().add(p1, res[1]), D3DCOLOR_XRGB(0, 0, 255));
                DBG_ClosedCashedDraw(30000);
#endif
                if (canfire0 != 0)
                {
                    //					Msg ("d[%f,%f,%f] - res [%f,%f,%f]", d.x, d.y, d.z, res[0].x, res[0].y, res[0].z);
                    d = res[0];
                };
            }
        };

        d.normalize();
        d.mul(m_fLaunchSpeed);
        VERIFY2(_valid(launch_matrix), "CWeaponRG6::LaunchGrenade. Invalid launch_matrix");
        CRocketLauncher::LaunchRocket(launch_matrix, d, {});

        CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
        VERIFY(pGrenade);
        pGrenade->SetInitiator(H_Parent()->ID());
        pGrenade->SetRealGrenadeName(m_ammoTypes[m_ammoType]);

        {
            NET_Packet P;
            u_EventGen(P, GE_LAUNCH_ROCKET, ID());
            P.w_u16(u16(getCurrentRocket()->ID()));
            u_EventSend(P);
        }
        dropCurrentRocket();
    }
}

void CWeaponRG6::FireTrace(const Fvector& P, const Fvector& D)
{
    inheritedSG::FireTrace(P, D);
    if (!IsMisfire())
    {
        LaunchGrenade(P, D);
    }
}

u8 CWeaponRG6::AddCartridge(u8 cnt)
{
    u8 t = inheritedSG::AddCartridge(cnt);
    u8 k = cnt - t;
    shared_str fake_grenade_name = pSettings->r_string(*m_ammoTypes[m_ammoType], "fake_grenade_name");
    while (k)
    {
        --k;
        inheritedRL::SpawnRocket(*fake_grenade_name, this);
    }
    return k;
}

void CWeaponRG6::OnEvent(NET_Packet& P, u16 type)
{
    inheritedSG::OnEvent(P, type);

    u16 id;
    switch (type)
    {
    case GE_OWNERSHIP_TAKE: {
        P.r_u16(id);
        inheritedRL::AttachRocket(id, this);
    }
    break;
    case GE_OWNERSHIP_REJECT:
    case GE_LAUNCH_ROCKET: {
        bool bLaunch = (type == GE_LAUNCH_ROCKET);
        P.r_u16(id);
        inheritedRL::DetachRocket(id, bLaunch);
    }
    break;
    }
}
