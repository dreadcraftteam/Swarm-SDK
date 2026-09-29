//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef PHYSPARTICLE_H
#define PHYSPARTICLE_H

#include "../Point3D.h"

class PhysParticle 
{
public:
	PhysParticle();

	Point3D center;
	float radius;
	int temp1;
	
	Point3D force;

	short group;
	short neighbor_count;
};

class PhysParticleAndDist 
{
public:
	PhysParticle* particle;
	float distSq;
};

#endif // PHYSPARTICLE_H
