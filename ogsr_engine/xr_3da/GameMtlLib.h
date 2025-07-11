//---------------------------------------------------------------------------
#pragma once

#define GAMEMTL_CURRENT_VERSION 0x0001
//----------------------------------------------------
#define GAMEMTLS_CHUNK_VERSION 0x1000
#define GAMEMTLS_CHUNK_AUTOINC 0x1001
#define GAMEMTLS_CHUNK_MTLS 0x1002
#define GAMEMTLS_CHUNK_MTLS_PAIR 0x1003
//----------------------------------------------------
#define GAMEMTL_CHUNK_MAIN 0x1000
#define GAMEMTL_CHUNK_FLAGS 0x1001
#define GAMEMTL_CHUNK_PHYSICS 0x1002
#define GAMEMTL_CHUNK_FACTORS 0x1003
#define GAMEMTL_CHUNK_FLOTATION 0x1004
#define GAMEMTL_CHUNK_DESC 0x1005
#define GAMEMTL_CHUNK_INJURIOUS 0x1006
#define GAMEMTL_CHUNK_DENSITY 0x1007
#define GAMEMTL_CHUNK_FACTORS_MP 0x1008
//----------------------------------------------------
#define GAMEMTLPAIR_CHUNK_PAIR 0x1000
#define GAMEMTLPAIR_CHUNK_BREAKING 0x1002
#define GAMEMTLPAIR_CHUNK_STEP 0x1003
#define GAMEMTLPAIR_CHUNK_COLLIDE 0x1005
//----------------------------------------------------

#define GAMEMTL_SUBITEM_COUNT 4

#define GAMEMTL_NONE_ID u32(-1)
#define GAMEMTL_NONE_IDX u16(-1)
#define GAMEMTL_FILENAME "gamemtl.xr"

DEFINE_VECTOR(ref_sound, SoundVec, SoundIt);
DEFINE_VECTOR(shared_str, PSVec, PSIt);

#include "../Include/xrRender/WallMarkArray.h"
#include "../Include/xrRender/RenderFactory.h"

struct ENGINE_API SGameMtl
{
    friend class CGameMtlLibrary;

protected:
    int ID; // auto number
public:
    enum
    {
        //flBreakable = (1ul << 0ul),
        //flShootable = (1ul<<1ul),
        flBounceable = (1ul << 2ul),
        //flSkidmark = (1ul << 3ul),
        flBloodmark = (1ul << 4ul),
        flClimable = (1ul << 5ul),
        //flWalkOn = (1ul<<6ul), // obsolette
        flPassable = (1ul << 7ul),
        flDynamic = (1ul << 8ul),
        flLiquid = (1ul << 9ul),
        flSuppressShadows = (1ul << 10ul),
        flSuppressWallmarks = (1ul << 11ul),
        flActorObstacle = (1ul << 12ul),
        //flNoRicoshet = (1ul << 13ul),
        flInjurious = (1ul << 28ul), // flInjurious = fInjuriousSpeed > 0.f
        //flShootable = (1ul << 29ul),
        //flTransparent = (1ul << 30ul),
        flSlowDown = (1ul << 31ul) // flSlowDown = (fFlotationFactor<1.f)
    };

public:
    shared_str m_Name;
    shared_str m_Desc;

    Flags32 Flags;

    // physics part
    float fPHFriction; // ?
    float fPHDamping; // ?
    float fPHSpring; // ?
    float fPHBounceStartVelocity; // ?
    float fPHBouncing; // ?

    // shoot&bounce&visibility&flotation
    float fFlotationFactor; // 0.f - 1.f   	(1.f-полностью проходимый)
    float fShootFactor; // 0.f - 1.f	(1.f-полностью простреливаемый)
    float fBounceDamageFactor; // 0.f - 100.f
    float fInjuriousSpeed; // 0.f - ...	(0.f-не отбирает здоровье (скорость уменьшения здоровья))
    float fVisTransparencyFactor; // 0.f - 1.f	(1.f-полностью прозрачный)
    float fSndOcclusionFactor; // 0.f - 1.f    (1.f-полностью слышен)
    float fDensityFactor;

public:
    SGameMtl()
    {
        ID = -1;
        m_Name = "unknown";
        Flags.zero();
        // factors
        fFlotationFactor = 1.f;
        fShootFactor = 0.f;
        fBounceDamageFactor = 1.f;
        fInjuriousSpeed = 0.f;
        fVisTransparencyFactor = 0.f;
        fSndOcclusionFactor = 0.f;
        // physics
        fPHFriction = 1.f;
        fPHDamping = 1.f;
        fPHSpring = 1.f;
        fPHBounceStartVelocity = 0.f;
        fPHBouncing = 0.1f;
        fDensityFactor = 0.0f;
    }
    void Load(IReader& fs);
    void Save(IWriter& fs);
    IC int GetID() { return ID; }
};
DEFINE_VECTOR(SGameMtl*, GameMtlVec, GameMtlIt);

struct ENGINE_API SGameMtlPair
{
    friend class CGameMtlLibrary;
    CGameMtlLibrary* m_Owner;

private:
    int mtl0;
    int mtl1;

protected:
    int ID; // auto number
    int ID_parent;

public:
    enum
    {
        //flFlotation		= (1<<0),
        //flBreakingSounds = (1 << 1),
        //flStepSounds = (1 << 2),
        //flCollideSounds	= (1<<3),
        //flCollideSounds = (1 << 4),
        //flCollideParticles = (1 << 5),
        //flCollideMarks = (1 << 6)
    };
    Flags32 OwnProps; // unused in game
    //	properties
    SoundVec BreakingSounds;
    SoundVec StepSounds;
    SoundVec CollideSounds;

    PSVec CollideParticles;

    xr_vector<std::string> BreakingSoundNames;
    xr_vector<std::string> StepSoundNames;
    xr_vector<std::string> CollideSoundNames;

    FactoryPtr<IWallMarkArray> m_pCollideMarks;

public:
    SGameMtlPair(CGameMtlLibrary* owner)
    {
        mtl0 = -1;
        mtl1 = -1;
        ID = -1;
        ID_parent = -1;
        m_Owner = owner;
        OwnProps.one();
    }
    ~SGameMtlPair();
    IC int GetMtl0() { return mtl0; }
    IC int GetMtl1() { return mtl1; }
    IC int GetID() { return ID; }
    IC void SetPair(int m0, int m1)
    {
        mtl0 = m0;
        mtl1 = m1;
    }
    IC bool IsPair(int m0, int m1) { return !!(((mtl0 == m0) && (mtl1 == m1)) || ((mtl0 == m1) && (mtl1 == m0))); }
    void Save(IWriter& fs);
    void Load(IReader& fs);
    //IC int GetParent() { return ID_parent; }
    BOOL SetParent(int parent);
#ifdef DEBUG
    LPCSTR dbg_Name();
#endif

    void CreateAllSounds();
    void CreateAllParticles();
};

DEFINE_VECTOR(SGameMtlPair*, GameMtlPairVec, GameMtlPairIt);

class ENGINE_API CGameMtlLibrary
{
    int material_index;
    int material_pair_index;

    GameMtlVec materials;
    GameMtlPairVec material_pairs;

    // game part
    u32 material_count;
    GameMtlPairVec material_pairs_rt;

public:
    CGameMtlLibrary();
    ~CGameMtlLibrary()
    {
        /*
        R_ASSERT		(0==material_pairs_rt.size());
        R_ASSERT		(0==material_pairs.size());
        R_ASSERT		(0==materials.size());
        */
    }
    IC void Unload()
    {
        material_count = 0;
        material_pairs_rt.clear();

        for (GameMtlIt m_it = materials.begin(); materials.end() != m_it; ++m_it)
            xr_delete(*m_it);
        materials.clear();
        for (GameMtlPairIt p_it = material_pairs.begin(); material_pairs.end() != p_it; ++p_it)
            xr_delete(*p_it);
        material_pairs.clear();
    }
    // material routine
    IC GameMtlIt GetMaterialIt(LPCSTR name)
    {
        for (GameMtlIt it = materials.begin(); materials.end() != it; ++it)
            if (0 == strcmpi(*(*it)->m_Name, name))
                return it;
        return materials.end();
    }
    IC GameMtlIt GetMaterialIt(shared_str& name)
    {
        for (GameMtlIt it = materials.begin(); materials.end() != it; ++it)
            if (name.equal((*it)->m_Name))
                return it;
        return materials.end();
    }
    IC GameMtlIt GetMaterialItByID(int ID)
    {
        for (GameMtlIt it = materials.begin(); materials.end() != it; ++it)
            if ((*it)->ID == ID)
                return it;
        return materials.end();
    }
    IC u32 GetMaterialID(LPCSTR name)
    {
        GameMtlIt it = GetMaterialIt(name);
        return (it == materials.end()) ? GAMEMTL_NONE_ID : (*it)->ID;
    }
    // game
    IC SGameMtl* GetMaterialByID(s32 id) { return GetMaterialByIdx(GetMaterialIdx(id)); }

    IC u16 GetMaterialIdx(int ID)
    {
        GameMtlIt it = GetMaterialItByID(ID);
        if (materials.end() != it)
        {
            return static_cast<u16>(std::distance(materials.begin(), it));
        }
        else
        {
            Msg("!![%s] material id [%d] not found!", __FUNCTION__, ID);
            return 0;
        }
    }
    IC u16 GetMaterialIdx(LPCSTR name)
    {
        GameMtlIt it = GetMaterialIt(name);
        if (materials.end() != it)
        {
            return static_cast<u16>(std::distance(materials.begin(), it));
        }
        else
        {
            Msg("!![%s] material name [%s] not found!", __FUNCTION__, name);
            return 0;
        }
    }
    IC SGameMtl* GetMaterialByIdx(u16 idx)
    {
        if (idx < static_cast<u16>(materials.size()))
        {
            return materials[idx];
        }
        else
        {
            Msg("!![%s] material id [%u] not found!", __FUNCTION__, idx);
            return materials.front();
        }
    }

    IC GameMtlIt FirstMaterial() { return materials.begin(); }
    IC GameMtlIt LastMaterial() { return materials.end(); }
    IC u32 CountMaterial() { return materials.size(); }

    // game
    IC SGameMtlPair* GetMaterialPair(u16 idx0, u16 idx1)
    {
        R_ASSERT((idx0 < material_count) && (idx1 < material_count));
        return material_pairs_rt[idx1 * material_count + idx0];
    }

    IC GameMtlPairIt FirstMaterialPair() { return material_pairs.begin(); }
    IC GameMtlPairIt LastMaterialPair() { return material_pairs.end(); }

    // IO routines
    void Load();
    bool Save();

private:
    void loadSounds();
    void loadParticles();
};

#define GET_RANDOM(a_vector) (a_vector[Random.randI(a_vector.size())])

#define CLONE_MTL_SOUND(_res_, _mtl_pair_, _a_vector_) \
    { \
        VERIFY(!_mtl_pair_##->_a_vector_.empty(), _mtl_pair_->dbg_Name()); \
        _res_.clone(GET_RANDOM(_mtl_pair_##->_a_vector_), st_Effect, sg_SourceType); \
    }

extern ENGINE_API CGameMtlLibrary GMLib;

#include "../include/xrapi/xrapi.h"
