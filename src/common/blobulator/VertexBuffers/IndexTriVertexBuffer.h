//===== Blobulator. Copyright Ilya D. Rosenberg, All rights reserved. =====//
//
// Purpose:
// 
//=========================================================================//

#ifndef INDEXTRIVERTEXBUFFER_H
#define INDEXTRIVERTEXBUFFER_H

#include "../common/blobulator/Implicit/ImpTiler.h"
#include "materialsystem/imesh.h"

struct vbId_t
{
    uint16 id; // +00 — mesh number
    uint16 offset;// +02 — vertex offset in mesh
};

struct vbId_t;

class IndexTriVertexBuffer
{
public:
    IndexTriVertexBuffer();
    ~IndexTriVertexBuffer();

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
    /* +02 */unsigned short m_vertexBase;
    /* +04 */ IMesh* m_pMesh;
    /* +08 */ CMeshBuilder* m_pMeshBuilder; //max 32k ind

    enum 
    { 
        MAX_VERTS = 16000, 
        MAX_INDICES = 32000 
    };
};

#endif // INDEXTRIVERTEXBUFFER_H
