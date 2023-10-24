// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PointerRoutedEventArgs.g.h"
#include "PointerPointTransform.h"
#include "Window.g.h"
#include "PointerEventArgs.h"
#include "TransformGroup.g.h"
#include "ScaleTransform.g.h"
#include "RootScale.h"
#include "Popup.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DirectUI::PointerRoutedEventArgs::GetCurrentPointImpl(
    _In_opt_ xaml::IUIElement* pRelativeTo,
    _Outptr_ ixp::IPointerPoint** ppPointerPoint)
{
    HRESULT hr = S_OK;
    ixp::IPointerPoint* pPointerPoint = nullptr;
    mui::IPointerPointTransform* pPointerPointTransform = nullptr;
    xref_ptr<CPointerEventArgs> spArgs;

    // Get PointerPointTransform from the specified pRelativeTo element
    // Right now get_PointerId always returns the same value. Once we move away from using
    // get_PointerId we should revisit this, especialy with regard to windowed popups.
    IFC(PointerPointTransform::CreatePointerPointTransform(pRelativeTo, &pPointerPointTransform));

    // Get PointerPoint from the specified transform point
    spArgs.attach(static_cast<CPointerEventArgs*>(GetCorePeer()));
    IFC(spArgs->m_pPointerPoint->GetTransformedPoint(pPointerPointTransform, &pPointerPoint));

    *ppPointerPoint = pPointerPoint;
    pPointerPoint = NULL;

Cleanup:
    ReleaseInterface(pPointerPoint);
    ctl::release_interface(pPointerPointTransform);

    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::PointerRoutedEventArgs::GetIntermediatePointsImpl(
    _In_opt_ xaml::IUIElement* pRelativeTo,
    _Outptr_ wfc::IVector<ixp::PointerPoint*>** ppPointerPoints)
{
    HRESULT hr = S_OK;
    wfc::IVector<ixp::PointerPoint*>* pPointerPoints = NULL;
    mui::IPointerPointTransform* pPointerPointTransform = nullptr;
    xref_ptr<CPointerEventArgs> spArgs;

    // Get PointerPointTransform from the specified pRelativeTo element
    IFC(PointerPointTransform::CreatePointerPointTransform(pRelativeTo, &pPointerPointTransform));

    // Get PointerPoint from the specified transform point
    spArgs.attach(static_cast<CPointerEventArgs*>(GetCorePeer()));
    IFC(spArgs->m_pPointerEventArgs->GetIntermediateTransformedPoints(pPointerPointTransform, &pPointerPoints));

    BOOLEAN isGenerated;
    IFC(get_IsGenerated(&isGenerated));
    if (isGenerated)
    {
        // We are in a generated event.  That means we are replaying a previous
        // event so we can respond to scene changes.  For replays, we only want
        // our intermediate points to contain the most current point.
        UINT points = 0;
        IFC(pPointerPoints->get_Size(&points));
        if (points > 1)
        {
            ctl::ComPtr<ixp::IPointerPoint> lastPointerPoint;
            IFC(pPointerPoints->GetAt(0, &lastPointerPoint));
            IFC(pPointerPoints->Clear());
            IFC(pPointerPoints->Append(lastPointerPoint.Get()));
        }
    }

    *ppPointerPoints = pPointerPoints;
    pPointerPoints = NULL;

Cleanup:
    ReleaseInterface(pPointerPoints);
    ctl::release_interface(pPointerPointTransform);

    RRETURN(hr);
}