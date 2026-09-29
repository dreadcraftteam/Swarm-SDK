//========== Copyright Ilya D. Rosenberg, All rights reserved. ============//
//
// Purpose:
// 
//=========================================================================//

#ifndef IMPRENDERER_H
#define IMPRENDERER_H

#include "SweepRenderer.h"
#include "../SmartArray.h"

class ImpRendererFactory
{
public:
    SweepRenderer* getRenderer();
    void returnRenderer(SweepRenderer* pRenderer);

    static ImpRendererFactory* factory;

private:
    ImpRendererFactory();

    SmartArray<SweepRenderer*, 0, 16> m_renderers;
    friend struct ImpRendererFactoryInit;
};

#endif