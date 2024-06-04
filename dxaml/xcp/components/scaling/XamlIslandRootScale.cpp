// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlIslandRootScale.h"
#include "VisualTree.h"
#include "UIElement.h"
#include "ConnectedAnimationRoot.h"
#include "MatrixTransform.h"
#include "Matrix.h"
#include "CMatrix.h"
#include <CoreP.h>
#include "real.h"
#include <NodeStreamCache.h>

#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>
#include <microsoft.ui.composition.internal.h>

XamlIslandRootScale::XamlIslandRootScale(
    CCoreServices* pCoreServices,
    VisualTree* pVisualTree)
    : RootScale(RootScaleConfig::ParentApply, pCoreServices, pVisualTree)
{
}

_Check_return_ HRESULT
XamlIslandRootScale::ApplyScaleProtected(bool scaleChanged)
{
    auto visualTree = GetVisualTreeNoRef();
    auto rootElement = visualTree->GetRootElementNoRef();
    if (rootElement && scaleChanged)
    {
        rootElement->SetEntireSubtreeDirty();
    }

    const auto connectedAnimationRoot = visualTree->GetConnectedAnimationRoot();
    if (connectedAnimationRoot)
    {
        // Plateau scale has been applied on the content, and we need to cancel it here,
        // because the snapshots are created using the pixel size including the plateau scale,
        // and we don't want double scaling.
        CValue inverseScaleTransform;
        const float scale = 1.0f / GetSystemScale();
        CREATEPARAMETERS cp(m_pCoreServices);
        CValue value;
        {
            xref_ptr<CDependencyObject> matrix;
            IFC_RETURN(CMatrix::Create(matrix.ReleaseAndGetAddressOf(), &cp));
            value.SetFloat(scale);
            IFC_RETURN(matrix.get()->SetValueByKnownIndex(KnownPropertyIndex::Matrix_M11, value));
            IFC_RETURN(matrix.get()->SetValueByKnownIndex(KnownPropertyIndex::Matrix_M22, value));
            {
                xref_ptr<CDependencyObject> matrixTransform;
                IFC_RETURN(CMatrixTransform::Create(matrixTransform.ReleaseAndGetAddressOf(), &cp));
                value.WrapObjectNoRef(matrix.get());
                IFC_RETURN(matrixTransform.get()->SetValueByKnownIndex(KnownPropertyIndex::MatrixTransform_Matrix, value));
                inverseScaleTransform.SetObjectAddRef(matrixTransform.get());
            }
        }

        IFC_RETURN(connectedAnimationRoot->SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransform, inverseScaleTransform));
    }

    if (scaleChanged)
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

    return S_OK;
}
