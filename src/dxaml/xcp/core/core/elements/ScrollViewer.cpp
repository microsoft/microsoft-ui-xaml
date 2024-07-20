// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollViewer.h"
#include "ManipulationTransform.h"
#include <DCompTreeHost.h>
#include <WindowsGraphicsDeviceManager.h>

CScrollViewer::CScrollViewer(_In_ CCoreServices *pCore)
    : CScrollContentControl(pCore)
    , m_viewportWidth()
    , m_scrollableWidth()
    , m_extentWidth()
    , m_viewportHeight()
    , m_scrollableHeight()
    , m_extentHeight()
    , m_sharedPrimaryContentTransformOwner(nullptr)
{
    SetIsCustomType();
}

CScrollViewer::~CScrollViewer()
{
}

_Check_return_ HRESULT CScrollViewer::EnsureManipulationTransformPropertySet(
    _Outptr_result_nullonfailure_ WUComp::ICompositionPropertySet **result)
{
    if (!m_manipulationTransformPropertySet)
    {
        *result = nullptr;

        // Create manipulation transform property set
        Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> manipulationPropertySet;
        Microsoft::WRL::ComPtr<WUComp::ICompositor> compositor;
        WindowsGraphicsDeviceManager *graphicsDeviceManager = GetContext()->GetBrowserHost()->GetGraphicsDeviceManager();
        IFCEXPECT_RETURN(graphicsDeviceManager);
        IFC_RETURN(graphicsDeviceManager->EnsureDCompDevice());
        compositor = graphicsDeviceManager->GetDCompTreeHost()->GetCompositor();
        ASSERT(compositor != nullptr);
        IFC_RETURN(::CreateManipulationTransformPropertySet(compositor.Get(), &manipulationPropertySet));

        // Connect the property set to the Content's legacy DManip transform.
        CInputServices *inputServices = GetContext()->GetInputServices();
        if (CUIElement *manipulatedElement = inputServices ? inputServices->GetPrimaryContentManipulatedElement(this) : nullptr)
        {
            IFC_RETURN(UpdateManipulationTransformPropertySet(manipulationPropertySet.Get(), manipulatedElement));
        }

        m_manipulationTransformPropertySet = manipulationPropertySet.Get();
    }

    IFC_RETURN(m_manipulationTransformPropertySet.CopyTo(result));

    return S_OK;
}

_Check_return_ HRESULT CScrollViewer::OnSharedContentTransformChanged(_In_ CUIElement *contentElement)
{
    if (m_manipulationTransformPropertySet)
    {
        IFC_RETURN(UpdateManipulationTransformPropertySet(m_manipulationTransformPropertySet.Get(), contentElement));
    }
    return S_OK;
}

_Check_return_ HRESULT CScrollViewer::OnContentOffsetChanged(_In_ CUIElement *contentElement)
{
    GetContext()->RequestReplayPreviousPointerUpdate();

    if (m_manipulationTransformPropertySet)
    {
        IFC_RETURN(UpdateManipulationTransformPropertySet(m_manipulationTransformPropertySet.Get(), contentElement));
    }
    return S_OK;
}

_Check_return_ HRESULT CScrollViewer::UpdateManipulationTransformPropertySet(
    _In_ WUComp::ICompositionPropertySet *manipulationTransformPropertySet,
    _In_ CUIElement *contentElement
    )
{
    IUnknown *sharedPrimaryContentTransform = contentElement->GetSharedPrimaryContentTransform();

    ctl::ComPtr<WUComp::ICompositionObject> manipulationTransformCO;
    if (sharedPrimaryContentTransform != nullptr)
    {
        IFC_RETURN(::ConnectManipulationPropertySetToTransform(
            manipulationTransformPropertySet,
            sharedPrimaryContentTransform,
            contentElement->GetDirectManipulationContentOffsetX(),
            contentElement->GetDirectManipulationContentOffsetY(),
            &manipulationTransformCO
            ));
    }
    else if (m_sharedPrimaryContentTransformOwner == contentElement)
    {
        // Only disconnect if contentElement is the owner. This way we allow the previous owner to be released after the new one takes up the post.
        IFC_RETURN(::DisconnectManipulationPropertySet(manipulationTransformPropertySet));
    }

    m_sharedPrimaryContentTransformOwner = sharedPrimaryContentTransform ? contentElement : nullptr;
    m_manipulationTransformCO = manipulationTransformCO;

    return S_OK;
}
