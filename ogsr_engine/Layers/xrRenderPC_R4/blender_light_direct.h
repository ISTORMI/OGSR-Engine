#pragma once

class CBlender_accum_direct : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: accumulate direct light"; }

    virtual void Compile(CBlender_Compile& C);

    CBlender_accum_direct();
    virtual ~CBlender_accum_direct();
};