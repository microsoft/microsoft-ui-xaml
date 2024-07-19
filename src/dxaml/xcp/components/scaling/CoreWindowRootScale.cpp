// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CoreWindowRootScale.h"

#include "CDependencyObject.h"
#include <CoreP.h>
#include <RootVisual.h>
#include <ConnectedAnimationRoot.h>
#include <real.h>
#include <NodeStreamCache.h>
#include <DXamlServices.h>

CoreWindowRootScale::CoreWindowRootScale(RootScaleConfig config, CCoreServices* pCoreServices, VisualTree* pVisualTree)
    : RootScale(config, pCoreServices, pVisualTree)
{
}

_Check_return_ HRESULT
CoreWindowRootScale::ApplyScaleProtected(bool scaleChanged)
{
    const auto mainRootVisual = m_pCoreServices->GetMainRootVisual();
    if (mainRootVisual)
    {
        const float scale = GetRootVisualScale();
        // The composition subsystem has installed a scale transform on top of our tree, update our tree to be aware of it
        // This ensures that Xaml will know about the physical DPI when needed, e.g. rendering crisp text.
        mainRootVisual->SetRasterizationScale(scale);

        const auto connectedAnimationRoot = m_pCoreServices->GetConnectedAnimationRoot();
        if (connectedAnimationRoot)
        {
            // Plateau scale has been applied on the island, and we need to cancel it here,
            // because the snapshots are created using the pixel size including the plateau scale,
            // and we don't want double scaling.
            CValue inverseScaleTransform;
            IFC_RETURN(CreateReverseTransform(&inverseScaleTransform));
            IFC_RETURN(connectedAnimationRoot->SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransform, inverseScaleTransform));
        }
    }

    if (scaleChanged)
    {
        // We need to force re-layout and re-render only if the scale has actually changed.
        m_pCoreServices->MarkRootScaleTransformDirty();

        // Warning: This will not work correctly in the Context of AppWindows
        //     Task 18843113: Do not use global MRT resource manager, instead add a new MRT Resource instance on each CRootVisualInstanc
        {
            const unsigned int scalePercentage = XcpRound(GetEffectiveRasterizationScale() * 100.0f);
            // Update the scale factor on the resource manager.
            xref_ptr<IPALResourceManager> resourceManager;
            IFC_RETURN(m_pCoreServices->GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
            IFC_RETURN(resourceManager->SetScaleFactor(scalePercentage));

            // Flush the XAML parser cache. If the application to reload XAML after a scale change, we want to
            // re-query MRT for the XAML resource, potentially picking up a new resource for the new scale.
            std::shared_ptr<XamlNodeStreamCacheManager> spXamlNodeStreamCacheManager;
            IFC_RETURN(m_pCoreServices->GetXamlNodeStreamCacheManager(spXamlNodeStreamCacheManager));
            spXamlNodeStreamCacheManager->Flush();
        }
    }

    return S_OK;
}
