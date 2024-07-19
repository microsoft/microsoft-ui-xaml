// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "RootScale.h"

#include <CoreP.h>
#include <DisplayListener.h>
#include <ContentRoot.h>
#include <DXamlServices.h>

RootScale::RootScale(RootScaleConfig config, CCoreServices* pCoreServices, VisualTree* visualTree)
    : m_config(config)
    , m_pVisualTree(visualTree)
    , m_pCoreServices(pCoreServices)
{
}

RootScale::~RootScale()
{
    // It's OK to still have some displayListeners at this point.  When a test is shutting down XAML, we'll
    // destroy this object, but we may still have a CLoadedImageSurface object in this list.  Since we're shutting
    // down, there won't be new scale changes anyway.
    m_displayListeners.clear();
    m_imageReloadManager.ClearImages();
}

float RootScale::GetSystemScale()
{
    float rasterizationScale = 1.0f;
    if (m_content)
    {
        // For CoreWindow scenarios, the CompositionContent is also listening for the CoreWindow's closed event.
        // CompositionContent will get the notification first and close the entire visual tree, then Xaml will
        // exit its message loop and tear down the tree. Since CompositionContent already closed everything,
        // Xaml will get lots of RO_E_CLOSED errors. These are all safe to ignore. So tolerate RO_E_CLOSED if
        // we're also in the middle of tearing down the tree.
        HRESULT hr = m_content->get_RasterizationScale(&rasterizationScale);
        if (FAILED(hr))
        {
            if (hr != RO_E_CLOSED)
            {
                IFCFAILFAST(hr);
            }
        }
    }
    return rasterizationScale;
}

void RootScale::SetContentIsland(_In_opt_ ixp::IContentIsland * content)
{
    m_content = content;
}

_Check_return_ HRESULT RootScale::UpdateSystemScale()
{
    // Remove SuspendFailFastOnStowedException
    //     Bug 19696972: QueryScaleFactor silently fails at statup
    SuspendFailFastOnStowedException raiiSuspender;
    const auto systemScale = GetSystemScale();
    if (systemScale!=0.0f)
    {
        IFC_RETURN(SetSystemScale(systemScale));
    }
    return S_OK;
}

float RootScale::GetEffectiveRasterizationScale()
{
    if (!IsInitialized() && !m_bUpdating)
    {
        VERIFYHR(UpdateSystemScale());
    }

    // A testOverrideScale of 0 means there's no override; just use the systemScale
    const float effectiveScale = m_testOverrideScale == 0.0f ? m_systemScale : m_testOverrideScale;
    return effectiveScale;
}

float RootScale::GetRootVisualScale()
{
    // In XamlOneCoreTransforms mode, there is no need to do a RenderTransform on the root, because the scale has already been
    // applied for us by the CompositionIsland. However, due to legacy reasons, our DComp tests has a dependency that, even when the scale is 1,
    // a RenderTransform is still applied on the root (Identity). To support these tests, we will always apply a scale transform on the root
    // in XamlOneCoreTransforms mode. When we've enabled XamlOneCoreTransforms mode by default, we can break this dependency and
    // update the tests to not expect an Identity RenderTransform set on the root.
    // In OneCoreTransforms mode, there's already a scale applied to XAML visuals matching the systemScale, so we factor that scale
    // out on the XAML content.
    float newRootVisualScale = 0.0f;
    const float effectiveScale = GetEffectiveRasterizationScale();
    if (m_config == RootScaleConfig::ParentApply)
    {
        // This is the case where we're pushing a non-identity scale into the root visual
        newRootVisualScale = effectiveScale / m_systemScale;
    }
    else
    {
        newRootVisualScale = effectiveScale;
    }
    return newRootVisualScale;
}

_Check_return_ HRESULT
RootScale::SetTestOverride(float scale)
{
    IFC_RETURN(SetScale(scale, RootScale::ScaleKind::Test));
    return S_OK;
}

_Check_return_ HRESULT
RootScale::SetSystemScale(float scale)
{
    IFC_RETURN(SetScale(scale, RootScale::ScaleKind::System));
    return S_OK;
}

_Check_return_ HRESULT
RootScale::SetScale(float scale, RootScale::ScaleKind kind)
{
    m_bUpdating = true;
    auto cleanup = wil::scope_exit([&]()
    {
        m_bUpdating = false;
    });

    const float oldScale = GetEffectiveRasterizationScale();
    const bool scaleIsValid = scale!=0.0f;
    switch (kind)
    {
        case RootScale::ScaleKind::System:
            if (scaleIsValid)
            {
                m_systemScale = scale;
            }
            break;
        case RootScale::ScaleKind::Test:
            m_testOverrideScale = scale;
            break;
    }
    const float newScale = GetEffectiveRasterizationScale();
    const bool scaleChanged = !IsCloseReal(oldScale, newScale);
    IFC_RETURN(ApplyScale(scaleChanged));
    m_bInitialized = true;
    return S_OK;
}

_Check_return_ HRESULT
RootScale::ApplyScale()
{
    IFC_RETURN(ApplyScale(false));
    return S_OK;
}

_Check_return_ HRESULT
RootScale::ApplyScale(bool scaleChanged)
{
    IFC_RETURN(ApplyScaleProtected(scaleChanged));

    if (scaleChanged)
    {
        for (auto displayListener : m_displayListeners)
        {
            IFC_RETURN(displayListener->OnScaleChanged());
        }

        if (IsInitialized())
        {
            // Reload images.
            IFC_RETURN(m_imageReloadManager.ReloadImages(ResourceInvalidationReason::ScaleChanged));
        }

        m_pVisualTree->GetContentRootNoRef()->AddPendingXamlRootChangedEvent(CContentRoot::ChangeType::RasterizationScale);
    }
    return S_OK;
}

void RootScale::AddDisplayListener(_In_ DisplayListener *displayListener)
{
    ASSERT(m_displayListeners.count(displayListener) == 0);
    m_displayListeners.insert(displayListener);
}

void RootScale::RemoveDisplayListener(_In_ DisplayListener *displayListener)
{
    ASSERT(m_displayListeners.count(displayListener) == 1);
    m_displayListeners.erase(displayListener);
}

CImageReloadManager& RootScale::GetImageReloadManager()
{
    return m_imageReloadManager;
}

RootScale* RootScale::GetRootScaleForElement(_In_ CDependencyObject* pDO)
{
    if (auto contentRoot = VisualTree::GetContentRootForElement(pDO))
    {
        auto rootScale = GetRootScaleForContentRoot(contentRoot);
        return rootScale;
    }
    return nullptr;
}

RootScale* RootScale::GetRootScaleForContentRoot(_In_opt_ CContentRoot* contentRoot)
{
    if (contentRoot)
    {
        if (auto visualTree = contentRoot->GetVisualTreeNoRef())
        {
            auto rootScale = visualTree->GetRootScale();
            return rootScale;
        }
    }
    return nullptr;
}

RootScale* RootScale::GetRootScaleForElementWithFallback(_In_opt_ CDependencyObject* pDO)
{
    RootScale* result = nullptr;
    if(pDO)
    {
        result = GetRootScaleForElement(pDO);
    }

    if (result == nullptr)
    {
        const auto coreServices = DirectUI::DXamlServices::GetHandle();
        const auto contentRootCoordinator = coreServices->GetContentRootCoordinator();
        if (const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot())
        {
            result = GetRootScaleForContentRoot(root);
        }
    }

    return result;
}
