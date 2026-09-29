//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef IMPTILER_H
#define IMPTILER_H

#include "ImpParticle.h"
#include "../SmartArray.h"
#include "../Point3D.h"

class SweepRenderer;

class ImpTile
{
public:
    ImpTile(int x, int y, int z);
    ~ImpTile();

    /* +00 */ bool done;
    /* +04 */ int x, y, z;
    /* +10 */ Point3D center;
    /* +20 */ bool old_tile;
    /* +30 */ Point3D debug_color;
    /* +40 */ float dist;
    /* +44 */ SmartArray<ImpParticle*, 0, 16> particles;
};

class ImpTiler
{
public:
    ImpTiler(SweepRenderer* pSweepRenderer);
    ~ImpTiler();

    void beginFrame(Point3D& _offset, void* pRenderContext, bool tile_mode);
    void endFrame();

    void insertParticle(ImpParticle* pParticle);

    void drawSurface();
    void drawSurfaceSorted(Point3D& pPoint);
    void drawTile(ImpTile* pTile);
    void drawTile(int x, int y, int z);

    int getNoTiles();
    ImpTile* getTile(int index);
    Point3D getTileOffset(int index);

    SweepRenderer* getRenderer() { return m_sweepRenderer; }
    void setMaxNoTilesToDraw(int maxnot) { maxNoTileToDraw = maxnot; }
    Point3D& getRenderDim() { return render_dim; }
    Point3D& getLastTilesOffset() { return last_tiles_offset; }

private:
    void addParticleToTile(ImpParticle* pParticle, int x, int y, int z);
    ImpTile* findTile(int x, int y, int z);
    ImpTile* createTile(int x, int y, int z);

    Point3D calcTileCorner(int x, int y, int z);
    Point3D calcTileOffset(int x, int y, int z);

public:
    /* +00 */ SweepRenderer* m_sweepRenderer;
    /* +04 */ SmartArray<ImpTile*, 0, 16> tiles;
    /* +10 */ int maxNoTileToDraw;
    /* +20 */ Point3D last_tiles_offset;
    /* +30 */ Point3D offset;
    /* +40 */ Point3D render_dim;
    /* +50 */ float render_margin;
    /* +60 */ Point3D corner_offset;
};

class ImpTilerFactory
{
public:
    ImpTilerFactory();

    ImpTiler* getTiler();
    void returnTiler(ImpTiler* pTiler);

    static ImpTilerFactory* factory;

private:
    SmartArray<ImpTiler*, 0, 16> m_tilers;
};

extern Point3D tile_debug_color;

float frand();
int trunc(float f);
void drawTile(ImpTile* pTile, SweepRenderer* pRenderer);

class C
{
public:
    static bool gt(ImpTile* a, ImpTile* b);
};

#endif // IMPTILER_H