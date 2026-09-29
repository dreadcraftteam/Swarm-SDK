//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef USERFUNCTIONS_H
#define USERFUNCTIONS_H

struct CornerInfo;
class IndexTriVertexBuffer;
class ProjectingParticleCache;
struct vbId_t;

// CalcSign Functions
bool calcSign(unsigned char, unsigned char, unsigned char, float, float, float, ProjectingParticleCache*);
bool calcSignSIMD(unsigned char, unsigned char, unsigned char, float, float, float, ProjectingParticleCache*);
void calcSign2(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);

// CalcConer Functions
void calcCornerNormal(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
void calcCornerNormalColorUVTan(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
void calcCornerNormalColor(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
void calcCornerNormalColorTanNoUV(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
void calcCornerNormalHiFreqColor(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);
void calcCornerNormalColorUVTanForBlob(unsigned char, unsigned char, unsigned char, float, float, float, CornerInfo* const, ProjectingParticleCache*);

// CalcVertex Functions
void calcVertexNormal(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);
void calcVertexNormalDebugColor(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);
void calcVertexNormalNColorUVTan(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);
void calcVertexNormalNColor(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);
void calcVertexNormalNColorTanNoUV(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);
void calcVertexNormalNTexCoord4(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);
//void calcVertexNormalNColorUVTanForBlob(float, float, float, int, CornerInfo const*, CornerInfo const*, IndexTriVertexBuffer*);

#endif // USERFUNCTIONS_H
