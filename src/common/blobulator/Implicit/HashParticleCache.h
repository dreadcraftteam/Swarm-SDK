//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose: Marching cubes with optimized hash stuff
// 
//=========================================================================//

#ifndef HASHPARTICLECACHE_H
#define HASHPARTICLECACHE_H

#include "ImpParticle.h"
#include "../Point3D.h"

struct MCArrayItem_t
{
    /* +00 */ MCArrayItem_t* head;
    /* +04 */ MCArrayItem_t* tail;
    /* +08 */ MCArrayItem_t* next;
    /* +0C */ unsigned char x;
    /* +0D */ unsigned char y;
    /* +0E */ unsigned char z;
    /* +0F */ unsigned char _pad;
};

class HashParticleCache
{
public:
    HashParticleCache(float cubewidth, float cutoffR);
    ~HashParticleCache();
    void setOffset(Point3D& offset);
    void addParticle(ImpParticle* pParticle);

private:
    MCArrayItem_t* findArrayItem(unsigned char x, unsigned char y, unsigned char z);
    void generate_check_list(float cutoffR, float cubewidth);

public:
    /* +000 */ int          m_nSomething0;
    /* +004 */ int          m_nSomething1;
    /* +008 */ int          m_nSomething2;
    /* +00C */ int          m_nSomething3;
    /* +010 */ MCArrayItem_t** hash_table;
    /* +014 */ int          hash_table_size;
    /* +018 */ int          hash_table_capacity;
    /* +01C */ int          m_nSomething1C;
    /* +020 */ MCArrayItem_t** items;
    /* +024 */ int          items_count;
    /* +028 */ int          items_capacity;
    /* +02C */ int          _pad2C;
    /* +030 */ Point3D      offset;
    /* +040 */ Point3D      outerDimensions;
    /* +050 */ Point3D      bbmins;
    /* +060 */ Point3D      innerBBMins;
    /* +070 */ int          nParticles;
    /* +074 */ int          m_nSomething74;
    /* +078 */ int          m_nSomething78;
    /* +07C */ int          m_nSomething7C;
    /* +080 */ int          nCheckListSize;
    /* +084 */ int          checkList[?];
    /* +1FC */ float        cubewidth;
    /* +200 */ float        oneOverCubewidth;
    /* +204 */ int          nNodes;
    /* +208 */ struct { ImpParticle* p; void* next; } nodes[?];
    /* +62E8 */ int         m_nSomething62E8;

    static const int mcpc_xSize = 0;
    static const int mcpc_ySize = 0;
    static const int mcpc_zSize = 0;
};

#endif // HASHPARTICLECACHE_H