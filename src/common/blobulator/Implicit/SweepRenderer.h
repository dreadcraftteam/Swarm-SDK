//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef SWEEPRENDERER_H
#define SWEEPRENDERER_H

#include "ImpTiler.h"
#include "ImpParticle.h"
#include "../Point3D.h"
#include "../VertexBuffers/IndexTriVertexBuffer.h"
#include "ProjectingParticleCache.h"

struct CubeInfo
{
    unsigned short cornerInfoNo;
    bool           doneAbove;
    bool           doneBelow;
    unsigned int   everything;
};

struct YZ
{
    unsigned char y, z;
};

struct Slice_t
{
    CubeInfo(*corners)[101];
    SmartArray<unsigned char, 1, 0> corner_info;
    SmartArray<YZ, 0, 0> todo_list;
    SmartArray<YZ, 0, 16> seed_list;
};

struct CornerInfo
{
    float    value;
    float    normal[3];
    vbId_t   edges[3];
    unsigned char x, y;
    unsigned char pad1, pad2;
};

// func pointer types
typedef void (*CalcCornerFunc_t)(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
typedef bool (*CalcSignFunc_t)(unsigned char, unsigned char, unsigned char, float, float, float, ProjectingParticleCache*);
typedef void (*CalcSign2Func_t)(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
typedef void (*CalcVertexFunc_t)(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);


#define CALC_CORNER_NORMAL_COLOR_UV_TAN_CI_SIZE 32
#define CALC_CORNER_NORMAL_COLOR_CI_SIZE 32
#define CALC_CORNER_NORMAL_CI_SIZE 32

class SweepRenderer
{
public:
    SweepRenderer();

    void beginFrame(bool tile_mode, void* pRenderContext);
    void endFrame();
    void beginTile(ImpTile* pTile);
    void endTile();

    void addParticle(ImpParticle* pParticle, bool p2 = false);

    static float   getCubeWidth();
    static float   getCutoffR();
    static float   getRenderR();
    static float   getMarginWidth();
    static int     getMarginNCubes();
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

    void setMaxNoSlicesToDraw(int maxno) { maxNoSlicesToDraw = maxno; }
    float getLastSliceDrawn() { return last_slice_drawn; }

    static float    cubewidth; // = 0.8f
    static float    oneOverCubewidth;// = 1.25f
    static int      cornerInfoSize;
    static float    cutoffR;// = 3.3f
    static float    renderR; // = 1.3f
    static float    addAmt; // = 0.25f

    static float    cutoffRSq;
    static float    scaledRenderRSq;
    static float    scalerSq;
    static float    oneOverThreshold;
    static float    scaler;
    static short    fieldCalcSteps;
    static float    threshold;

    static bool     tile_mode;
    static int      marginNCubes;
    static float    marginWidth;

    static Point3D  outerDimensions;
    static Point3D  innerDimensions;

private:
    void allocSliceCorners(Slice_t* pSlice);
    void allocSliceTodoList(Slice_t* pSlice);
    void deallocSliceCorners(Slice_t* pSlice);
    void deallocSliceTodoList(Slice_t* pSlice);

    static void changeCubeWidth(float flWidth);
    static void changeRadii(float flRad0, float flRad1);

    static void recalculateDimensions();
    void recalculateBB();

    void render_slice(unsigned char p1, Slice_t* p2, Slice_t* p3, Slice_t* p4);
    void render_slices();

    void seed_surface(Point3D& p1);

    /* +00 */ ProjectingParticleCache* pCache;
    /* +04 */ IndexTriVertexBuffer* vertexBuffer;
    /* +08 */ int                      maxNoSlicesToDraw;
    /* +0C */ int                      last_slice_drawn;
    /* +10 */ bool                     polygonizationEnabled;
    /* +20 */ Point3D                  offset;
    /* +30 */ Point3D                  outerBBMins;
    /* +40 */ Point3D                  outerBBMaxs;
    /* +50 */ Point3D                  innerBBMins;
    /* +60 */ Point3D                  innerBBMaxs;
    /* +70 */ SmartArray<ImpParticle*, 0, 16> particles;
    /* +7C */ Slice_t                  slices[102];// stride 0x28
    /* +106C */ SmartArray<CubeInfo(*)[101], 0, 16>              unused_slice_corners;
    /* +1078 */ SmartArray<SmartArray<unsigned char, 1, 0>, 0, 16> unused_corner_info;
    /* +1084 */ SmartArray<SmartArray<YZ, 0, 0>, 0, 16>          unused_todo_lists;
    /* +1090 */ int                      n_allocated_slice_corners;
    /* +1094 */ int                      n_allocated_slice_corner_infos;
    /* +1098 */ int                      n_allocated_slice_todo_lists;
    /* +109C */ SmartArray<unsigned char, 0, 16> recursion_stack_src;
    /* +10A8 */
};

#endif // SWEEPRENDERER_H