// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemsPresenter.g.h"
#include "Panel.g.h"
#include "ScrollViewer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

#undef min
#undef max

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::NotifySnapPointsInfoPanelChanged
//
//  Synopsis:
//      Called when the inner panel has been updated
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ItemsPresenter::NotifySnapPointsInfoPanelChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;

    IFC(get_Panel(&spPanel));

    if (m_tpScrollSnapPointsInfo.Cast<Panel>() != spPanel.Cast<Panel>())
    {
        // Refresh the m_tpScrollSnapPointsInfo member based on the new content.
        // The snap point event handlers are unhooked.
        IFC(UnhookScrollSnapPointsInfoEvents(TRUE /*isForHorizontalSnapPoints*/));
        IFC(UnhookScrollSnapPointsInfoEvents(FALSE /*isForHorizontalSnapPoints*/));
        m_tpScrollSnapPointsInfo.Clear();

        if (ctl::is<IScrollSnapPointsInfo>(spPanel))
        {
            SetPtrValue(m_tpScrollSnapPointsInfo, spPanel.Get());
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::HookScrollSnapPointsInfoEvents
//
//  Synopsis:
//    Hooks up one snap points change event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
    ItemsPresenter::HookScrollSnapPointsInfoEvents(
    _In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollSnapPointsInfo> spScrollSnapPointsInfo;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spHorizontalSnapPointsChangedEventHandler;
    ctl::ComPtr<wf::IEventHandler<IInspectable*>> spVerticalSnapPointsChangedEventHandler;

    if (m_tpScrollSnapPointsInfo)
    {
        IFC(m_tpScrollSnapPointsInfo.As(&spScrollSnapPointsInfo));
        if (!m_spHorizontalSnapPointsChangedEventHandler && isForHorizontalSnapPoints)
        {
            spHorizontalSnapPointsChangedEventHandler.Attach(new HorizontalSnapPointsChangedHandler<ItemsPresenter>(this));
            IFC(spScrollSnapPointsInfo->add_HorizontalSnapPointsChanged(spHorizontalSnapPointsChangedEventHandler.Get(), &m_HorizontalSnapPointsChangedToken));
            m_spHorizontalSnapPointsChangedEventHandler = spHorizontalSnapPointsChangedEventHandler;
        }

        if (!m_spVerticalSnapPointsChangedEventHandler && !isForHorizontalSnapPoints)
        {
            spVerticalSnapPointsChangedEventHandler.Attach(new VerticalSnapPointsChangedHandler<ItemsPresenter>(this));
            IFC(spScrollSnapPointsInfo->add_VerticalSnapPointsChanged(spVerticalSnapPointsChangedEventHandler.Get(), &m_VerticalSnapPointsChangedToken));
            m_spVerticalSnapPointsChangedEventHandler = spVerticalSnapPointsChangedEventHandler;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::UnhookScrollSnapPointsInfoEvents
//
//  Synopsis:
//    Unhooks one snap points change event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ItemsPresenter::UnhookScrollSnapPointsInfoEvents(
    _In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollSnapPointsInfo> spScrollSnapPointsInfo;

    if (auto peg = m_tpScrollSnapPointsInfo.TryMakeAutoPeg())
    {
        IFC(m_tpScrollSnapPointsInfo.As(&spScrollSnapPointsInfo));
        if (m_spHorizontalSnapPointsChangedEventHandler && isForHorizontalSnapPoints)
        {
            IFC(spScrollSnapPointsInfo->remove_HorizontalSnapPointsChanged(m_HorizontalSnapPointsChangedToken));
            ZeroMemory(&m_HorizontalSnapPointsChangedToken, sizeof(m_HorizontalSnapPointsChangedToken));
            m_spHorizontalSnapPointsChangedEventHandler = nullptr;
        }

        if (m_spVerticalSnapPointsChangedEventHandler && !isForHorizontalSnapPoints)
        {
            IFC(spScrollSnapPointsInfo->remove_VerticalSnapPointsChanged(m_VerticalSnapPointsChangedToken));
            ZeroMemory(&m_VerticalSnapPointsChangedToken, sizeof(m_VerticalSnapPointsChangedToken));
            m_spVerticalSnapPointsChangedEventHandler = nullptr;
        }
    }

Cleanup:
    RRETURN(hr);
}

// IScrollSnapPointsInfo interface implementation:

_Check_return_ HRESULT ItemsPresenter::get_AreHorizontalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;

    *pValue = FALSE;

    IFC(IsHorizontal(isHorizontal));

    if (isHorizontal)
    {
        IFC(AreScrollSnapPointsRegular(isHorizontal, pValue));
    }

    // When the orientation is vertical, there are no horizontal snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsPresenter::get_AreVerticalSnapPointsRegularImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isHorizontal = FALSE;

    *pValue = FALSE;

    IFC(IsHorizontal(isHorizontal));

    if (!isHorizontal)
    {
        IFC(AreScrollSnapPointsRegular(isHorizontal, pValue));
    }

    // When the orientation is horizontal, there are no vertical snap points.
    // We simply return FALSE then.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::GetRegularSnapPoints
//
//  Synopsis:
//    Returns an original offset and interval for equidistant snap points for
//    the provided orientation. Returns 0 when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ItemsPresenter::GetRegularSnapPointsImpl(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The offset of the first snap point.
    _Out_ FLOAT* pOffset,
    // The interval between the regular snap points.
    _Out_ FLOAT* pInterval)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollSnapPointsInfo> spScrollSnapPointsInfo;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    DOUBLE leadingSnapPointKey = 0.0;
    DOUBLE trailingSnapPointKey = 0.0;
    DOUBLE leadingMarginSnapPointKey = 0.0;
    DOUBLE trailingMarginSnapPointKey = 0.0;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Horizontal;

    *pOffset = 0.0;
    *pInterval = 0.0;

    IFC(get_PhysicalOrientation(&physicalOrientation));

    if (orientation == physicalOrientation)
    {
        IFC(AreScrollSnapPointsRegular(physicalOrientation == xaml_controls::Orientation_Horizontal, &areScrollSnapPointsRegular));

        if (!areScrollSnapPointsRegular)
        {
            // Accessing the regular snap points while AreScrollSnapPointsRegular is False is not supported.
            IFC(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC(ResetSnapPointKeys());

        IFC(GetCommonSnapPointKeys(&leadingSnapPointKey, &trailingSnapPointKey, &leadingMarginSnapPointKey, &trailingMarginSnapPointKey));

        if (m_tpScrollSnapPointsInfo)
        {
            IFC(m_tpScrollSnapPointsInfo.As(&spScrollSnapPointsInfo));
            IFC(spScrollSnapPointsInfo->GetRegularSnapPoints(orientation, alignment, pOffset, pInterval));
        }

        switch (alignment)
        {
        case xaml_primitives::SnapPointsAlignment_Near:
        case xaml_primitives::SnapPointsAlignment_Center:
            *pOffset += (FLOAT)leadingSnapPointKey;
            break;
        case xaml_primitives::SnapPointsAlignment_Far:
            *pOffset += (FLOAT)trailingSnapPointKey;
            break;
        }

        m_regularSnapPointKey = *pInterval;
        m_leadingSnapPointKey = leadingSnapPointKey;
        m_trailingSnapPointKey = trailingSnapPointKey;
        m_leadingMarginSnapPointKey = leadingMarginSnapPointKey;
        m_trailingMarginSnapPointKey = trailingMarginSnapPointKey;
        m_bAreSnapPointsKeysHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

        // Next snap point change needs to raise a notification
        if (m_bAreSnapPointsKeysHorizontal)
        {
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
        else
        {
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   VirtualizingStackPanel::GetIrregularSnapPointsInternal
//
//  Synopsis:
//    Returns a read-only collection of numbers representing the snap points for
//    the provided orientation. Returns an empty collection when no snap points are present.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT ItemsPresenter::GetIrregularSnapPointsImpl(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The read-only collection of snap points.
    _Outptr_ wfc::IVectorView<FLOAT>** pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollSnapPointsInfo> spScrollSnapPointsInfo;
    ctl::ComPtr<wfc::IVectorView<FLOAT>> spPanelsSnapPoints;
    ctl::ComPtr<ValueTypeView<FLOAT>> spSnapPointVV;
    DOUBLE leadingSnapPointKey = 0.0;
    DOUBLE trailingSnapPointKey = 0.0;
    DOUBLE leadingMarginSnapPointKey = 0.0;
    DOUBLE trailingMarginSnapPointKey = 0.0;
    DOUBLE headerSize = 0;
    DOUBLE panelSize = 0;
    DOUBLE footerSize = 0;
    FLOAT offset = 0.0;
    FLOAT interval = 0.0;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Horizontal;
    UINT32 cSnapPoints = 0;
    UINT32 cSnapPointKeys = 0;
    FLOAT* pSnapPoints = NULL;
    FLOAT* pSnapPointKeys = NULL;
    wf::Size viewportSize = {};

    *pValue = NULL;

    IFC(get_PhysicalOrientation(&physicalOrientation));

    if (orientation == physicalOrientation)
    {
        BOOLEAN areScrollSnapPointsRegular = FALSE;
        IFC(AreScrollSnapPointsRegular(orientation == xaml_controls::Orientation_Horizontal, &areScrollSnapPointsRegular));

        if (areScrollSnapPointsRegular)
        {
            // Accessing the regular snap points while AreScrollSnapPointsRegular is False is not supported.
            IFC(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC(ResetSnapPointKeys());

        IFC(GetCommonSnapPointKeys(&leadingSnapPointKey, &trailingSnapPointKey, &leadingMarginSnapPointKey, &trailingMarginSnapPointKey));

        if (m_tpScrollSnapPointsInfo)
        {
            IFC(m_tpScrollSnapPointsInfo.As(&spScrollSnapPointsInfo));
            if (orientation == xaml_controls::Orientation_Horizontal)
            {
                IFC(spScrollSnapPointsInfo->get_AreHorizontalSnapPointsRegular(&areScrollSnapPointsRegular));
            }
            else
            {
                IFC(spScrollSnapPointsInfo->get_AreVerticalSnapPointsRegular(&areScrollSnapPointsRegular));
            }

            if (!areScrollSnapPointsRegular)
            {
                IFC(spScrollSnapPointsInfo->GetIrregularSnapPoints(orientation, alignment, &spPanelsSnapPoints));
            }
            else
            {
                IFC(spScrollSnapPointsInfo->GetRegularSnapPoints(orientation, alignment, &offset, &interval));
            }
        }

        IFC(GetSentinelItemSize(HeaderSentinelIndex, orientation == xaml_controls::Orientation_Horizontal, headerSize));
        IFC(GetSentinelItemSize(FooterSentinelIndex, orientation == xaml_controls::Orientation_Horizontal, footerSize));
        IFC(GetSentinelItemSize(PanelSentinelIndex, orientation == xaml_controls::Orientation_Horizontal, panelSize));

        if (spPanelsSnapPoints)
        {
            UINT32 nCount = 0;
            const UINT32 nHeaderIncrement = headerSize > 0 ? 1 : 0;
            const UINT32 nFooterIncrement = footerSize > 0 ? 1 : 0;

            IFC(spPanelsSnapPoints->get_Size(&nCount));

            const UINT32 nTotalCount = nCount + nHeaderIncrement + nFooterIncrement;

            pSnapPoints = new FLOAT[nTotalCount];
            pSnapPointKeys = new FLOAT[nTotalCount];

            if (nHeaderIncrement > 0)
            {
                pSnapPointKeys[0] = (FLOAT)leadingSnapPointKey;

                switch (alignment)
                {
                case xaml_primitives::SnapPointsAlignment_Near:
                    {
                        pSnapPoints[0] = (FLOAT)leadingSnapPointKey;
                        break;
                    }
                case xaml_primitives::SnapPointsAlignment_Center:
                    {
                        pSnapPoints[0] = (FLOAT)(leadingSnapPointKey + headerSize / 2);
                        break;
                    }
                case xaml_primitives::SnapPointsAlignment_Far:
                    {
                        pSnapPoints[0] = (FLOAT)(leadingSnapPointKey + headerSize);
                        break;
                    }
                }
            }

            for (cSnapPoints = 0; cSnapPoints < nCount; cSnapPoints++, cSnapPointKeys++)
            {
                FLOAT snapPoint = 0;
                IFC(spPanelsSnapPoints->GetAt(cSnapPoints, &snapPoint));

                pSnapPoints[cSnapPoints + nHeaderIncrement] = (FLOAT)(leadingSnapPointKey + headerSize + snapPoint);
                pSnapPointKeys[cSnapPointKeys + nHeaderIncrement] = (FLOAT)snapPoint;
            }

            if(nFooterIncrement > 0)
            {
                FLOAT footerSnapPointKey = (FLOAT)(leadingSnapPointKey + headerSize + panelSize);

                pSnapPointKeys[nCount + nHeaderIncrement] = footerSnapPointKey;

                switch (alignment)
                {
                case xaml_primitives::SnapPointsAlignment_Near:
                    {
                        pSnapPoints[nCount + nHeaderIncrement] = footerSnapPointKey;
                        break;
                    }
                case xaml_primitives::SnapPointsAlignment_Center:
                    {
                        pSnapPoints[nCount + nHeaderIncrement] = footerSnapPointKey + (FLOAT)(footerSize / 2);
                        break;
                    }
                case xaml_primitives::SnapPointsAlignment_Far:
                    {
                        pSnapPoints[nCount + nHeaderIncrement] = footerSnapPointKey + (FLOAT)footerSize;
                        break;
                    }
                }
            }

            cSnapPoints += nHeaderIncrement + nFooterIncrement;
            cSnapPointKeys += nHeaderIncrement + nFooterIncrement;
        }
        else if (interval > 0)
        {
            // let's prepare 5 screens of snap points.
            // Since footers are not supported for logical panels, we will
            // not add a footer snap point if our panel is logical.

            FLOAT viewport = 0.0;
            FLOAT count = 0.0;
            FLOAT start = 0.0;
            UINT32 nCount = 0;

            FLOAT snapPoint = 0;

            if (ctl::is<IModernCollectionBasePanel>(m_tpScrollSnapPointsInfo.Get()))
            {
                // Get the viewport and offset of the ScrollViewer
                ctl::ComPtr<IScrollViewer> spScrollViewer;

                IFC(m_wrScrollViewer.As(&spScrollViewer));
                if (spScrollViewer)
                {
                    FLOAT zoomFactor;
                    FLOAT scrollOffset;

                    IFC(spScrollViewer->get_ZoomFactor(&zoomFactor));

                    if (orientation == xaml_controls::Orientation_Horizontal)
                    {
                        DOUBLE temp;
                        IFC(spScrollViewer->get_ViewportWidth(&temp));
                        viewport = static_cast<FLOAT>(temp) / zoomFactor;
                        IFC(spScrollViewer->get_HorizontalOffset(&temp));
                        scrollOffset = static_cast<FLOAT>(temp) / zoomFactor;
                    }
                    else
                    {
                        DOUBLE temp;
                        IFC(spScrollViewer->get_ViewportHeight(&temp));
                        viewport = static_cast<FLOAT>(temp) / zoomFactor;
                        IFC(spScrollViewer->get_VerticalOffset(&temp));
                        scrollOffset = static_cast<FLOAT>(temp) / zoomFactor;
                    }

                    count = viewport / interval * 5;

                    FLOAT startOfPanel = static_cast<FLOAT>(leadingSnapPointKey + headerSize);
                    FLOAT startOfSnapPointWindowOnPanel = std::max(scrollOffset - (2 * viewport) - startOfPanel, 0.0f);

                    // If alignment is near or center, we just need to start from the
                    // first snap point and count intervals until we reach our desired pixel start
                    if (alignment == xaml_primitives::SnapPointsAlignment_Near ||
                        alignment == xaml_primitives::SnapPointsAlignment_Center)
                    {
                        FLOAT intervalsToWindow = std::max((startOfSnapPointWindowOnPanel - offset) / interval, 0.0f);
                        snapPoint = offset + (std::floor(intervalsToWindow) * interval);
                        if (alignment == xaml_primitives::SnapPointsAlignment_Center)
                        {
                            snapPoint -= interval / 2;
                        }
                    }
                    else
                    {
                        // We need to walk backwards from the end snap point
                        FLOAT endOfPanel = static_cast<FLOAT>(leadingSnapPointKey + headerSize + panelSize);
                        FLOAT intervalsToWindow = std::max((endOfPanel - offset - startOfSnapPointWindowOnPanel) / interval, 0.0f);
                        snapPoint = static_cast<FLOAT>(panelSize) - offset - (std::ceil(intervalsToWindow) * interval);
                    }
                }
            }
            else
            {
                // Look at the viewport and offset of the panel if it is logical
                IFC(get_DesiredSize(&viewportSize));

                if (orientation == xaml_controls::Orientation_Horizontal)
                {
                    viewport = viewportSize.Width;
                }
                else
                {
                    viewport = viewportSize.Height;
                }

                count = viewport / interval * 5;
                start = viewport / interval * 2;

                DOUBLE panelOffset = 0;
                DOUBLE panelMaxOffset = 0;
                IFC(GetInnerPanelOffset(orientation == xaml_controls::Orientation_Horizontal, &panelOffset));
                IFC(GetInnerPanelsScrollableDimension(orientation == xaml_controls::Orientation_Horizontal, &panelMaxOffset));
                panelOffset = MAX(panelOffset + MIN(panelMaxOffset - (panelOffset + (count - start)), 0) - start, 0);
                snapPoint = static_cast<FLOAT>(DoubleUtil::Floor(panelOffset)) * interval;
            }

            nCount = static_cast<UINT32>(DoubleUtil::Floor(count));

            if (nCount > 0)
            {
                pSnapPoints = new FLOAT[nCount];
                pSnapPointKeys = new FLOAT[nCount];
            }

            for (cSnapPoints = 0; cSnapPoints < nCount; ++cSnapPoints, ++cSnapPointKeys)
            {
                switch (alignment)
                {
                case xaml_primitives::SnapPointsAlignment_Near:
                    {
                        if (headerSize > 0 && cSnapPoints == 0)
                        {
                            pSnapPoints[cSnapPoints] = (FLOAT)leadingSnapPointKey;
                            pSnapPointKeys[cSnapPointKeys] = (FLOAT)leadingSnapPointKey;
                        }
                        else
                        {
                            pSnapPoints[cSnapPoints] = (FLOAT)(leadingSnapPointKey + headerSize + snapPoint);
                            pSnapPointKeys[cSnapPointKeys] = (FLOAT)(leadingSnapPointKey + headerSize + snapPoint);
                            snapPoint += interval;
                        }
                        break;
                    }
                case xaml_primitives::SnapPointsAlignment_Center:
                    {
                        if (headerSize > 0 && cSnapPoints == 0)
                        {
                            pSnapPoints[cSnapPoints] = (FLOAT)(leadingSnapPointKey + headerSize / 2);
                            pSnapPointKeys[cSnapPointKeys] = (FLOAT)leadingSnapPointKey;
                        }
                        else
                        {
                            pSnapPoints[cSnapPoints] = (FLOAT) (leadingSnapPointKey + headerSize + snapPoint + interval / 2);
                            pSnapPointKeys[cSnapPointKeys] = (FLOAT)(leadingSnapPointKey + headerSize + snapPoint);
                            snapPoint += interval;
                        }
                        break;
                    }
                case xaml_primitives::SnapPointsAlignment_Far:
                    {
                        if (headerSize > 0 && cSnapPoints == 0)
                        {
                            pSnapPoints[cSnapPoints] = (FLOAT)(leadingSnapPointKey + headerSize);
                            pSnapPointKeys[cSnapPointKeys] = (FLOAT)leadingSnapPointKey;
                        }
                        else
                        {
                            pSnapPoints[cSnapPoints] = (FLOAT)(leadingSnapPointKey + headerSize + snapPoint + interval);
                            pSnapPointKeys[cSnapPointKeys] = (FLOAT)(leadingSnapPointKey + headerSize + snapPoint);
                            snapPoint += interval;
                        }
                        break;
                    }
                }
            }
        }

        if (cSnapPoints > 0)
        {
            IFC(ctl::make<ValueTypeView<FLOAT>>(&spSnapPointVV));
            IFC(spSnapPointVV->SetView(pSnapPoints, cSnapPoints));
            IFC(spSnapPointVV.CopyTo(pValue));
        }

        if (cSnapPointKeys > 0)
        {
            m_pIrregularSnapPointKeys = pSnapPointKeys;
            m_cIrregularSnapPointKeys = cSnapPointKeys;
            // TODO figure out how to re-pack offset
            // m_irregularSnapPointKeysOffset = irregularSnapPointKeysOffset;
            m_bAreSnapPointsKeysHorizontal = orientation == xaml_controls::Orientation_Horizontal;
            pSnapPointKeys = NULL;
        }

        m_leadingSnapPointKey = leadingSnapPointKey;
        m_trailingSnapPointKey = trailingSnapPointKey;
        m_leadingMarginSnapPointKey = leadingMarginSnapPointKey;
        m_trailingMarginSnapPointKey = trailingMarginSnapPointKey;
        m_bAreSnapPointsKeysHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

        // Next snap point change needs to raise a notification
        if (m_bAreSnapPointsKeysHorizontal)
        {
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
        else
        {
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

Cleanup:
    delete [] pSnapPoints;
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::add_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
ItemsPresenter::add_HorizontalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            true /*isForHorizontalSnapPoints*/,
            true /*notifyChanges*/));

        IFC(HookScrollSnapPointsInfoEvents(TRUE));
    }

    IFC(ItemsPresenterGenerated::add_HorizontalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::remove_HorizontalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
ItemsPresenter::remove_HorizontalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(ItemsPresenterGenerated::remove_HorizontalSnapPointsChanged(tToken));

    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));

    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            true /*isForHorizontalSnapPoints*/,
            false /*notifyChanges*/));

        IFC(UnhookScrollSnapPointsInfoEvents(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::add_VerticalSnapPointsChanged
//
//  Synopsis:
//    Adds an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
ItemsPresenter::add_VerticalSnapPointsChanged(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken)
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            false /*isForHorizontalSnapPoints*/,
            true /*notifyChanges*/));
        IFC(HookScrollSnapPointsInfoEvents(FALSE));
    }

    IFC(ItemsPresenterGenerated::add_VerticalSnapPointsChanged(pValue, ptToken));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   OrientedVirtualizingPanel::remove_VerticalSnapPointsChanged
//
//  Synopsis:
//    Removes an event handler for the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
ItemsPresenter::remove_VerticalSnapPointsChanged(_In_ EventRegistrationToken tToken)
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;

    IFC(ItemsPresenterGenerated::remove_VerticalSnapPointsChanged(tToken));

    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        IFC(SetSnapPointsChangeNotificationsRequirement(
            false /*isForHorizontalSnapPoints*/,
            false /*notifyChanges*/));

        IFC(UnhookScrollSnapPointsInfoEvents(FALSE));
    }

Cleanup:
    RRETURN(hr);
}

// End of IScrollSnapPointsInfo interface implementation

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::AreScrollSnapPointsRegular
//
//  Synopsis:
//    Returns combined information about children snap points and self
//    True only if children snap points are regular and LeadingPadding==HeaderSize==ContainerSize=TrailingPadding
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
ItemsPresenter::AreScrollSnapPointsRegular(
    _In_ BOOLEAN isForHorizontalSnapPoints,
    _Out_ BOOLEAN* pAreScrollSnapPointsRegular)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollSnapPointsInfo> spScrollSnapPointsInfo;
    BOOLEAN areSnapPointsRegular = FALSE;

    if (m_tpScrollSnapPointsInfo)
    {
        IFC(m_tpScrollSnapPointsInfo.As(&spScrollSnapPointsInfo));
        if (isForHorizontalSnapPoints)
        {
            IFC(spScrollSnapPointsInfo->get_AreHorizontalSnapPointsRegular(&areSnapPointsRegular));
        }
        else
        {
            IFC(spScrollSnapPointsInfo->get_AreVerticalSnapPointsRegular(&areSnapPointsRegular));
        }
    }

    *pAreScrollSnapPointsRegular = areSnapPointsRegular;

    if (areSnapPointsRegular)
    {
        DOUBLE leadingPaddingSize = 0;
        DOUBLE trailingPaddingSize = 0;
        DOUBLE headerSize = 0;
        DOUBLE footerSize = 0;
        wf::Size containerSize = {};

        IFC(GetSentinelItemSize(TrailingPaddingSentinelIndex, isForHorizontalSnapPoints, trailingPaddingSize));
        IFC(GetSentinelItemSize(LeadingPaddingSentinelIndex, isForHorizontalSnapPoints, leadingPaddingSize));
        IFC(GetSentinelItemSize(HeaderSentinelIndex, isForHorizontalSnapPoints, headerSize));
        IFC(GetSentinelItemSize(FooterSentinelIndex, isForHorizontalSnapPoints, footerSize));

        // is we don't affect layout then we regular
        if (trailingPaddingSize == 0 && leadingPaddingSize == 0 && headerSize == 0 && footerSize == 0)
        {
            goto Cleanup;
        }

        IFC(GetSizeOfContainer(containerSize));

        *pAreScrollSnapPointsRegular = DoubleUtil::Floor(trailingPaddingSize) == DoubleUtil::Floor(leadingPaddingSize);
        *pAreScrollSnapPointsRegular &= DoubleUtil::Floor(leadingPaddingSize) == DoubleUtil::Floor(footerSize);
        *pAreScrollSnapPointsRegular &= DoubleUtil::Floor(footerSize) == DoubleUtil::Floor(headerSize);
        *pAreScrollSnapPointsRegular &= DoubleUtil::Floor(headerSize) == DoubleUtil::Floor(isForHorizontalSnapPoints ? containerSize.Width : containerSize.Height);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::SetSnapPointsChangeNotificationsRequirement
//
//  Synopsis:
//    Determines whether the OrientedVirtualizingPanel must call NotifySnapPointsChanged
//    when snap points change or not.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::SetSnapPointsChangeNotificationsRequirement(
    _In_ BOOLEAN isForHorizontalSnapPoints,
    _In_ BOOLEAN notifyChanges)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));

    IFC(AreScrollSnapPointsRegular(isHorizontal, &areScrollSnapPointsRegular));

    if (isForHorizontalSnapPoints)
    {
        m_bNotifyHorizontalSnapPointsChanges = notifyChanges;
        if (isHorizontal && notifyChanges)
        {
            if (areScrollSnapPointsRegular)
            {
                IFC(RefreshRegularSnapPointKeys());
            }
            else
            {
                IFC(RefreshIrregularSnapPointKeys());
            }
            m_bNotifiedHorizontalSnapPointsChanges = FALSE;
        }
    }
    else
    {
        m_bNotifyVerticalSnapPointsChanges = notifyChanges;
        if (isHorizontal && notifyChanges)
        {
            if (areScrollSnapPointsRegular)
            {
                IFC(RefreshRegularSnapPointKeys());
            }
            else
            {
                IFC(RefreshIrregularSnapPointKeys());
            }
            m_bNotifiedVerticalSnapPointsChanges = FALSE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::RefreshRegularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_regularSnapPointKey field based on a single child.
//    Refreshes also the m_leadingSnapPointKey/m_trailingSnapPointKey fields based
//    on the current margins.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::RefreshRegularSnapPointKeys()
{
    HRESULT hr = S_OK;
    FLOAT snapPointKey = 0.0;
    DOUBLE leadingSnapPointKey = 0.0;
    DOUBLE trailingSnapPointKey = 0.0;
    DOUBLE leadingMarginSnapPointKey = 0.0;
    DOUBLE trailingMarginSnapPointKey = 0.0;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));

#ifdef DBG
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    IFC(AreScrollSnapPointsRegular(isHorizontal, &areScrollSnapPointsRegular));
    ASSERT(areScrollSnapPointsRegular);
#endif

    IFC(ResetSnapPointKeys());

    m_regularSnapPointKey = 0.0;

    IFC(GetRegularSnapPointKeys(isHorizontal, &snapPointKey, &leadingSnapPointKey, &trailingSnapPointKey, &leadingMarginSnapPointKey, &trailingMarginSnapPointKey));

    m_bAreSnapPointsKeysHorizontal = isHorizontal;
    m_regularSnapPointKey = snapPointKey;
    m_leadingSnapPointKey = leadingSnapPointKey;
    m_trailingSnapPointKey = trailingSnapPointKey;
    m_leadingMarginSnapPointKey = leadingMarginSnapPointKey;
    m_trailingMarginSnapPointKey = trailingMarginSnapPointKey;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::RefreshIrregularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_pIrregularSnapPointKeys/m_cIrregularSnapPointKeys
//    fields based on all children.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::RefreshIrregularSnapPointKeys()
{
    HRESULT hr = S_OK;
    INT32 cSnapPointKeys = 0;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT irregularSnapPointKeysOffset = 0.0;
    DOUBLE leadingSnapPointKey = 0.0;
    DOUBLE trailingSnapPointKey = 0.0;
    DOUBLE leadingMarginSnapPointKey = 0.0;
    DOUBLE trailingMarginSnapPointKey = 0.0;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));

#ifdef DBG
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    IFC(AreScrollSnapPointsRegular(isHorizontal, &areScrollSnapPointsRegular));
    ASSERT(!areScrollSnapPointsRegular);
#endif

    IFC(ResetSnapPointKeys());

    IFC(GetIrregularSnapPointKeys(
        isHorizontal,
        &pSnapPointKeys,
        &cSnapPointKeys,
        &irregularSnapPointKeysOffset,
        &leadingSnapPointKey,
        &trailingSnapPointKey,
        &leadingMarginSnapPointKey,
        &trailingMarginSnapPointKey));

    m_pIrregularSnapPointKeys = pSnapPointKeys;
    m_cIrregularSnapPointKeys = cSnapPointKeys;
    m_irregularSnapPointKeysOffset = irregularSnapPointKeysOffset;
    m_leadingSnapPointKey = leadingSnapPointKey;
    m_trailingSnapPointKey = trailingSnapPointKey;
    m_leadingMarginSnapPointKey = leadingMarginSnapPointKey;
    m_trailingMarginSnapPointKey = trailingMarginSnapPointKey;
    m_bAreSnapPointsKeysHorizontal = isHorizontal;
    pSnapPointKeys = NULL;

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::ResetSnapPointKeys
//
//  Synopsis:
//    Resets regular snap point keys.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::ResetSnapPointKeys()
{
    m_regularSnapPointKey = -1.0;
    m_leadingSnapPointKey = 0;
    m_trailingSnapPointKey = 0;
    m_leadingMarginSnapPointKey = 0;
    m_trailingMarginSnapPointKey = 0;

    delete [] m_pIrregularSnapPointKeys;
    m_pIrregularSnapPointKeys = NULL;
    m_cIrregularSnapPointKeys = -1;
    m_irregularSnapPointKeysOffset = 0.0f;

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::GetRegularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for regular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
ItemsPresenter::GetRegularSnapPointKeys(
    _In_ BOOLEAN isForHorizontalSnapPoints,
    _Out_ FLOAT* pSnapPointKey,
    _Out_ DOUBLE* pLeadingSnapPointKey,
    _Out_ DOUBLE* pTrailingSnapPointKey,
    _Out_ DOUBLE* pLeadingMarginSnapPointKey,
    _Out_ DOUBLE* pTrailingMarginSnapPointKey)
{
    HRESULT hr = S_OK;
    wf::Size containerSize = {};

    *pSnapPointKey = 0.0;
    *pLeadingSnapPointKey = 0.0;
    *pTrailingSnapPointKey = 0.0;

    IFC(GetSizeOfContainer(containerSize));
    *pSnapPointKey = isForHorizontalSnapPoints ? containerSize.Width : containerSize.Height;

    IFC(GetCommonSnapPointKeys(pLeadingSnapPointKey, pTrailingSnapPointKey, pLeadingMarginSnapPointKey, pTrailingMarginSnapPointKey));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::GetIrregularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
ItemsPresenter::GetIrregularSnapPointKeys(
    _In_ BOOLEAN isForHorizontalSnapPoints,
    _Outptr_result_buffer_(*pcSnapPointKeys) FLOAT** ppSnapPointKeys,
    _Out_ INT32* pcSnapPointKeys,
    _Out_ FLOAT* pSnapPointKeysOffset,
    _Out_ DOUBLE* pLeadingSnapPointKey,
    _Out_ DOUBLE* pTrailingSnapPointKey,
    _Out_ DOUBLE* pLeadingMarginSnapPointKey,
    _Out_ DOUBLE* pTrailingMarginSnapPointKey)
{
    HRESULT hr = S_OK;

    *ppSnapPointKeys = NULL;
    *pcSnapPointKeys = 0;
    *pSnapPointKeysOffset = 0.0;
    *pLeadingSnapPointKey = 0.0;
    *pTrailingSnapPointKey = 0.0;

    // for now we are not storing irregular keys, it make every arrange to invalidate snap points.

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::GetCommonSnapPointKeys
//
//  Synopsis:
//    Determines the common keys for regular and irregular snap points.
//    Those keys are the left/right padding for a horizontal panel,
//    or the top/bottom padding for a vertical panel.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::GetCommonSnapPointKeys(
    _Out_ DOUBLE* pLeadingSnapPointKey,
    _Out_ DOUBLE* pTrailingSnapPointKey,
    _Out_ DOUBLE* pLeadingMarginSnapPointKey,
    _Out_ DOUBLE* pTrailingMarginSnapPointKey)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    BOOLEAN isHorizontal = FALSE;
    xaml::Thickness panelsMargins = {};

    *pLeadingSnapPointKey = 0.0;
    *pTrailingSnapPointKey = 0.0;

    IFC(IsHorizontal(isHorizontal));

    IFC(GetSentinelItemSize(TrailingPaddingSentinelIndex, isHorizontal, *pTrailingSnapPointKey));
    IFC(GetSentinelItemSize(LeadingPaddingSentinelIndex, isHorizontal, *pLeadingSnapPointKey));

    IFC(get_Panel(&spPanel));
    IFCCHECK(spPanel.Get());

    IFC(spPanel.Cast<Panel>()->get_Margin(&panelsMargins));
    if (isHorizontal)
    {
        *pLeadingMarginSnapPointKey = panelsMargins.Left;
        *pTrailingMarginSnapPointKey = panelsMargins.Right;
    }
    else
    {
        *pLeadingMarginSnapPointKey = panelsMargins.Top;
        *pTrailingMarginSnapPointKey = panelsMargins.Bottom;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::NotifySnapPointsChanges
//
//  Synopsis:
//    Checks if the snap point keys have changed and a notification needs
//    to be raised.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::NotifySnapPointsChanges()
{
    HRESULT hr = S_OK;
    INT32 cSnapPointKeys = 0;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT snapPointKeysOffset = 0.0;
    FLOAT snapPointKey = 0.0;
    DOUBLE leadingSnapPointKey = 0.0;
    DOUBLE trailingSnapPointKey = 0.0;
    DOUBLE leadingMarginSnapPointKey = 0.0;
    DOUBLE trailingMarginSnapPointKey = 0.0;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    BOOLEAN notifyForHorizontalSnapPoints = FALSE;
    BOOLEAN notifyForVerticalSnapPoints = FALSE;
    BOOLEAN isHorizontal = FALSE;

    IFC(IsHorizontal(isHorizontal));
    IFC(AreScrollSnapPointsRegular(isHorizontal, &areScrollSnapPointsRegular));

    if (!isHorizontal)
    {
        if (((m_regularSnapPointKey != -1.0) || (m_cIrregularSnapPointKeys != -1)) &&
            m_bAreSnapPointsKeysHorizontal && m_bNotifyHorizontalSnapPointsChanges)
        {
            // Last computed snap point keys were for horizontal orientation.
            // New orientation is vertical.
            // Consumer wants notifications for horizontal snap points.
            notifyForHorizontalSnapPoints = TRUE;
        }
    }
    else
    {
        if (((m_regularSnapPointKey != -1.0) || (m_cIrregularSnapPointKeys != -1)) &&
            !m_bAreSnapPointsKeysHorizontal && m_bNotifyVerticalSnapPointsChanges)
        {
            // Last computed snap point keys were for vertical orientation.
            // New orientation is horizontal.
            // Consumer wants notifications for vertical snap points.
            notifyForVerticalSnapPoints = TRUE;
        }
    }

    if ((m_bNotifyHorizontalSnapPointsChanges && isHorizontal &&
         m_bAreSnapPointsKeysHorizontal && !m_bNotifiedHorizontalSnapPointsChanges) ||
        (m_bNotifyVerticalSnapPointsChanges && !isHorizontal &&
         !m_bAreSnapPointsKeysHorizontal && !m_bNotifiedVerticalSnapPointsChanges))
    {
        if (m_regularSnapPointKey != -1.0)
        {
            if (areScrollSnapPointsRegular)
            {
                IFC(GetRegularSnapPointKeys(isHorizontal, &snapPointKey, &leadingSnapPointKey, &trailingSnapPointKey, &leadingMarginSnapPointKey, &trailingMarginSnapPointKey));
                if (m_regularSnapPointKey != snapPointKey ||
                    m_leadingSnapPointKey != leadingSnapPointKey ||
                    m_trailingSnapPointKey != trailingSnapPointKey ||
                    m_leadingMarginSnapPointKey != leadingMarginSnapPointKey ||
                    m_trailingMarginSnapPointKey != trailingMarginSnapPointKey)
                {
                    if (m_bAreSnapPointsKeysHorizontal)
                    {
                        notifyForHorizontalSnapPoints = TRUE;
                    }
                    else
                    {
                        notifyForVerticalSnapPoints = TRUE;
                    }
                }
            }
            else
            {
                if (m_bAreSnapPointsKeysHorizontal)
                {
                    notifyForHorizontalSnapPoints = TRUE;
                }
                else
                {
                    notifyForVerticalSnapPoints = TRUE;
                }
            }
        }
        else if (m_cIrregularSnapPointKeys != -1)
        {
            if (!areScrollSnapPointsRegular)
            {
                IFC(GetIrregularSnapPointKeys(isHorizontal, &pSnapPointKeys, &cSnapPointKeys, &snapPointKeysOffset, &leadingSnapPointKey, &trailingSnapPointKey, &leadingMarginSnapPointKey, &trailingMarginSnapPointKey));
                if (m_cIrregularSnapPointKeys != cSnapPointKeys ||
                    m_irregularSnapPointKeysOffset != snapPointKeysOffset ||
                    m_leadingSnapPointKey != leadingSnapPointKey ||
                    m_trailingSnapPointKey != trailingSnapPointKey ||
                    m_leadingMarginSnapPointKey != leadingMarginSnapPointKey ||
                    m_trailingMarginSnapPointKey != trailingMarginSnapPointKey)
                {
                    if (m_bAreSnapPointsKeysHorizontal)
                    {
                        notifyForHorizontalSnapPoints = TRUE;
                    }
                    else
                    {
                        notifyForVerticalSnapPoints = TRUE;
                    }
                }
                else
                {
                    for (INT32 iSnapPointKey = 0; iSnapPointKey < cSnapPointKeys; iSnapPointKey++)
                    {
                        if (m_pIrregularSnapPointKeys[iSnapPointKey] != pSnapPointKeys[iSnapPointKey])
                        {
                            if (m_bAreSnapPointsKeysHorizontal)
                            {
                                notifyForHorizontalSnapPoints = TRUE;
                            }
                            else
                            {
                                notifyForVerticalSnapPoints = TRUE;
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                if (m_bAreSnapPointsKeysHorizontal)
                {
                    notifyForHorizontalSnapPoints = TRUE;
                }
                else
                {
                    notifyForVerticalSnapPoints = TRUE;
                }
            }
        }
    }

    if (notifyForHorizontalSnapPoints)
    {
        IFC(NotifySnapPointChanges(TRUE /*isForHorizontalSnapPoints*/));
    }

    if (notifyForVerticalSnapPoints)
    {
        IFC(NotifySnapPointChanges(FALSE /*isForHorizontalSnapPoints*/));
    }

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::NotifySnapPointChanges
//
//  Synopsis:
//
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
ItemsPresenter::NotifySnapPointChanges(_In_ BOOLEAN isForHorizontalSnapPoints)
{
    HRESULT hr = S_OK;

    if ((isForHorizontalSnapPoints && m_bNotifyHorizontalSnapPointsChanges && !m_bNotifiedHorizontalSnapPointsChanges) ||
        (!isForHorizontalSnapPoints && m_bNotifyVerticalSnapPointsChanges && !m_bNotifiedVerticalSnapPointsChanges))
    {
        if (isForHorizontalSnapPoints)
        {
            // Raise HorizontalSnapPointsChanged event.
            m_bNotifiedHorizontalSnapPointsChanges = TRUE;
            IFC(OnHorizontalSnapPointsChanged());
        }
        else
        {
            // Raise VerticalSnapPointsChanged event.
            m_bNotifiedVerticalSnapPointsChanges = TRUE;
            IFC(OnVerticalSnapPointsChanged());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called by HorizontalSnapPointsChangedHandler and
// VerticalSnapPointsChangedHandler when snap points changed
_Check_return_ HRESULT ItemsPresenter::OnSnapPointsChanged(_In_ DMMotionTypes motionType)
{
    HRESULT hr = S_OK;
    switch(motionType)
    {
    case DMMotionTypePanX:
        IFC(OnHorizontalSnapPointsChanged());
        break;

    case DMMotionTypePanY:
        IFC(OnVerticalSnapPointsChanged());
        break;

    default:
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}
//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::OnHorizontalSnapPointsChanged
//
//  Synopsis:
//    Raises the HorizontalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::OnHorizontalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    HorizontalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make<EventArgs>(&spArgs));

    // Raise the event
    IFC(GetHorizontalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   ItemsPresenter::OnVerticalSnapPointsChanged
//
//  Synopsis:
//    Raises the VerticalSnapPointsChanged event.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT ItemsPresenter::OnVerticalSnapPointsChanged()
{
    HRESULT hr = S_OK;
    VerticalSnapPointsChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<EventArgs> spArgs;

    // Create the args
    IFC(ctl::make<EventArgs>(&spArgs));

    // Raise the event
    IFC(GetVerticalSnapPointsChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), ctl::as_iinspectable(spArgs.Get())));

Cleanup:
    RRETURN(hr);
}
