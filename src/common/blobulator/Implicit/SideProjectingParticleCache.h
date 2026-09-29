//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef SIDEPROJECTINGPARTICLECACHE_H
#define SIDEPROJECTINGPARTICLECACHE_H

#include "ImpParticle.h"
#include "../Point3D.h"
#include "../SmartArray.h"

struct PCacheElem_t
{
    /* +00 */ short center_x;
    /* +02 */ unsigned char bot_x;
    /* +03 */ unsigned char top_x;
    /* +04 */ PCacheElem_t* prev;
    /* +08 */ PCacheElem_t* next;
    /* +0C */ ImpParticle* p;
};

struct PCacheParticle_t
{
    /* +00 */ ImpParticle* pParticle;
    /* +04 */ float r;
    /* +08 */ float center_y;
    /* +0C */ float center_z;
    /* +10 */ unsigned char bot_y;
    /* +11 */ unsigned char top_y;
    /* +12 */ unsigned char _pad0;
    /* +13 */ unsigned char _pad1;
};

struct PSliceData_t
{
    /* +000 */ PCacheElem_t* slices[100];
    /* +190 */ int _pad0;
    /* +194 */ PCacheElem_t* pElements;
    /* +198 */ int nElements;
    /* +19C */ int nElementsCapacity;
    /* +1A0 */ unsigned char* pClearList;
    /* +1A4 */ int nClearList;
    /* +1A8 */ int nClearListCapacity;
    /* +1AC */ int nMaxElements;
};

struct PCacheSlice_t
{
    /* +00 */ PSliceData_t* pData;
    /* +04 */ void* _pUnk50;
    /* +08 */ int count;
    /* +0C */ int capacity;
};

class SideProjectingParticleCache
{
public:
    SideProjectingParticleCache();
    ~SideProjectingParticleCache();

    void beginTile();
    void endTile();
    void addParticle(ImpParticle* pParticle);
    void buildCache(float cubewidth, float cutoffR, Point3D& bbmins, Point3D& bbmaxs);
    void beginSlice(unsigned char slice);
    void endSlice(unsigned char slice);

private:
    void insertIntoCache(ImpParticle* p, float cubewidth, float cutoffR, Point3D& bbmins, Point3D& bbmaxs);
    void allocSliceData(PCacheSlice_t* pSlice);
    void clearSlicesData(PCacheSlice_t* pSlice);
    void insertParticleIntoSlice(PCacheSlice_t* pSlice, PCacheParticle_t* pParticle);

public:
    static const unsigned int midSentinel = 0;
    static const unsigned int botSentinel = 1;
    static const unsigned int topSentinel = 2;
    static const int max_clearableSlices = 3;

    /* +000 */ int m_nFrames;
    /* +004 */ int _pad0;
    /* +008 */ long long m_nTotalMemory;
    /* +010 */ int m_nTotalSlices;
    /* +014 */ int _pad1;
    /* +018 */ SmartArray<ImpParticle*, 0, 16> particles;
    /* +024 */ SmartArray<PSliceData_t*, 0, 16> freeSlices;
    /* +030 */ SmartArray<PSliceData_t*, 0, 16> allSlices;
    /* +03C */ SmartArray<unsigned char, 0, 16> activeSlices;
    /* +048 */ int m_nCacheElements;
    /* +04C */ PCacheSlice_t slices[101];
};

#endif // SIDEPROJECTINGPARTICLECACHE_H