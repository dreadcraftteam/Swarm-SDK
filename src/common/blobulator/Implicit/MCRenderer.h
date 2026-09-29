//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef MCRENDERER_H
#define MCRENDERER_H

#include "ImpTiler.h"
#include "ImpParticle.h"
#include "../Point3D.h"
#include "../VertexBuffers/IndexTriVertexBuffer.h"
#include "ProjectingParticleCache.h"

struct XYZ
{
    int x, y, z;
};

struct CubeInfo
{
    unsigned short cornerInfoNo;
    bool           doneAbove;
    bool           doneBelow;
    unsigned int   everything;
};

struct CornerInfo
{
    float    value;
    float    normal[3];
    vbId_t   edges[3];
    unsigned char x, y;
    unsigned char pad1, pad2;
};

typedef void (*CalcCornerFunc_t)(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
typedef bool (*CalcSignFunc_t)(unsigned char, unsigned char, unsigned char, float, float, float, ProjectingParticleCache*);
typedef void (*CalcSign2Func_t)(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
typedef void (*CalcVertexFunc_t)(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);

class MCRenderer
{
public:
    MCRenderer();

    void beginFrame(bool tile_mode, void* pRenderContext);
    void endFrame();
    void beginTile(ImpTile* pTile);
    void endTile();

    void addParticle(ImpParticle* pParticle, bool p2 = false);
    static float getCubeWidth();
    static float getCutoffR();
    static float getRenderR();
    static float getMarginWidth();
    static int getMarginNCubes();
    static Point3D& getInnerDimensions();

    bool isParticleWithinBounds(ImpParticle* pParticle);

    static void setCalcCornerFunc(int p1, CalcCornerFunc_t pFunc);
    static void setCalcCornerFunc(CalcCornerFunc_t pFunc) { setCalcCornerFunc(32, pFunc); }
    static void setCalcSignFunc(CalcSignFunc_t pFunc);
    static void setCalcSign2Func(CalcSign2Func_t pFunc);
    static void setCalcVertexFunc(CalcVertexFunc_t pFunc);
    static void setCubeWidth(float flWidth);
    static void setCutoffR(float flRad);
    static void setRenderR(float flRad);

    void setOffset(Point3D& offset);

    static float cubewidth; // = 0.8f
    static float oneOverCubewidth;// = 1.25f
    static int  cornerInfoSize; // = 32
    static float cutoffR;// = 3.3f
    static float renderR;// = 1.3f
    static float addAmt; // = 0.25f

    static float cutoffRSq;
    static float scaledRenderRSq;
    static float scalerSq;
    static float oneOverThreshold;
    static float scaler;
    static short fieldCalcSteps;
    static float threshold;

    static bool tile_mode;
    static int marginNCubes;
    static float marginWidth;

    static Point3D outerDimensions;
    static Point3D innerDimensions;

private:
    void crawl(unsigned char x, unsigned char y, unsigned char z);
    void docrawl(unsigned char x, unsigned char y, unsigned char z);
    void seed_surface(Point3D& p);

    static void changeCubeWidth(float flWidth);
    static void changeRadii(float flRad0, float flRad1);

    static void recalculateDimensions();
    void recalculateBB();

    static const int xsize    = 100;
    static const int ysize    = 100;
    static const int zsize    = 100;
    static const int xsizeP1  = 101;
    static const int ysizeP1  = 101;
    static const int zsizeP1  = 101;

    /* +000 */ ProjectingParticleCache* pCache;
    /* +004 */ IndexTriVertexBuffer* vertexBuffer;
    /* +008 */ int                      frameCounter; // = 0
    /* +00C */ int                      m_nSomething0C; // = 0
    /* +010 */ long long                memoryUsage;
    /* +018 */ int                      m_nSomething18;// = 0
    /* +01C */ int                      m_nSomething1C;// = 0
    /* +020 */ unsigned short           cellId; // = 0
    /* +022 */ unsigned short           _pad22;
    /* +024 */ int                      _pad24;
    /* +028 */ int                      _pad28;
    /* +02C */ int                      _pad2C;
    /* +030 */ Point3D                  offset;
    /* +040 */ Point3D                  outerBBMins;
    /* +050 */ Point3D                  outerBBMaxs;
    /* +060 */ Point3D                  innerBBMins;
    /* +070 */ Point3D                  innerBBMaxs;
    /* +080 */ SmartArray<ImpParticle*, 0, 16> particles;
    /* +08C */ void*                    cells;// malloc(0x7DC4E8)
    /* +090 */ SmartArray<XYZ, 0, 16>   something90;
    /* +09C */ SmartArray<XYZ, 0, 16>   something9C;
    /* +0A8 */ void*                    pRecursionStack;// malloc(0x213)
    /* +0AC */ int                      nCornerInfoAlloc;// = 0
    /* +0B0 */ int                      nCornerInfoCapacity;// = 512
};

#endif // MCRENDERER_H