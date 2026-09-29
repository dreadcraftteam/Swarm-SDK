//===== Blobulator. Copyright Ilya D. Rosenberg, All rights reserved. =====//
//
// Purpose:
// 
//=========================================================================//

#ifndef SORTEDINDEXTRIVERTEXBUFFER_H
#define SORTEDINDEXTRIVERTEXBUFFER_H

#include "../common/blobulator/Implicit/ImpTiler.h"
#include "materialsystem/imesh.h"
#include "IndexTriVertexBuffer.h"
#include "../SmartArray.h"
#include <cstdint>
#include <cstddef>

struct vbVertex_t
{
    unsigned char data[80];
};

class SortedIndexTriVertexBuffer
{
public:
    SortedIndexTriVertexBuffer();
    ~SortedIndexTriVertexBuffer();

    void beginFrame(void* p1);
    void endFrame();

    void beginCube(unsigned char p1);
    void endCube();

    void beginTile(ImpTile* p1);
    void endTile();

    void beginVertex(vbId_t* p1);
    void drawVertex(vbId_t const*);
    void endVertex(vbId_t* p1);

    bool isIdValid(vbId_t const* p1);

    static void clearId(vbId_t* id);

    void setVertexPosition(Point3D& p);
    void setVertexNormal(Point3D& p);
    void setVertexTangent(Point3D& p);
    void setVertexColor(Point3D& p);
    void setVertexUV(int stage, float u, float v);
    void setVertexUV(int stage, float u, float v, float w, float q);

private:
    void beginMesh();
    void endMesh();

    /* +000 */ IMesh* m_pMesh;
    /* +004 */ SmartArray<vbVertex_t, 0, 16> m_vertices;
    /* +010 */ SmartArray<unsigned int, 0, 16> m_buckets[100];
    /* +4C0 */ int m_nUnk4C0; 
    /* +4C4 */ int m_nUnk4C4; 
    /* +4C8 */ int m_nUnk4C8; 

    enum
    {
        MAX_VERTS   = 16000,
        MAX_INDICES = 32000,
        NUM_BUCKETS = 100
    };
};

#endif // SORTEDINDEXTRIVERTEXBUFFER_H