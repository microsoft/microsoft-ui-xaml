// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PointerPointTransform.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace xaml_media;

PointerPointTransform::PointerPointTransform()
    : m_pTransform(nullptr),
      m_isInverse(false)
{
    m_windowTranslation.X = 0;
    m_windowTranslation.Y = 0;
}

PointerPointTransform::~PointerPointTransform()
{
    ReleaseInterface(m_pTransform);
}

//--------------------------------- ----------------------------------------
//
//  Function:   get_Inverse
//
//  Synopsis:   Return the inversed pointer point transform
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
PointerPointTransform::get_Inverse(
    _Outptr_ mui::IPointerPointTransform **ppPointerPointTransform)
{
    HRESULT hr = S_OK;
    PointerPointTransform* pPointerPointTransform = NULL;
    IGeneralTransform* pInvertedTransform = NULL;

    IFCPTR(ppPointerPointTransform);

    IFC(m_pTransform->get_Inverse(&pInvertedTransform));
    IFC(ctl::ComObject<PointerPointTransform>::CreateInstance(&pPointerPointTransform));

    IFC(pPointerPointTransform->SetTransform(
        pInvertedTransform,
        &m_windowTranslation,
        !m_isInverse));

    *ppPointerPointTransform = pPointerPointTransform;
    pPointerPointTransform = NULL;

Cleanup:
    ctl::release_interface(pPointerPointTransform);
    ReleaseInterface(pInvertedTransform);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   TryTransform
//
//  Synopsis:   Tranform the windows coordinated point to Xaml based element
//              relative point.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
PointerPointTransform::TryTransform(
    _In_ wf::Point inPoint,
    _Out_ wf::Point *pOutPoint,
    _Out_ BOOLEAN *pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pOutPoint);
    IFCPTR(pReturnValue);

    IFCEXPECT(m_pTransform);

    // Apply optional translation from pointer's target window to Jupiter window
    if (!m_isInverse)
    {
        inPoint.X += m_windowTranslation.X;
        inPoint.Y += m_windowTranslation.Y;
    }

    IFC(m_pTransform->TryTransform(inPoint, pOutPoint, pReturnValue));

    // Apply optional inverse translation from Jupiter window to pointer's target window
    if (*pReturnValue && m_isInverse)
    {
        pOutPoint->X -= m_windowTranslation.X;
        pOutPoint->Y -= m_windowTranslation.Y;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   TransformBounds
//
//  Synopsis:   Tranform the rect bounds from the windows coordinated point
//              to Xaml based element relative point rect bounds.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
PointerPointTransform::TryTransformBounds(
    _In_ wf::Rect inRect,
    _Out_ wf::Rect *pOutRect,
    _Out_ BOOLEAN *pReturnValue)
{
    HRESULT hr = S_OK;
    *pReturnValue = FALSE;

    IFCPTR(pOutRect);

    // Apply optional translation from pointer's target window to Jupiter window
    if (!m_isInverse)
    {
        inRect.X += m_windowTranslation.X;
        inRect.Y += m_windowTranslation.Y;
    }

    IFC(m_pTransform->TransformBounds(inRect, pOutRect));

    // Apply optional inverse translation from Jupiter window to pointer's target window
    if (m_isInverse)
    {
        pOutRect->X -= m_windowTranslation.X;
        pOutRect->Y -= m_windowTranslation.Y;
    }

    *pReturnValue = TRUE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PointerPointTransform::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(mui::IPointerPointTransform)) ||
        InlineIsEqualGUID(iid, IID_IUnknown)) // HIP GestureRecognizer assumed IUnknown object as IPointerPointTransform
    {
        *ppObject = static_cast<mui::IPointerPointTransform*>(this);
    }
    else
    {
        return ComBase::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

_Check_return_ HRESULT
PointerPointTransform::SetTransform(
    _In_ IGeneralTransform* pTransform,
    _In_ wf::Point *pWindowTranslation,
    _In_ bool isInverse)
{
    HRESULT hr = S_OK;

    IFCEXPECT(pTransform);

    ReleaseInterface(m_pTransform);
    m_pTransform = pTransform;
    AddRefInterface(m_pTransform);

    m_windowTranslation = *pWindowTranslation;
    m_isInverse = isInverse;

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
PointerPointTransform::CreatePointerPointTransform(
    _In_opt_ xaml::IUIElement* relativeTo,
    _Outptr_ mui::IPointerPointTransform** returnedPointerPointTransform)
{
    IFCPTR_RETURN(returnedPointerPointTransform);
    *returnedPointerPointTransform = nullptr;
    auto dxamlCore = DXamlCore::GetCurrent();
    auto coreServices = dxamlCore->GetHandle();

    CUIElement* relativeElement = relativeTo && static_cast<UIElement*>(relativeTo)->IsInLiveTree() ?
        (static_cast<UIElement*>(relativeTo))->GetHandle() : nullptr;

    // Get the transform  from the hidden root visual to the relative element.
    xref_ptr<CGeneralTransform> transformNative;
    IFC_RETURN(CoreImports::RootVisual_TransformToVisual(
        coreServices,
        relativeElement,
        transformNative.ReleaseAndGetAddressOf()));

    ctl::ComPtr<DependencyObject> transformPeer;
    IFC_RETURN(dxamlCore->GetPeer(transformNative.get(), &transformPeer));
    ctl::ComPtr<IGeneralTransform> transform;
    IFC_RETURN(transformPeer.As(&transform));

    // Create PointerPointTransform that base on the relativeTo element
    ctl::ComPtr<PointerPointTransform> pointerPointTransform;
    IFC_RETURN(ctl::make<PointerPointTransform>(&pointerPointTransform));

    // Set the relativeTo transform
    // Note: use an explicit zero here for the "windowTranslation". We already have a PointerPoint relative to the root
    // of a Xaml tree. We can transform it down to the specified element just by accounting for offsets applied by Xaml
    // (e.g. layout offsets, RenderTransforms). The fact that windowed popup hwnds have an offset is an implementation
    // detail; the same offset is already counted as part of the Xaml Popup.HorizontalOffset/VerticalOffset or a layout
    // offset.
    wf::Point zero = {};
    IFC_RETURN(pointerPointTransform->SetTransform(
        transform.Get(),
        &zero,
        false /* isInverse */));
    IFC_RETURN(pointerPointTransform.CopyTo(returnedPointerPointTransform));

    return S_OK;
}