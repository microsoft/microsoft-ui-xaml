// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DirectManipulationServiceSharedState.h"
#include "LoadLibraryAbs.h"
#include <Microsoft.DirectManipulation.h>
#include "DirectManipulationHelper.h"

DirectManipulationServiceSharedState::DirectManipulationServiceSharedState()
    : m_compositorUseCount(0)
{
}

DirectManipulationServiceSharedState::~DirectManipulationServiceSharedState()
{
    ASSERT(m_compositorUseCount == 0);
}

HRESULT DirectManipulationServiceSharedState::GetSharedDCompManipulationCompositor(IDirectManipulationCompositor **ppResult)
{
    if (!m_compositor)
    {
        HMODULE hmodDManip = LoadLibraryExWAbs(L"Microsoft.DirectManipulation.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        IFCW32_RETURN(hmodDManip != nullptr);

        wrl::ComPtr<IClassFactory> directManipulationFactory;
        IFC_RETURN(DirectManipulationHelper::GetDCompManipulationCompositorFactory(hmodDManip, &directManipulationFactory));
        
        IFC_RETURN(directManipulationFactory->CreateInstance(nullptr, IID_PPV_ARGS(m_compositor.ReleaseAndGetAddressOf())))
    }
    m_compositorUseCount++;
    m_compositor.CopyTo(ppResult);
    return S_OK;
}

void DirectManipulationServiceSharedState::ReleaseSharedDCompManipulationCompositor(IDirectManipulationCompositor *&compositor)
{
    // This function releases all compositors including ones obtained before
    // reset so only decrement the use count when releasing the current one.
    if (compositor && m_compositor.Get() == compositor)
    {
        ASSERT(m_compositorUseCount > 0);
        if (--m_compositorUseCount == 0)
        {
            m_compositor.Reset();
        }
    }
    ReleaseInterface(compositor);
}

void DirectManipulationServiceSharedState::ResetSharedDCompManipulationCompositor()
{
    m_compositor.Reset();
    m_compositorUseCount = 0;
}
