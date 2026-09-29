//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef VOLUMETRICPARTICLECACHE_H
#define VOLUMETRICPARTICLECACHE_H

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

struct pcache_YZ_t
{
    unsigned char y, z;
};

class VolumetricParticleCache
{
public:
    VolumetricParticleCache();
    ~VolumetricParticleCache();

    void beginTile();
    void endTile();

    void addParticle(ImpParticle* pParticle);
    void buildCache(float cubewidth, float cutoffR, Point3D& bbmins, Point3D& bbmaxs);

private:
    void insertIntoCache(ImpParticle* p, float cubewidth, float cutoffR,
        Point3D& bbmins, Point3D& bbmaxs);

public:
    /* +00 */ PCacheElem_t* botSentinel;
    /* +04 */ PCacheElem_t* midSentinel;
    /* +08 */ PCacheElem_t* topSentinel;
    /* +0C */ SmartArray<ImpParticle*, 0, 16> particles;
    /* +18 */ SmartArray<PCacheElem_t*, 0, 16> cacheElements;
    /* +24 */ SmartArray<pcache_YZ_t, 0, 16>   clearList;
    /* +30 */ int curCacheElement;
    /* +34 */ PCacheElem_t** cachePlane;
};

#endif // VOLUMETRICPARTICLECACHE_H