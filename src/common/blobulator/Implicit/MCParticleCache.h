//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef MCPARTICLECACHE_H
#define MCPARTICLECACHE_H

#include "ImpParticle.h"
#include "../Point3D.h"

struct MCArrayItem_t
{
    /* +00 */ MCArrayItem_t* next;
    /* +04 */ ImpParticle* p;
};

class MCParticleCache
{
public:
    MCParticleCache(float cubewidth, float cutoffR);
    ~MCParticleCache();

    void setOffset(Point3D& offset);
    void addParticle(ImpParticle* pParticle);

private:
    void generate_check_list(float cutoffR, float cubewidth);

public:
    /* +000 */ int m_nSomething0; 
    /* +004 */ int m_nSomething1; 
    /* +008 */ int m_nSomething2; 
    /* +00C */ int  m_nSomething3; 
    /* +010 */ Point3D offset; 
    /* +020 */ Point3D outerDimensions; 
    /* +030 */ Point3D innerBBMins; 
    /* +040 */ Point3D outerBBMins; 
    /* +050 */ int nParticles; 
    /* +054 */ int m_nSomething54; 
    /* +058 */ int m_nSomething58; 
    /* +05C */ int m_nSomething5C; 
    /* +060 */ int nCheckListSize; 
    /* +064 */ int checkList[?];
    /* +258 */ float cubewidth; 
    /* +25C */ float oneOverCubewidth;
    /* +260 */ int nNodes; 
    /* +264 */ struct { ImpParticle* p; void* next; } nodes[?]; 
    /* +6344 */ int nActiveCells;
    /* +6348 */ MCArrayItem_t* activeCells[?];
    /* +93B8 */ int m_nSomething93B8;
    /* +186ADC */ int m_nSomething186ADC; 
    static MCArrayItem_t (*mcpc_array)[100][100][100];
};

#endif // MCPARTICLECACHE_H