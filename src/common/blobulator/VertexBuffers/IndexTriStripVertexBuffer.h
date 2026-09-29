//===== Blobulator. Copyright Ilya D. Rosenberg, All rights reserved. =====//
//
// Purpose:
// 
//=========================================================================//

#ifndef INDEXTRIVERTEXBUFFER_H
#define INDEXTRIVERTEXBUFFER_H

#include "../common/blobulator/Implicit/ImpTiler.h"
#include "materialsystem/imesh.h"

#include <cstdint>
#include <cstddef>

struct vbId_t
{
    uint16_t id; // +00 — mesh number
    uint16_t offset; // +02 — vertex offset in mesh
};

class IndexTriStripVertexBuffer
{
public:
    IndexTriStripVertexBuffer();
    ~IndexTriStripVertexBuffer();

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

    /* +00 */ unsigned short m_id;
    /* +02 */ unsigned short m_vertexBase;
    /* +04 */ IMesh* m_pMesh;
    /* +08 */ CMeshBuilder* m_pMeshBuilder;
    /* +0C */ int m_nUnk0C;

    enum
    {
        MAX_VERTS   = 16000,
        MAX_INDICES = 32000
    };
};

#endif // INDEXTRIVERTEXBUFFER_H