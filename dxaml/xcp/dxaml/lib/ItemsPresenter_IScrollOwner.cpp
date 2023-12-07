// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemsPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// IScrollOwner implementation.

_Check_return_
HRESULT
ItemsPresenter::InvalidateScrollInfoImpl()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));

    if (spScrollOwner)
    {
        // We have layout cycle if pass it to the owner
        // Since we are controlling scrolling data is is safe to not pass it to the owner

        // Need to invalidate our own measure to ensure that we update the scroll information
        // which includes the scroll bars
        IFC(InvalidateMeasure());
    }

Cleanup:
    RRETURN(hr);
}

// Notifies the scroll owner that an ArrangeOverride execution occurred
// after its IManipulationDataProvider::UpdateInManipulation(...) call.
_Check_return_
HRESULT
ItemsPresenter::NotifyLayoutRefreshed()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(spScrollOwner->NotifyLayoutRefreshed());
    }

Cleanup:
    RRETURN(hr);
}

// Called to notify the scroll owner of a new horizontal offset request.
_Check_return_
HRESULT
ItemsPresenter::NotifyHorizontalOffsetChanging(
    _In_ DOUBLE targetHorizontalOffset,
    _In_ DOUBLE targetVerticalOffset)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(IsHorizontal(isHorizontal));
        IFC(spScrollOwner->NotifyHorizontalOffsetChanging(isHorizontal ? targetHorizontalOffset + 2 : targetHorizontalOffset,
                                                          isHorizontal ? targetVerticalOffset : targetVerticalOffset + 2));
    }

Cleanup:
    RRETURN(hr);
}

// Called to notify the scroll owner of a new vertical offset request.
_Check_return_
HRESULT
ItemsPresenter::NotifyVerticalOffsetChanging(
    _In_ DOUBLE targetHorizontalOffset,
    _In_ DOUBLE targetVerticalOffset)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));
    if (spScrollOwner)
    {
        IFC(IsHorizontal(isHorizontal));
        IFC(spScrollOwner->NotifyVerticalOffsetChanging(isHorizontal ? targetHorizontalOffset + 2 : targetHorizontalOffset,
                                                        isHorizontal ? targetVerticalOffset : targetVerticalOffset + 2));
    }

Cleanup:
    RRETURN(hr);
}

// Scrolls the content within the scroll owner to the specified
// horizontal offset position.
_Check_return_
HRESULT
ItemsPresenter::ScrollToHorizontalOffsetImpl(
    _In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));
    IFC(SetHorizontalOffsetImpl(isHorizontal ? offset + 2 : offset));

Cleanup:
    RRETURN(hr);
}

// Scrolls the content within the scroll owner to the specified vertical
// offset position.
_Check_return_
HRESULT
ItemsPresenter::ScrollToVerticalOffsetImpl(
    _In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));
    IFC(SetVerticalOffsetImpl(isHorizontal ? offset : offset + 2));

Cleanup:
    RRETURN(hr);
}

// Sets reference to the IScrollInfo implementation
_Check_return_ HRESULT ItemsPresenter::put_ScrollInfo(
    _In_opt_ IScrollInfo* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IScrollInfo> spPanelsInfo;

    IFC(get_Panel(&spPanel));
    IFC(spPanel.As(&spPanelsInfo));

    IFCEXPECT(spPanelsInfo.Get()== pValue);

Cleanup:
    RRETURN(hr);
}

// Returns reference to the IScrollInfo implementation
_Check_return_
HRESULT
ItemsPresenter::get_ScrollInfo(
    _Outptr_result_maybenull_ IScrollInfo** ppValue)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IPanel> spPanel;

    IFC(get_Panel(&spPanel));
    IFC(spPanel.CopyTo(ppValue));

Cleanup:
    RRETURN(S_OK);
}

// Returns zoom factor
_Check_return_
HRESULT
ItemsPresenter::get_ZoomFactorImpl(
    _Out_ FLOAT* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));

    if (spScrollOwner)
    {
        IFC(spScrollOwner->get_ZoomFactorImpl(pValue));
    }

Cleanup:
    RRETURN(hr);
}

// Called when this DM container wants the DM handler to process the current
// pure inertia input message, by forwarding it to DirectManipulation.
// The handler must set the isHandled flag to True if the message was handled.
// Unfortunately, callers to this method must determine whether or not DM will treat
// the current input message as a pure inertia manipulation.
// PLEASE NOTE: You won't find an input message as a parameter to this function.
// The implementation just calls ProcessInputMessage on the manipulation handler
// (in most cases).
_Check_return_
HRESULT
ItemsPresenter::ProcessPureInertiaInputMessage(
    _In_ ZoomDirection zoomDirection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));

    if (spScrollOwner)
    {
        IFC(spScrollOwner->ProcessPureInertiaInputMessage(zoomDirection));
    }

Cleanup:
    RRETURN(hr);
}

// Returns true if currently in DM zooming
_Check_return_
HRESULT
ItemsPresenter::IsInDirectManipulationZoom(
    _Out_ BOOLEAN& bIsInDirectManipulationZoom)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    bIsInDirectManipulationZoom = FALSE;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));

    if (spScrollOwner)
    {
        IFC(spScrollOwner->IsInDirectManipulationZoom(bIsInDirectManipulationZoom));
    }

Cleanup:
    RRETURN(hr);
}

// We cannot invalidate the grandchild directly. So this property is
// informing that we are invalidating the child so the grandchild can
// use it.
_Check_return_ HRESULT ItemsPresenter::IsInChildInvalidateMeasure(
    _Out_ BOOLEAN& bIsInChildInvalidateMeasure)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spScrollOwner;
    bIsInChildInvalidateMeasure = FALSE;

    IFC(m_ScrollData.get_ScrollOwner(&spScrollOwner));

    if (spScrollOwner)
    {
        IFC(spScrollOwner->IsInChildInvalidateMeasure(bIsInChildInvalidateMeasure));
    }

Cleanup:
    RRETURN(hr);
}

