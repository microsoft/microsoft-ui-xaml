// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "JupiterWindow.h"
#include "PointerPointTransform.h"
#include "GeneralTransform.h"
#include "InputSiteAdapter.h"
#include "WindowedPopupInputSiteAdapter.h"

void WindowedPopupInputSiteAdapter::Initialize(_In_ CPopup* popup, _In_ ixp::IContentIsland* contentIsland, _In_ CContentRoot* contentRoot, _In_ CJupiterWindow* jupiterWindow)
{
    m_windowedPopupNoRef = popup;
    __super::Initialize(contentIsland, contentRoot, jupiterWindow);
    IFCFAILFAST(ctl::make<DirectUI::PointerPointTransform>(&m_pointerPointTransformFromContentRoot));

    // Windowed popups do not ever take activation.
    IFCFAILFAST(m_inputPointerSource2->put_ActivationBehavior(ixp::InputPointerActivationBehavior_NoActivate));
}

_Check_return_ HRESULT WindowedPopupInputSiteAdapter::OnDirectManipulationHitTest(_In_ ixp::IPointerEventArgs* args)
{
    wrl::ComPtr<ixp::IPointerPoint> transformedPointerPoint;
    IFC_RETURN(GetTransformedPointerPoint(args, &transformedPointerPoint));

    bool handled = false;
    HRESULT hr = m_jupiterWindow->OnIslandDirectManipulationHitTest(GetContentRoot(), transformedPointerPoint.Get(), &handled);
    IFCFAILFAST(args->put_Handled(handled));
    IFC_RETURN(hr);

    return S_OK;
}

_Check_return_ HRESULT WindowedPopupInputSiteAdapter::SetTransformFromContentRoot(_In_ xaml_media::IGeneralTransform* transform, _In_ wf::Point* offset)
{
    FAIL_FAST_ASSERT(m_pointerPointTransformFromContentRoot != nullptr);

    IFC_RETURN(m_pointerPointTransformFromContentRoot->SetTransform(
        transform,
        offset,
        false /* isInverse */));

    return S_OK;
}

bool WindowedPopupInputSiteAdapter::ReplayPointerUpdate()
{
    ixp::IPointerPoint* previousPointerPoint = m_previousPointerPoint.Get();
    ixp::IPointerEventArgs* previousPointerEventArgs = m_previousPointerEventArgs.Get();

    if (previousPointerPoint != nullptr)
    {
        bool handled_dontCare;
        IFCFAILFAST(m_jupiterWindow->OnIslandPointerMessage(
            WM_POINTERUPDATE,
            GetContentRoot(),
            previousPointerPoint,
            previousPointerEventArgs,
            true /* isReplayedMessage */,
            &handled_dontCare));

        return true;
    }

    return false;
}

 _Check_return_ HRESULT WindowedPopupInputSiteAdapter::OnPointerMessage(const UINT uMsg, _In_ ixp::IPointerEventArgs* args)
 {
    // The input pointer source can still deliver us pointer input after the popup is closed (if that input happened
    // shortly before the popup closes). Ignore the input in that case. The thing we want to avoid is for that closed
    // popup to cache the pointer input, then try to replay it when the popup opens again.
    if (m_windowedPopupNoRef->IsOpen())
    {
        wrl::ComPtr<ixp::IPointerPoint> transformedPointerPoint;
        IFC_RETURN(GetTransformedPointerPoint(args, &transformedPointerPoint));

        UpdateLastPointerPointForReplay(uMsg, transformedPointerPoint.Get(), args);

        bool handled;
        HRESULT hr = m_jupiterWindow->OnIslandPointerMessage(
            uMsg,
            GetContentRoot(),
            transformedPointerPoint.Get(),
            args,
            false /* isReplayedMessage */,
            &handled);
        IFCFAILFAST(args->put_Handled(handled));
        IFC_RETURN(hr);
    }

    return S_OK;
 }

_Check_return_ HRESULT WindowedPopupInputSiteAdapter::GetTransformedPointerPoint(_In_ ixp::IPointerEventArgs* args, _Out_ ixp::IPointerPoint** transformedPointerPoint)
{
    FAIL_FAST_ASSERT(m_pointerPointTransformFromContentRoot != nullptr);

    wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFCFAILFAST(args->get_CurrentPoint(&pointerPoint));

    UINT32 pointId;
    IFCFAILFAST(pointerPoint->get_PointerId(&pointId));

    IFC_RETURN(pointerPoint->GetTransformedPoint(
        m_pointerPointTransformFromContentRoot.Get(),
        transformedPointerPoint));

    return S_OK;
}
