//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef IMPPARTICLE_H
#define IMPPARTICLE_H

#include "../Point3D.h"

class ImpParticle
{
public:
    ImpParticle();
    void setFieldScale(float fscale) { scale = fscale; }

    /* +00 */ Point3D center;
    /* +10 */ float fieldRScaleSq;
    /* +14 */ float scale;
};

class ImpParticleWithOneInterpolant : public ImpParticle
{
public:
    ImpParticleWithOneInterpolant();
    /* +20 */ Point3D interpolants1;
};

class ImpParticleWithTwoInterpolants : public ImpParticleWithOneInterpolant
{
public:
    ImpParticleWithTwoInterpolants();
    /* +30 */ Point3D interpolants2;
};

class ImpParticleWithFourInterpolants : public ImpParticleWithTwoInterpolants
{
public:
    ImpParticleWithFourInterpolants();
    /* +40 */ Point3D interpolants3;
    /* +50 */ Point3D interpolants4;
};

#endif