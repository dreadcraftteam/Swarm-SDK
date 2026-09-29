//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef INFHASHRENDERER_H
#define INFHASHRENDERER_H

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
    /* +00 */ float value;
    /* +04 */ float normal[3];
    /* +10 */ vbId_t edges[3];
    /* +1C */ unsigned char x, y, z;
    /* +1F */ unsigned char flags;//1 = crawled, 2 = corner computed
    /* +20 */ CornerInfo* next;//next in hash
};

typedef void (*CalcCornerFunc_t)(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
typedef bool (*CalcSignFunc_t)(unsigned char, unsigned char, unsigned char, float, float, float, ProjectingParticleCache*);
typedef void (*CalcSign2Func_t)(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
typedef void (*CalcVertexFunc_t)(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);

class InfHashRenderer
{
public:
    InfHashRenderer();

    void beginFrame(bool tile_mode, void* pRenderContext);
    void endFrame();
    void beginTile(ImpTile* pTile);
    void endTile();

    void addParticle(ImpParticle* pParticle);
    static float getCubeWidth();
    static float getCutoffR();
    static float getRenderR();
    static float getMarginWidth();
    static int getMarginNCubes();
    static Point3D& getInnerDimensions();

    bool isParticleWithinBounds(ImpParticle* pParticle);//ALWAYS TRUE?!
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
    static float oneOverCubewidth; // = 1.25f
    static int cornerInfoSize; // = 32
    static float cutoffR; // = 3.3f
    static float renderR; // = 1.3f
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

    static const int xsize = 100;
    static const int ysize = 100;
    static const int zsize = 100;
    static const int xsizeP1 = 101;
    static const int ysizeP1 = 101;
    static const int zsizeP1 = 101;

private:
    bool findCorner(unsigned char x, unsigned char y, unsigned char z, CornerInfo** ppCorner);
    void crawl(unsigned char x, unsigned char y, unsigned char z);
    void docrawl(unsigned char x, unsigned char y, unsigned char z);
    void seed_surface(Point3D& p);

    static void changeCubeWidth(float flWidth);
    static void changeRadii(float flRad0, float flRad1);
    static void recalculateDimensions();
    void recalculateBB();

    /* +000 */ ProjectingParticleCache* pCache;
    /* +004 */ IndexTriVertexBuffer* vertexBuffer;
    /* +008 */ int frameCounter;
    /* +00C */ int m_nSomething0C;
    /* +010 */ long long memoryUsage;
    /* +018 */ int m_nSomething18;
    /* +01C */ int m_nSomething1C;
    /* +020 */ Point3D offset;
    /* +030 */ Point3D bbmins;
    /* +040 */ Point3D bbmaxs;
    /* +050 */ Point3D innerBBMins;
    /* +060 */ Point3D innerBBMaxs;
    /* +070 */ SmartArray<ImpParticle*, 0, 16> particles;
    /* +07C */ SmartArray<CornerInfo*, 0, 16> cornerBuckets;//1000 buckets
    /* +088 */ SmartArray<XYZ, 0, 16> something88;
    /* +094 */ SmartArray<XYZ, 0, 16> something94;
    /* +0A0 */ int nCornerInfoAlloc;
    /* +0A4 */ SmartArray<CornerInfo*, 0, 16> allocatedCornerInfos;
};

#endif // INFHASHRENDERER_H