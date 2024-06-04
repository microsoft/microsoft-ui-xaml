// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VirtualizingStackPanel.g.h"
#include "PropertyMetadata.g.h"
#include "ListViewBase.g.h"
#include "ItemContainerGenerator.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT VirtualizingStackPanel::GetVirtualizationModePropertyMetadata(_Out_ xaml::IPropertyMetadata **ppMetadata)
{
    RRETURN(PropertyMetadata::CreateWithReferenceDefaultValue<xaml_controls::VirtualizationMode>(xaml_controls::VirtualizationMode_Recycling, ppMetadata));
}

VirtualizingStackPanel::VirtualizingStackPanel() :
        m_irregularSnapPointKeysOffset(0.0f),
        m_pIrregularSnapPointKeys(NULL),
        m_cIrregularSnapPointKeys(0),
        m_newItemsStartPosition(0.0f),
        m_viewPortSizeInPixels(0)
{
}

VirtualizingStackPanel::~VirtualizingStackPanel()
{
    delete [] m_pIrregularSnapPointKeys;
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
_Check_return_
HRESULT
VirtualizingStackPanel::GetIrregularSnapPointsInternal(
    // The direction of the requested snap points.
    _In_ xaml_controls::Orientation orientation,
    // The alignment used by the caller when applying the requested snap points.
    _In_ xaml_primitives::SnapPointsAlignment alignment,
    // The read-only collection of snap points.
    _Outptr_ wfc::IVectorView<FLOAT>** ppValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ValueTypeView<FLOAT>> spSnapPointsVTV;
    UINT32 cSnapPoints = 0;
    FLOAT* pSnapPoints = NULL;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(GetIrregularSnapPoints(
        orientation == xaml_controls::Orientation_Horizontal,
        alignment == xaml_primitives::SnapPointsAlignment_Near,
        alignment == xaml_primitives::SnapPointsAlignment_Far,
        &pSnapPoints,
        &cSnapPoints));

    IFC(ctl::make(&spSnapPointsVTV));
    IFC(spSnapPointsVTV->SetView(pSnapPoints, cSnapPoints));

    IFC(spSnapPointsVTV.CopyTo(ppValue));

Cleanup:
    delete [] pSnapPoints;
    RRETURN(hr);
}

_Check_return_
HRESULT VirtualizingStackPanel::AreScrollSnapPointsRegular(_Out_ BOOLEAN* pAreScrollSnapPointsRegular)
{
    HRESULT hr = S_OK;

    IFCPTR(pAreScrollSnapPointsRegular);

    // We use the VirtualizingStackPanel's AreScrollSnapPointsRegular property to answer the question.
    IFC(VirtualizingStackPanelGenerated::get_AreScrollSnapPointsRegular(pAreScrollSnapPointsRegular));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   VirtualizingStackPanel::GetIrregularSnapPoints
//
//  Synopsis:
//    Used to retrieve an array of irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT VirtualizingStackPanel::GetIrregularSnapPoints(
    _In_ BOOLEAN isForHorizontalSnapPoints,  // True when horizontal snap points are requested.
    _In_ BOOLEAN isForLeftAlignment,         // True when requested snap points will align to the left/top of the children
    _In_ BOOLEAN isForRightAlignment,        // True when requested snap points will align to the right/bottom of the children
    _Outptr_opt_result_buffer_(*pcSnapPoints) FLOAT** ppSnapPoints,   // Placeholder for returned array
    _Out_ UINT32* pcSnapPoints)                                   // Number of snap points returned
{
    HRESULT hr = S_OK;
    BOOLEAN isFirstChild = TRUE;
    FLOAT* pSnapPoints = NULL;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT childDim = 0.0;
    FLOAT cumulatedDim = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    FLOAT upperMarginSnapPointKey = 0.0;
    FLOAT irregularSnapPointKeysOffset = 0.0;
    UINT32 nCount = 0;
    UINT32 cSnapPoints = 0;
    INT32 cSnapPointKeys = 0;
    wf::Size childDesiredSize = {};
    xaml_controls::Orientation orientation;

    IFCEXPECT(ppSnapPoints);
    *ppSnapPoints = NULL;
    IFCEXPECT(pcSnapPoints);
    *pcSnapPoints = 0;

    IFC(VirtualizingStackPanelGenerated::get_Orientation(&orientation));

    if ((orientation == xaml_controls::Orientation_Vertical && !isForHorizontalSnapPoints) ||
        (orientation == xaml_controls::Orientation_Horizontal && isForHorizontalSnapPoints))
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
        BOOLEAN areScrollSnapPointsRegular = FALSE;

        IFC(VirtualizingStackPanelGenerated::get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
        if (areScrollSnapPointsRegular)
        {
            // Accessing the irregular snap points while AreScrollSnapPointsRegular is True is not supported.
            IFC(E_FAIL); // TODO: Replace with custom error code. Something similar to InvalidOperationException.
        }

        IFC(ResetSnapPointKeys());

        IFC(GetCommonSnapPointKeys(&lowerMarginSnapPointKey, &upperMarginSnapPointKey));

        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nCount));

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

        if (nCount > 0)
        {
            IFC(ComputeUnrealizedChildrenEstimatedDimension(cumulatedDim));

            irregularSnapPointKeysOffset = cumulatedDim;

            pSnapPoints = new FLOAT[nCount];

            pSnapPointKeys = new FLOAT[nCount];

            for (INT childIndex = m_iFirstVisibleChildIndex - m_iBeforeTrail;
                 childIndex < m_iFirstVisibleChildIndex + m_iVisibleCount;
                 childIndex++)
            {
                ctl::ComPtr<xaml::IUIElement> spChild;
                IFC(spRealizedChildren->GetAt(childIndex, &spChild));
                if (spChild)
                {
                    IFC(spChild->get_DesiredSize(&childDesiredSize));

                    if (isForLeftAlignment)
                    {
                        // Snap points are aligned to the left/top of the children
                        pSnapPoints[cSnapPoints] = cumulatedDim;
                    }

                    if (orientation == xaml_controls::Orientation_Vertical)
                    {
                        childDim = childDesiredSize.Height;
                    }
                    else
                    {
                        childDim = childDesiredSize.Width;
                    }

                    if (!isForLeftAlignment && !isForRightAlignment)
                    {
                        // Snap points are centered on the children
                        pSnapPoints[cSnapPoints] = cumulatedDim + childDim / 2;
                    }

                    cumulatedDim += childDim;

                    if (isForRightAlignment)
                    {
                        // Snap points are aligned to the right/bottom of the children
                        pSnapPoints[cSnapPoints] = cumulatedDim;
                    }

                    if (!(isForLeftAlignment && isFirstChild))
                    {
                        // Do not include the lower margin for the first child's snap point when the alignment is Near
                        pSnapPoints[cSnapPoints] += lowerMarginSnapPointKey;
                    }

                    isFirstChild = FALSE;

                    pSnapPointKeys[cSnapPointKeys] = childDim;

                    cSnapPoints++;
                    cSnapPointKeys++;
                }
            }

            if (cSnapPoints > 0)
            {
                *ppSnapPoints = pSnapPoints;
                *pcSnapPoints = cSnapPoints;
                pSnapPoints = NULL;
            }
        }

        if (cSnapPointKeys > 0)
        {
            m_pIrregularSnapPointKeys = pSnapPointKeys;
            m_cIrregularSnapPointKeys = cSnapPointKeys;
            m_irregularSnapPointKeysOffset = irregularSnapPointKeysOffset;
            m_bAreSnapPointsKeysHorizontal = isForHorizontalSnapPoints;
            pSnapPointKeys = NULL;
        }

        m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
        m_upperMarginSnapPointKey = upperMarginSnapPointKey;

        // Next snap point change needs to raise a notification
        if (isForHorizontalSnapPoints)
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
//  Function:   VirtualizingStackPanel::GetIrregularSnapPointKeys
//
//  Synopsis:
//    Determines the keys for irregular snap points.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT VirtualizingStackPanel::GetIrregularSnapPointKeys(
    _In_ xaml_controls::Orientation orientation,
    _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
    _In_ UINT32 nCount,
    _Outptr_result_buffer_(*pcSnapPointKeys) FLOAT** ppSnapPointKeys,
    _Out_ INT32* pcSnapPointKeys,
    _Out_ FLOAT* pSnapPointKeysOffset,
    _Out_ FLOAT* pLowerMarginSnapPointKey,
    _Out_ FLOAT* pUpperMarginSnapPointKey)
{
    HRESULT hr = S_OK;
    INT32 cSnapPointKeys = 0;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT snapPointKeysOffset = 0.0;
    wf::Size childDesiredSize = {};

    IFCEXPECT(ppSnapPointKeys);
    *ppSnapPointKeys = NULL;
    IFCEXPECT(pcSnapPointKeys);
    *pcSnapPointKeys = 0;
    IFCEXPECT(pSnapPointKeysOffset);
    *pSnapPointKeysOffset = 0.0;
    IFCEXPECT(pLowerMarginSnapPointKey);
    *pLowerMarginSnapPointKey = 0.0;
    IFCEXPECT(pUpperMarginSnapPointKey);
    *pUpperMarginSnapPointKey = 0.0;

    if (nCount > 0)
    {
        IFC(ComputeUnrealizedChildrenEstimatedDimension(snapPointKeysOffset));

        pSnapPointKeys = new FLOAT[nCount];

        for (INT childIndex = m_iFirstVisibleChildIndex - m_iBeforeTrail;
             childIndex < m_iFirstVisibleChildIndex + m_iVisibleCount;
             childIndex++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;
            IFCEXPECT(pRealizedChildren);
            IFC(pRealizedChildren->GetAt(childIndex, &spChild));
            if (spChild)
            {
                IFC(spChild->get_DesiredSize(&childDesiredSize));

                if (orientation == xaml_controls::Orientation_Vertical)
                {
                    pSnapPointKeys[cSnapPointKeys] = childDesiredSize.Height;
                }
                else
                {
                    pSnapPointKeys[cSnapPointKeys] = childDesiredSize.Width;
                }

                cSnapPointKeys++;
            }
        }

        if (cSnapPointKeys > 0)
        {
            *ppSnapPointKeys = pSnapPointKeys;
            *pcSnapPointKeys = cSnapPointKeys;
            *pSnapPointKeysOffset = snapPointKeysOffset;
            pSnapPointKeys = NULL;
        }
    }

    IFC(GetCommonSnapPointKeys(pLowerMarginSnapPointKey, pUpperMarginSnapPointKey));

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   VirtualizingStackPanel::NotifySnapPointsChanges
//
//  Synopsis:
//    Checks if the snap point keys have changed and a notification needs
//    to be raised.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT VirtualizingStackPanel::NotifySnapPointsChanges(
    _In_opt_ wfc::IVector<xaml::UIElement*>* pRealizedChildren,
    _In_ UINT32 nCount)
{
    HRESULT hr = S_OK;
    INT32 cSnapPointKeys = 0;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT snapPointKeysOffset = 0.0;
    FLOAT snapPointKey = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    FLOAT upperMarginSnapPointKey = 0.0;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    BOOLEAN notifyForHorizontalSnapPoints = FALSE;
    BOOLEAN notifyForVerticalSnapPoints = FALSE;
    xaml_controls::Orientation orientation;

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    IFC(AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    IFC(get_PhysicalOrientation(&orientation));

    if (orientation == xaml_controls::Orientation_Vertical)
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

    if ((m_bNotifyHorizontalSnapPointsChanges && orientation == xaml_controls::Orientation_Horizontal &&
         m_bAreSnapPointsKeysHorizontal && !m_bNotifiedHorizontalSnapPointsChanges) ||
        (m_bNotifyVerticalSnapPointsChanges && orientation == xaml_controls::Orientation_Vertical &&
         !m_bAreSnapPointsKeysHorizontal && !m_bNotifiedVerticalSnapPointsChanges))
    {
        if (m_regularSnapPointKey != -1.0)
        {
            if (areScrollSnapPointsRegular)
            {
                IFC(GetRegularSnapPointKeys(orientation, pRealizedChildren, nCount, &snapPointKey, &lowerMarginSnapPointKey, &upperMarginSnapPointKey));
                if (m_regularSnapPointKey != snapPointKey ||
                    m_lowerMarginSnapPointKey != lowerMarginSnapPointKey ||
                    m_upperMarginSnapPointKey != upperMarginSnapPointKey)
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
                IFC(GetIrregularSnapPointKeys(orientation, pRealizedChildren, nCount, &pSnapPointKeys, &cSnapPointKeys, &snapPointKeysOffset, &lowerMarginSnapPointKey, &upperMarginSnapPointKey));
                if (m_cIrregularSnapPointKeys != cSnapPointKeys ||
                    m_irregularSnapPointKeysOffset != snapPointKeysOffset ||
                    m_lowerMarginSnapPointKey != lowerMarginSnapPointKey ||
                    m_upperMarginSnapPointKey != upperMarginSnapPointKey)
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
//  Function:   VirtualizingStackPanel::RefreshIrregularSnapPointKeys
//
//  Synopsis:
//    Refreshes the m_pIrregularSnapPointKeys/m_cIrregularSnapPointKeys
//    fields based on all children.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT VirtualizingStackPanel::RefreshIrregularSnapPointKeys()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    INT32 cSnapPointKeys = 0;
    FLOAT* pSnapPointKeys = NULL;
    FLOAT irregularSnapPointKeysOffset = 0.0;
    FLOAT lowerMarginSnapPointKey = 0.0;
    FLOAT upperMarginSnapPointKey = 0.0;
    UINT32 nCount = 0;
    xaml_controls::Orientation orientation;

#ifdef DBG
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    IFC(VirtualizingStackPanelGenerated::get_AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));
    ASSERT(!areScrollSnapPointsRegular);
#endif

    IFC(VirtualizingStackPanelGenerated::get_Orientation(&orientation));

    IFC(ResetSnapPointKeys());

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

#if DBG
    IFC(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    IFC(GetIrregularSnapPointKeys(
        orientation,
        spRealizedChildren.Get(),
        nCount,
        &pSnapPointKeys,
        &cSnapPointKeys,
        &irregularSnapPointKeysOffset,
        &lowerMarginSnapPointKey,
        &upperMarginSnapPointKey));

    m_pIrregularSnapPointKeys = pSnapPointKeys;
    m_cIrregularSnapPointKeys = cSnapPointKeys;
    m_irregularSnapPointKeysOffset = irregularSnapPointKeysOffset;
    m_lowerMarginSnapPointKey = lowerMarginSnapPointKey;
    m_upperMarginSnapPointKey = upperMarginSnapPointKey;
    m_bAreSnapPointsKeysHorizontal = (orientation == xaml_controls::Orientation_Horizontal);
    pSnapPointKeys = NULL;

Cleanup:
    delete [] pSnapPointKeys;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   VirtualizingStackPanel::ResetSnapPointKeys
//
//  Synopsis:
//    Resets both regular and irregular snap point keys.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT VirtualizingStackPanel::ResetSnapPointKeys()
{
    delete [] m_pIrregularSnapPointKeys;
    m_pIrregularSnapPointKeys = NULL;
    m_cIrregularSnapPointKeys = -1;
    m_irregularSnapPointKeysOffset = 0.0f;
    RRETURN(OrientedVirtualizingPanel::ResetSnapPointKeys());
}

//-------------------------------------------------------------------------
//
//  Function:   VirtualizingStackPanel::SetSnapPointsChangeNotificationsRequirement
//
//  Synopsis:
//    Determines whether the VirtualizingStackPanel must call NotifySnapPointsChanged
//    when snap points change or not.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT VirtualizingStackPanel::SetSnapPointsChangeNotificationsRequirement(
    _In_ BOOLEAN isForHorizontalSnapPoints,
    _In_ BOOLEAN notifyChanges)
{
    HRESULT hr = S_OK;
    BOOLEAN areScrollSnapPointsRegular = FALSE;
    xaml_controls::Orientation orientation;

    IFC(get_PhysicalOrientation(&orientation));
    IFC(AreScrollSnapPointsRegular(&areScrollSnapPointsRegular));

    if (isForHorizontalSnapPoints)
    {
        m_bNotifyHorizontalSnapPointsChanges = notifyChanges;
        if (orientation == xaml_controls::Orientation_Horizontal && notifyChanges)
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
        if (orientation == xaml_controls::Orientation_Vertical && notifyChanges)
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

// Handle the custom property changed event and call the
// OnPropertyChanged2 methods.
_Check_return_
HRESULT
VirtualizingStackPanel::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(VirtualizingStackPanelGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::VirtualizingStackPanel_Orientation:
            IFC(ResetScrolling());
            break;

        case KnownPropertyIndex::VirtualizingStackPanel_AreScrollSnapPointsRegular:
            IFC(OnAreScrollSnapPointsRegularChanged());
            break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VirtualizingStackPanel::OnCleanUpVirtualizedItemImpl(_In_ xaml_controls::ICleanUpVirtualizedItemEventArgs* e)
{
    HRESULT hr = S_OK;
    CleanUpVirtualizedItemEventEventSourceType* pEventSource = nullptr;

    IFC(GetCleanUpVirtualizedItemEventEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), e));

Cleanup:
    RRETURN(hr);
}

// General VirtualizingStackPanel layout behavior is to grow unbounded in the "stacking" direction (Size To Content).
// Children in this dimension are encouraged to be as large as they like.  In the other dimension,
// VirtualizingStackPanel will assume the maximum size of its children.
// When scrolling, VirtualizingStackPanel will not grow in layout size but effectively add the children on a z-plane which
// will probably be clipped by some parent (typically a ScrollContentPresenter) to Stack's size.
IFACEMETHODIMP
VirtualizingStackPanel::MeasureOverride(
    // Measurement constraints, a control cannot return a size larger than the
    // constraint.
    _In_ wf::Size availableSize,
    // The desired size of the control.
    _Out_ wf::Size* returnValue)
{
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl;

    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    wf::Size stackDesiredSize = {};
    wf::Size layoutSlotSize = availableSize;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN isHorizontal = FALSE;
    DOUBLE firstItemOffset = 0.0;       // Offset of the top of the first child relative to the top of the viewport.
    INT lastViewport = -1;            // Last child index in the viewport.  -1 indicates we have not yet iterated through the last child.
    DOUBLE logicalVisibleSpace = 0;
    xaml_primitives::GeneratorPosition startPos = {-1, 0};
    INT32 tick = 0;
    FLOAT zoomFactor = 1.0;
    INT maxNumOfItemsToGenerateInCurrentMeasureCycle;
    BOOLEAN bIsInDMZoom = FALSE;
    DOUBLE viewPortSizeInPixels = 0;

    auto guard = wil::scope_exit([this]()
    {
        m_bInMeasure = FALSE;
        m_isGeneratingNewContainers = FALSE;
    });

    m_bInMeasure = TRUE;
    IFCPTR_RETURN(returnValue);

    IFC_RETURN(get_Orientation(&orientation));
    isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    // Collect information from the ItemsControl, if there is one.
    IFC_RETURN(ItemsControl::GetItemsOwner(this, &spItemsControl));

    if (spItemsControl)
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
        IFC_RETURN(get_ItemContainerGenerator(&spGenerator));
        IFC_RETURN(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spItems));
        IFC_RETURN(spItems->get_Size(&m_nItemsCount));
    }
    else
    {
        m_nItemsCount = 0;
    }

    m_lineCount = m_nItemsCount;

    IFC_RETURN(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(static_cast<UIElement*>(this)->GetHandle()), (XINT16*)&tick));
    // the tick was stored by itemscontrol while setting up this itemshost
    // it represents the layouttick in which all containers should be considered to have been 'loaded' on the screen
    m_isGeneratingNewContainers = tick == GetItemsHostValidatedTick();

    IFC_RETURN(SetVirtualizationState(spItemsControl.Get()));

    BOOLEAN isScrolling = FALSE;
    IFC_RETURN(get_IsScrolling(&isScrolling));
    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    //
    // Initialize child sizing and iterator data
    // Allow children as much size as they want along the stack.
    //
    if (isHorizontal)
    {
        // Apply zoom factor
        availableSize.Width = availableSize.Width / zoomFactor;
        layoutSlotSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
        if (isScrolling)
        {
            BOOLEAN canVerticallyScroll = FALSE;
            IFC_RETURN(get_CanVerticallyScroll(&canVerticallyScroll));
            if (canVerticallyScroll)
            {
                layoutSlotSize.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
        }
        logicalVisibleSpace = availableSize.Width;
        stackDesiredSize.Height = !DoubleUtil::IsInfinity(availableSize.Height) ?  availableSize.Height : 0;
    }
    else
    {
        // Apply zoom factor
        availableSize.Height = availableSize.Height / zoomFactor;
        layoutSlotSize.Height = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
        if (isScrolling)
        {
            BOOLEAN canHorizontallyScroll = FALSE;
            IFC_RETURN(get_CanHorizontallyScroll(&canHorizontallyScroll));
            if (canHorizontallyScroll)
            {
                layoutSlotSize.Width = static_cast<FLOAT>(DoubleUtil::PositiveInfinity);
            }
        }
        logicalVisibleSpace = availableSize.Height;
        stackDesiredSize.Width = !DoubleUtil::IsInfinity(availableSize.Width) ?  availableSize.Width : 0;
    }

    if (availableSize.Height < m_LastSetAvailableSize.Height || availableSize.Width < m_LastSetAvailableSize.Width)
    {
        // Reset the _maxDesiredSize cache if available size reduces from last available size.
        wf::Size empty = {};
        m_ScrollData.m_MaxDesiredSize = empty;
    }

    m_LastSetAvailableSize = availableSize;
    m_LastSetChildLayoutSlotSize = layoutSlotSize;

    // setting preCachedSize to logicalVisibleSpace to cache 1 page on both sides
    //m_dPrecacheWindowSize = logicalVisibleSpace;

    // Compute index of first item in the viewport
    m_iVisibleStart = ComputeIndexOfFirstVisibleItem(isHorizontal, firstItemOffset);

    // If recycling clean up before generating children.
    if (m_bIsVirtualizing)
    {
        IFC_RETURN(CleanupContainers(spItemsControl.Get(), availableSize));
#if DBG
        if (InRecyclingMode())
        {
            IFC_RETURN(debug_VerifyRealizedChildren());
        }
#endif
    }

    //If this measure requires ensuring a particular item is in View, we need to correct offset and m_iVisibleStart
    if (m_IndexToEnsureInView >= 0)
    {
        IFC_RETURN(CorrectOffsetForScrollIntoView(isHorizontal ? availableSize.Width : availableSize.Height, isHorizontal, spItemsControl.Get(), layoutSlotSize, firstItemOffset));
        m_IndexToEnsureInView = -1;
    }

//    if (_horizontalFlickSB == null)
//    {
//        TranslateTransform.X = 0;
//    }
//
//    if (_verticalFlickSB == null)
//    {
//        TranslateTransform.Y = 0;
//    }

    // Restrict number of items being generated to the number of items generated in previous cycle if we are currently zooming.
    IFC_RETURN(IsInDirectManipulationZoom(bIsInDMZoom));
    maxNumOfItemsToGenerateInCurrentMeasureCycle = bIsInDMZoom ? m_iVisibleCount + m_iBeforeTrail : m_nItemsCount;

    // Validate the m_iVisibleStart index value whether it is on the valid range of ItemContainerGenerator's item map.
    // Some apps(Music.UI.exe) stress run hits the accessing invalid index since ItemContainerGenerator isn't ready
    // about having the valid item map block.
    if (spGenerator.Cast<ItemContainerGenerator>()->IsIndexInRange(m_iVisibleStart))
    {
        //
        // Figure out the position of the first visible item
        //
        IFC_RETURN(IndexToGeneratorPositionForStart(spItemsControl.Get(), m_iVisibleStart, m_iFirstVisibleChildIndex, startPos));
    }

    //
    // Main loop: generate and measure all children (or all visible children if virtualizing).
    //
    BOOLEAN fillRemainingSpace = TRUE;
    BOOLEAN visualOrderChanged = FALSE;
    m_iVisibleCount = 0;
    m_iBeforeTrail = 0;
    m_iAfterTrail = 0;
    m_dPrecacheAfterTrailSize = 0;

    if (m_nItemsCount > 0)
    {
        IFC_RETURN(spGenerator->StartAt(startPos, xaml_primitives::GeneratorDirection_Forward, TRUE));
        INT childIndex = m_iFirstVisibleChildIndex;

        for (INT i = m_iVisibleStart; i < static_cast<INT>(m_nItemsCount); ++i)
        {
            wf::Size childDesiredSize = {};
            DOUBLE childLogicalSize = 0.0;
            ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
            ctl::ComPtr<xaml::IUIElement> spUIChild;

            // Get next child.
            BOOLEAN newlyRealized = FALSE;
            IFC_RETURN(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));

            if (!ctl::is<xaml::IUIElement>(spChildAsDO.Get()))
            {
                ASSERT(!newlyRealized, L"The generator realized a null value.");
                // We reached the end of the items (because of a group)
                break;
            }

            IFC_RETURN(spChildAsDO.As(&spUIChild));
            IFC_RETURN(AddContainerFromGenerator(spItemsControl.Get(), childIndex++, spUIChild.Get(), newlyRealized, visualOrderChanged));
            IFC_RETURN(MeasureChild(spUIChild.Get(), layoutSlotSize, &childDesiredSize));

            m_iVisibleCount++;

            // Accumulate child size.
            if (isHorizontal)
            {
                if (i == m_iVisibleStart)
                {
                    childLogicalSize = childDesiredSize.Width * (1 - firstItemOffset);
                    m_dPrecacheBeforeTrailSize = childDesiredSize.Width * firstItemOffset;
                }
                else
                {
                    childLogicalSize = childDesiredSize.Width;
                }

                stackDesiredSize.Width += static_cast<FLOAT>(childLogicalSize);
                stackDesiredSize.Height = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Height, childDesiredSize.Height));
            }
            else
            {
                if (i == m_iVisibleStart)
                {
                    childLogicalSize = childDesiredSize.Height * (1 - firstItemOffset);
                    m_dPrecacheBeforeTrailSize = childDesiredSize.Height * firstItemOffset;
                }
                else
                {
                    childLogicalSize = childDesiredSize.Height;
                }

                stackDesiredSize.Height += static_cast<FLOAT>(childLogicalSize);
                stackDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Width, childDesiredSize.Width));
            }

            // Adjust remaining viewport space if we are scrolling and within the viewport region.
            // While scrolling (not virtualizing), we always measure children before and after the viewport.
            if (isScrolling && lastViewport == -1)
            {
                logicalVisibleSpace -= childLogicalSize;
                if (DoubleUtil::LessThanOrClose(logicalVisibleSpace, 0.0))
                {
                    lastViewport = i;
                    m_dPrecacheAfterTrailSize = -logicalVisibleSpace;
                }
            }

            // When under a viewport, virtualizing and at or beyond the first element, stop creating elements when out of space.
            if (m_bIsVirtualizing)
            {
                DOUBLE viewportSize = 0.0;
                DOUBLE totalGenerated = 0.0;

                if (isHorizontal)
                {
                    viewportSize = availableSize.Width;
                    totalGenerated = stackDesiredSize.Width;
                }
                else
                {
                    viewportSize = availableSize.Height;
                    totalGenerated = stackDesiredSize.Height;
                }

                if (m_iVisibleCount + m_iBeforeTrail >= maxNumOfItemsToGenerateInCurrentMeasureCycle)
                {
                    // By default we do not need to fill remaining space in this case since this will be true
                    // 1. We have less than viewport number of items.
                    // 2. Or we are zooming and want to restrict to previously measured number of items.
                    fillRemainingSpace = FALSE;

                    // If we are starting with non-zero fractional offset (which means m_dPrecacheBeforeTrailSize > 0)
                    // And totalGenerated is less than viewport size we should adjust offset and try filling remaining space.
                    if (DoubleUtil::GreaterThan(m_dPrecacheBeforeTrailSize, 0) && DoubleUtil::LessThan(totalGenerated, viewportSize))
                    {
                        fillRemainingSpace = TRUE;
                    }

                    break;
                }

                if (DoubleUtil::LessThanOrClose(viewportSize, totalGenerated))
                {
                    // Either we passed the limit or the child was focusable
                    fillRemainingSpace = FALSE;

                    if (lastViewport != -1 && lastViewport != i)
                    {
                        m_iAfterTrail++;
                        m_dPrecacheAfterTrailSize += childLogicalSize;
                    }

                    // The end of this child is outside the viewport.  Check if we want to generate some more.
                    // We should have at least 1 item after the view
                    if (m_iAfterTrail > 0 && m_dPrecacheAfterTrailSize >= m_dPrecacheWindowSize - logicalVisibleSpace)
                    {
                        break;
                    }
                }
            }
            // Loop around and generate another item
        }
        IFC_RETURN(spGenerator->Stop());
    }

#if DBG
    if (m_bIsVirtualizing && InRecyclingMode())
    {
        IFC_RETURN(debug_VerifyRealizedChildren());
    }
#endif
    //
    // Adjust the scroll offset, extent, etc.
    //
    if (isScrolling)
    {
        // Compute the extent before we fill remaining space and modify the stack desired size
        wf::Size extent = {};
        IFC_RETURN(ComputeLogicalExtent(stackDesiredSize, isHorizontal, extent));

        if (fillRemainingSpace)
        {
            // If we or children have resized, it's possible that we can now display more content.
            // This is true if we started at a nonzero offset and still have space remaining.
            // In this case, we loop back through previous children until we run out of space.
            IFC_RETURN(FillRemainingSpace(spItemsControl.Get(), logicalVisibleSpace, stackDesiredSize, layoutSlotSize, isHorizontal, maxNumOfItemsToGenerateInCurrentMeasureCycle));
        }

        DOUBLE preCacheBuffer = 0;
        if (m_bShouldMeasureBuffers)
        {
            preCacheBuffer = m_dPrecacheWindowSize + DoubleUtil::Max(0, m_dPrecacheWindowSize - m_dPrecacheAfterTrailSize);
        }

        IFC_RETURN(GeneratePreviousItems(spItemsControl.Get(), preCacheBuffer, stackDesiredSize, layoutSlotSize, isHorizontal, FALSE, maxNumOfItemsToGenerateInCurrentMeasureCycle));

        DOUBLE unusedBuffer = DoubleUtil::Max(0, m_dPrecacheWindowSize - m_dPrecacheBeforeTrailSize);
        INT firstUnrealizedContainerIndex = MAX(0, m_iVisibleStart + m_iVisibleCount);   // beforeTrail is not included in m_iVisibleCount

        if (m_bShouldMeasureBuffers && unusedBuffer > 0 && firstUnrealizedContainerIndex < static_cast<INT>(m_nItemsCount) &&
            spGenerator.Cast<ItemContainerGenerator>()->IsIndexInRange(firstUnrealizedContainerIndex))
        {
            INT childIndex = 0;
            IFC_RETURN(IndexToGeneratorPositionForStart(spItemsControl.Get(), firstUnrealizedContainerIndex, childIndex, startPos));

            IFC_RETURN(spGenerator->StartAt(startPos, xaml_primitives::GeneratorDirection_Forward, TRUE));
            while (unusedBuffer > 0 && firstUnrealizedContainerIndex++ < static_cast<INT>(m_nItemsCount)
                && (m_iVisibleCount + m_iBeforeTrail < maxNumOfItemsToGenerateInCurrentMeasureCycle))
            {
                wf::Size childDesiredSize = {};
                DOUBLE childLogicalSize = 0.0;
                ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;
                ctl::ComPtr<xaml::IUIElement> spUIChild;

                // Get next child.
                BOOLEAN newlyRealized = FALSE;
                IFC_RETURN(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));

                if (!ctl::is<xaml::IUIElement>(spChildAsDO.Get()))
                {
                    ASSERT(!newlyRealized, L"The generator realized a null value.");
                    // We reached the end of the items (because of a group)
                    break;
                }

                IFC_RETURN(spChildAsDO.As(&spUIChild));
                IFC_RETURN(AddContainerFromGenerator(spItemsControl.Get(), childIndex++, spUIChild.Get(), newlyRealized, visualOrderChanged));
                IFC_RETURN(MeasureChild(spUIChild.Get(), layoutSlotSize, &childDesiredSize));

                m_iVisibleCount++;
                m_iAfterTrail++;

                // Accumulate child size.
                if (isHorizontal)
                {
                    childLogicalSize = childDesiredSize.Width;
                    stackDesiredSize.Width += static_cast<FLOAT>(childLogicalSize);
                    stackDesiredSize.Height = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Height, childDesiredSize.Height));
                }
                else
                {
                    childLogicalSize = childDesiredSize.Height;
                    stackDesiredSize.Height += static_cast<FLOAT>(childLogicalSize);
                    stackDesiredSize.Width = static_cast<FLOAT>(DoubleUtil::Max(stackDesiredSize.Width, childDesiredSize.Width));
                }
                m_dPrecacheAfterTrailSize += childLogicalSize;
                unusedBuffer -= childLogicalSize;

                // Loop around and generate another item
            }
            IFC_RETURN(spGenerator->Stop());
        }

        // Compute Scrolling data such as extent, viewport, and offset.
        IFC_RETURN(UpdateLogicalScrollData(stackDesiredSize, availableSize, logicalVisibleSpace,
                                    extent, lastViewport, isHorizontal));
    }

    if (orientation == xaml_controls::Orientation_Horizontal)
    {
        IFC_RETURN(get_ActualWidth(&viewPortSizeInPixels));
    }
    else
    {
        IFC_RETURN(get_ActualHeight(&viewPortSizeInPixels));
    }

    if (!DoubleUtil::AreClose(m_viewPortSizeInPixels, viewPortSizeInPixels))
    {
        m_viewPortSizeInPixels = viewPortSizeInPixels;
        IFC_RETURN(OnScrollChange());
    }

    //
    // Cleanup items no longer in the viewport
    //
    if (m_bIsVirtualizing)
    {
        IFC_RETURN(CleanupContainers(spItemsControl.Get(), availableSize));

        if (InRecyclingMode())
        {
            IFC_RETURN(CollectRecycledContainers());
        }
    }

#if DBG
    IFC_RETURN(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    if (isHorizontal)
    {
        stackDesiredSize.Width *= zoomFactor;
    }
    else
    {
        stackDesiredSize.Height *= zoomFactor;
    }

    *returnValue = stackDesiredSize;

    return S_OK;
}

// Provides the behavior for the Arrange pass of layout.  Classes
// can override this method to define their own Arrange pass
// behavior.
IFACEMETHODIMP VirtualizingStackPanel::ArrangeOverride(
    // The computed size that is used to arrange the content.
    _In_ wf::Size arrangeSize,
    // The size of the control.
    _Out_ wf::Size* returnValue)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    wf::Rect rcChild = {0, 0, arrangeSize.Width, arrangeSize.Height};
    DOUBLE previousChildSize = 0.0;
    FLOAT zoomFactor = 1.0;
    wf::Rect contentBounds = {static_cast<FLOAT>(DoubleUtil::PositiveInfinity), static_cast<FLOAT>(DoubleUtil::PositiveInfinity), 0, 0};

    auto guard = wil::scope_exit([this]()
    {
        // in vsp, a scroll will trigger layout which will trigger transitions
        // that scroll has now been processed, so new transitions are allowed to be created
        VERIFYHR(put_IsIgnoringTransitions(FALSE));
    });

    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    IFC_RETURN(get_Orientation(&orientation));
    const bool isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    //ScrollVector currentTransform = {TranslateTransform.X, TranslateTransform.Y};
    m_ScrollData.m_ArrangedOffset = m_ScrollData.m_ComputedOffset;

//#if DBG
//       WCHAR szValue2[250];
//       IFCEXPECT_RETURN(swprintf_s(szValue2, 250, L"m_ScrollData.m_ArrangedOffset.X - %f m_ScrollData.m_ArrangedOffset.Y - %f m_iCacheStart - %d",
//            m_ScrollData.m_ArrangedOffset.X,
//            m_ScrollData.m_ArrangedOffset.Y,
//            m_iCacheStart) >= 0);
//        Trace(szValue2);
//#endif

    FLOAT estimatedOffset = 0.0;
    IFC_RETURN(ComputeUnrealizedChildrenEstimatedDimension(estimatedOffset));

    if (isHorizontal)
    {
        rcChild.X = estimatedOffset;
    }
    else
    {
        rcChild.Y = estimatedOffset;
    }

    // The area in which the items are arranged
    m_arrangedItemsRect.X = rcChild.X;
    m_arrangedItemsRect.Y = rcChild.Y;

    //
    // Arrange and Position Children.
    //
#if DBG
    IFC_RETURN(debug_AssertRealizedChildrenEqualVisualChildren());
#endif

    UINT nCount = 0;
    BOOLEAN hasAccountedForEdge = FALSE;
    BOOLEAN isIgnoring = FALSE;
    FLOAT startDeltaOfNewElements = 0.0f;

    IFC_RETURN(get_IsIgnoringTransitions(&isIgnoring));
    if (!isIgnoring && m_bInManipulation)
    {
        // make sure not to setup transitions during DM
        IFC_RETURN(put_IsIgnoringTransitions(TRUE));
        isIgnoring = TRUE;
    }

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

//#if DBG
//       WCHAR szValue[250];
//       IFCEXPECT_RETURN(swprintf_s(szValue, 250, L"m_iBTrail - %d m_iATrail - %d Count -%d m_iVisibleStart -%d FVCI -%d m_iVisibleCount -%d",
//            m_iBeforeTrail,
//            m_iAfterTrail,
//            nCount,
//            m_iVisibleStart,
//            m_iFirstVisibleChildIndex,
//            m_iVisibleCount) >= 0);
//       Trace(szValue);
//
//        IFCEXPECT_RETURN(swprintf_s(szValue, 250, L"rcChild.X - %f rcChild.Y - %f m_bInDirectManipulation - %d",
//            rcChild.X,
//            rcChild.Y,
//            m_bInDirectManipulation) >= 0);
//        Trace(szValue);
//#endif
    for (UINT i = 0; i < nCount; ++i)
    {
        // we are looping through the actual containers; the visual children of this panel.
        ctl::ComPtr<xaml::IUIElement> spChild;
        UIElement* pChildAsUINoRef;
        wf::Size childSize = {};
        bool isFocusChild = false;

        IFC_RETURN(spRealizedChildren->GetAt(i, &spChild));
        IFC_RETURN(spChild->get_DesiredSize(&childSize));

        pChildAsUINoRef = static_cast<UIElement*>(spChild.Get());

        if (isHorizontal)
        {
            rcChild.X += static_cast<FLOAT>(previousChildSize);
            previousChildSize = childSize.Width;
            rcChild.Width = static_cast<FLOAT>(previousChildSize);
            rcChild.Height = static_cast<FLOAT>(DoubleUtil::Max(arrangeSize.Height, childSize.Height));
        }
        else
        {
            rcChild.Y += static_cast<FLOAT>(previousChildSize);
            previousChildSize = childSize.Height;
            rcChild.Height = static_cast<FLOAT>(previousChildSize);
            rcChild.Width = static_cast<FLOAT>(DoubleUtil::Max(arrangeSize.Width, childSize.Width));
        }

    //    TranslateTransform tChild = container.RenderTransform as TranslateTransform;

    //    if (_horizontalFlickSB != null || _verticalFlickSB != null)
    //    {
    //        if (tChild == null)
    //        {
    //            tChild = new TranslateTransform();
    //            container.RenderTransform = tChild;
    //        }

    //        tChild.X = -currentTransform.X;
    //        tChild.Y = -currentTransform.Y;
    //    }
    //    else if (tChild != null)
    //    {
    //        tChild.X = 0;
    //        tChild.Y = 0;
    //    }
        {
            if (!isIgnoring)
            {
                BOOLEAN isLocationValid = pChildAsUINoRef->GetIsLocationValid();
                if (!isLocationValid)
                {
                    BOOLEAN wasInserted = FALSE;
                    IFC_RETURN(OrientedVirtualizingPanelFactory::GetIsContainerGeneratedForInsertStatic(spChild.Get(), &wasInserted));
                    if (!wasInserted)
                    {
                        // this will be a load transition from the border of the screen
                        if (!hasAccountedForEdge)
                        {
                            if (isHorizontal)
                            {
                                startDeltaOfNewElements = m_newItemsStartPosition - rcChild.X;
                            }
                            else
                            {
                                startDeltaOfNewElements = m_newItemsStartPosition - rcChild.Y;
                            }
                            hasAccountedForEdge = TRUE;
                        }
                        IFC_RETURN(CoreImports::UIElement_SetIsEntering(static_cast<CUIElement*>(pChildAsUINoRef->GetHandle()), FALSE));

                        IFC_RETURN(CoreImports::UIElement_SetCurrentTransitionLocation(
                            static_cast<CUIElement*>(pChildAsUINoRef->GetHandle()),
                            isHorizontal ? rcChild.X + startDeltaOfNewElements : rcChild.X,
                            isHorizontal ? rcChild.Y : rcChild.Y + startDeltaOfNewElements,
                            rcChild.Width,
                            rcChild.Height));
                    }
                    else
                    {
                        // we'll simply check if this element was place outside of the view, in which case we optimize by not starting a transition at all.
                        BOOLEAN isOutsideWindowLocationBased = rcChild.X < (-1 * rcChild.Width) || rcChild.Y < (-1 * rcChild.Height) || rcChild.X > arrangeSize.Width || rcChild.Y > arrangeSize.Height;

                        // if we are arranging an element inside of the viewwindow, we wish it to potentially do a load transition
                        // for that to happen we fake the entered state, by saying it just 'entered' the visual tree, even though virtualization
                        // might have re-used an element that was already in the visual tree.
                        //
                        // if we are arranging outside of the viewwindow, we just want an immediate move, so set the enter to have occurred before this moment
                        // and set the new location before the arrange comes in. When the arrange comes to this new location, no delta will be there so no
                        // transition will have to be setup.
                        IFC_RETURN(CoreImports::UIElement_SetIsEntering(static_cast<CUIElement*>(pChildAsUINoRef->GetHandle()), !isOutsideWindowLocationBased));

                        // now that it has entered, the start location needs to be set. this corresponds to the currentOffset and the nextGenerationOffset in the
                        // layouttransition storage. A load transition would then be calculated from this point, instead of its old location in the visual tree
                        IFC_RETURN(CoreImports::UIElement_SetCurrentTransitionLocation(static_cast<CUIElement*>(pChildAsUINoRef->GetHandle()), rcChild.X, rcChild.Y, rcChild.Width, rcChild.Height));
                    }
                }
            }
        }

        isFocusChild = (m_iFirstVisibleChildIndex - m_iBeforeTrail > static_cast<INT>(i)) ||
                       (m_iVisibleCount + m_iFirstVisibleChildIndex -1 < static_cast<INT>(i));

        // If Manipulation is ON, we don't want the focus item to be visible, otherwise we end up seeing it while panning.
        // but since it is one of the realized item, we need to arrange it.
        // Drawing before trail items outside layout area
        // and same for after Trail items
        if (isFocusChild)
        {
            wf::Rect focusChild = rcChild;
            previousChildSize = 0;

            // Arranging elements at the FLT_MAX could cause issues with
            // graphics stack. For e.g. anything greater than 1<<21 may cause
            // issues with D2D glyph computations. Hence we arrange the focus child at a
            // large offset but not at FLT_MAX.
            focusChild.X = VirtualizingPanel::ExtraContainerArrangeOffset;
            focusChild.Y = VirtualizingPanel::ExtraContainerArrangeOffset;

            IFC_RETURN(spChild->Arrange(focusChild));
        }
        else
        {
            IFC_RETURN(spChild->Arrange(rcChild));
            if (isHorizontal)
            {
                m_arrangedItemsRect.Height = MAX(m_arrangedItemsRect.Height, rcChild.Height);
            }
            else
            {
                m_arrangedItemsRect.Width = MAX(m_arrangedItemsRect.Width, rcChild.Width);
            }
        }

        if (!isFocusChild && EventEnabledVirtualizedCollectionUpdatedInfo())
        {
            contentBounds.X = static_cast<FLOAT>(DoubleUtil::Min(rcChild.X, contentBounds.X));
            contentBounds.Y = static_cast<FLOAT>(DoubleUtil::Min(rcChild.Y, contentBounds.Y));
            contentBounds.Width = static_cast<FLOAT>(DoubleUtil::Max(rcChild.X + rcChild.Width - contentBounds.X, contentBounds.Width));
            contentBounds.Height = static_cast<FLOAT>(DoubleUtil::Max(rcChild.Y + rcChild.Height - contentBounds.Y, contentBounds.Height));
        }

        pChildAsUINoRef->SetIsLocationValid(true);
    }

    m_newItemsStartPosition = isHorizontal ? rcChild.X + rcChild.Width : rcChild.Y + rcChild.Height;    // todo: or the bottom if not enough elements.

    // Arrange end
    if (isHorizontal)
    {
        m_arrangedItemsRect.Width = rcChild.X + rcChild.Width - m_arrangedItemsRect.X;
    }
    else
    {
        m_arrangedItemsRect.Height = rcChild.Y + rcChild.Height - m_arrangedItemsRect.Y;
    }

    // This method arranges the extra visual children which are not part of realized children
    // It arrage them out of viweport so it is not visible
    IFC_RETURN(ArrangeExtraContainers(isHorizontal));


    // Snap point might have changed, which might require to raise an event.
    IFC_RETURN(NotifySnapPointsChanges(spRealizedChildren.Get(), nCount));
    IFC_RETURN(SetupItemBoundsClip());

    if (!m_bShouldMeasureBuffers)
    {
        IFC_RETURN(SetFillBuffersTimer());
    }

    if (m_bNotifyLayoutRefresh)
    {
        ctl::ComPtr<IScrollOwner> spOwner;
        IFC_RETURN(m_ScrollData.get_ScrollOwner(&spOwner));

        m_bNotifyLayoutRefresh = FALSE;

        if (spOwner)
        {
            IFC_RETURN(spOwner->NotifyLayoutRefreshed());
        }
    }

    IFC_RETURN(RaiseVirtualizedCollectionUpdatedEvent(contentBounds));

    *returnValue = arrangeSize;

    return S_OK;
}

// Immediately cleans up any containers that have gone offscreen.  Called by MeasureOverride.
// When recycling this runs before generating and measuring children; otherwise it runs after.
_Check_return_
HRESULT
VirtualizingStackPanel::CleanupContainers(
    _In_ xaml_controls::IItemsControl* pItemsControl,
    _In_ wf::Size constraint)
{
    ctl::ComPtr<xaml_controls::IListViewBase> spListViewBase;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl = pItemsControl;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

    xaml_primitives::GeneratorPosition startPos = {-1, 0};

    ASSERT(m_bIsVirtualizing, L"Can't clean up containers if not virtualizing");
    ASSERT(pItemsControl, L"We can't cleanup if we aren't the itemshost");

    //
    // It removes items outside of the container cache window (a logical 'window' at
    // least as large as the viewport).
    //
    // firstViewport is the index of first data item that will be in the viewport
    // at the end of Measure.  This is effectively the scroll offset.
    //
    // m_iVisibleStart is index of the first data item that was previously at the top of the viewport
    // At the end of a Measure pass m_iVisibleStart == firstViewport.
    //
    // m_iVisibleCount is the number of data items that were previously visible in the viewport.

    INT cleanupRangeStart = -1;
    INT cleanupCount = 0;
    INT itemIndex = -1;              // data item index used to compare with the cache window position.
    INT lastItemIndex = -1;
    INT focusedChild = -1, previousFocusable = -1, nextFocusable = -1;  // child indices for the focused item and before and after focus trail items

    BOOLEAN performCleanup = FALSE;

    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));

#if DBG
    //Trace(L"==============================Cleanup containers========================================");
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
#endif

    UINT nCount = 0;
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

    if (nCount == 0)
    {
        return S_OK;// nothing to do
    }

    AdjustCacheWindow();

    IFC_RETURN(FindFocusedChildInRealizedChildren(focusedChild, previousFocusable, nextFocusable));

    BOOLEAN bIsIgnoringTransitions = FALSE;
    IFC_RETURN(get_IsIgnoringTransitions(&bIsIgnoringTransitions));

    //
    // Iterate over all realized children and recycle the ones that are eligible.  Items NOT eligible for recycling
    // have one or more of the following properties
    //
    //  - inside the cache window
    //  - the item is its own container
    //  - has keyboard focus
    //  - is the first focusable item before or after the focused item
    //  - the CleanupVirtualizedItem event was canceled
    //  - is the primary dragged item.
    //

    spListViewBase = spItemsControl.AsOrNull<xaml_controls::IListViewBase>();
    for (UINT childIndex = 0; childIndex < nCount; childIndex++)
    {
        lastItemIndex = itemIndex;
        IFC_RETURN(GetGeneratedIndex(childIndex, pItemsControl, itemIndex));

        if (itemIndex - lastItemIndex != 1)
        {
            // There's a generated gap between the current item and the last.  Clean up the last range of items.
            performCleanup = TRUE;
        }

        if (performCleanup)
        {
            if (cleanupRangeStart >= 0 && cleanupCount > 0)
            {
                //
                // We've hit a non-virtualizable container or a non-contiguous section.
                //

                IFC_RETURN(CleanupRange(pItemsControl, cleanupRangeStart, cleanupCount));
                IFC_RETURN(spRealizedChildren->get_Size(&nCount));

                // CleanupRange just modified the _realizedChildren list.  Adjust the childIndex.
                childIndex -= cleanupCount;
                focusedChild -= cleanupCount;
                previousFocusable -= cleanupCount;
                nextFocusable -= cleanupCount;

                cleanupCount = 0;
                cleanupRangeStart = -1;
            }
        }

        // Assume non-recyclable container;
        performCleanup = TRUE;

        if (IsOutsideCacheWindow(itemIndex) &&
            childIndex != focusedChild &&
            childIndex != previousFocusable &&
            childIndex != nextFocusable)
        {
            BOOLEAN isInsideViewWindow = FALSE;
            ctl::ComPtr<xaml::IUIElement> spChild;

            IFC_RETURN(spRealizedChildren->GetAt(childIndex, &spChild));


            // Only check to see if an Item is inside the view window if we are not ignoring transitions.

            // so this is about elements that are not currently being transitioned, but they might have been
            // moved offscreen by the insertion of a bunch of other elements. This might push them off the
            // cachewindow, but in truth they are still there, waiting for a transition to move them
            // to their new location.

            // notice that we do not differentiate between transitioning items or not transitioning items
            if (!bIsIgnoringTransitions)
            {
                IFC_RETURN(IsInsideViewWindowLocationBased(spChild.Get(), constraint, isInsideViewWindow));
            }

            if (!isInsideViewWindow)
            {
                BOOLEAN isOwnContainer = FALSE;
                ctl::ComPtr<IInspectable> spItem;
                ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
                ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;

                IFC_RETURN(get_ItemContainerGenerator(&spGenerator));
                IFC_RETURN(spGenerator.Cast<ItemContainerGenerator>()->get_View(&spItems));

                // to understand bug 1255485, changing this to a failfast so we can look at the locals
                // we seem to be getting an out of range, but it is hard to imagine how that can be and harder
                // to imagine how to fix that. Would be great to actually understand what is going on.
                IFCFAILFAST(spItems->GetAt(itemIndex, &spItem));
                if (!ctl::value_is<xaml_controls::IGroupItem>(spItem.Get()))
                {
                    IFC_RETURN(static_cast<ItemsControl*>(pItemsControl)->IsItemItsOwnContainer(spItem.Get(), &isOwnContainer));
                }

                if (!isOwnContainer)
                {
                    BOOLEAN isContainerDragDropOwner = FALSE;
                    if (spListViewBase && spChild)
                    {
                        IFC_RETURN(spListViewBase.Cast<ListViewBase>()->IsContainerDragDropOwner(
                            spChild.Get(), &isContainerDragDropOwner));
                    }

                    if (!isContainerDragDropOwner)
                    {
                        BOOLEAN bCanceled = FALSE;
                        IFC_RETURN(NotifyCleanupItem(spItem.Get(), spChild.Get(), pItemsControl, bCanceled));
                        if (!bCanceled)
                        {
                            //
                            // The container is eligible to be virtualized
                            //
                            performCleanup = FALSE;

                            if (cleanupRangeStart == -1)
                            {
                                cleanupRangeStart = childIndex;
                            }

                            cleanupCount++;
                        }
                    }
                }
            }
        }
    }

    if (cleanupRangeStart >= 0 && cleanupCount > 0)
    {
        IFC_RETURN(CleanupRange(pItemsControl, cleanupRangeStart, cleanupCount));
        //
        // Figure out the position of the first visible item
        //
        IFC_RETURN(IndexToGeneratorPositionForStart(pItemsControl, VisibleStartItemIndex(), m_iFirstVisibleChildIndex, startPos));
    }

#if DBG
    //debug_DumpRealizedChildren();
    //debug_DumpVisualChildren();
    //Trace(L"==============================Containers cleaned========================================");
#endif

    return S_OK;
}

// Sets up IsVirtualizing, VirtualizationMode
//
// IsVirtualizing is TRUE if turned on via the items control and if the panel has a viewport.
// VSP has a viewport if it's either the scrolling panel or it was given MeasureData.
_Check_return_
HRESULT
VirtualizingStackPanel::SetVirtualizationState(
    _In_ xaml_controls::IItemsControl* pItemsControl)
{
    HRESULT hr = S_OK;
    xaml_controls::VirtualizationMode mode = xaml_controls::VirtualizationMode_Standard;

    if (pItemsControl)
    {
        ctl::ComPtr<xaml::IDependencyObject> spDO;

        IFC(ctl::do_query_interface(spDO, pItemsControl));

        BOOLEAN isScrolling = FALSE;
        IFC(VirtualizingStackPanelFactory::GetVirtualizationModeStatic(spDO.Get(), &mode));

        // Set IsVirtualizing.  This panel can only virtualize if IsVirtualizing is set on its ItemsControl and it has viewport data.
        // It has viewport data if it's either the scroll host or was given viewport information by measureData.

        // if ItemsControl is using VirtualizingStackPanel then ItemsControl is in Virtualizing mode
        // if we want user to control IsVirtualizing property then we should use
        // "if (GetIsVirtualizing(itemsControl) && IsScrolling)"

        IFC(get_IsScrolling(&isScrolling));
        if (isScrolling)
        {
            IFC(put_IsVirtualizing(TRUE));
            if (m_bIsVirtualizing)
            {
                IFC(static_cast<ItemsControl*>(pItemsControl)->SetVirtualizationStateByPanel());
            }
        }
    }
    else
    {
        IFC(put_IsVirtualizing(FALSE));
    }

    m_VirtualizationMode = mode;

Cleanup:
    RRETURN(hr);
}

// Computes the total dimension of all realized children except potentially the focused child
_Check_return_
HRESULT
VirtualizingStackPanel::ComputeTotalRealizedChildrenDimension(
    _Out_ FLOAT& cumulatedChildDim, _Out_ UINT& nCount)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    wf::Size childDesiredSize = {};

    nCount = 0;
    cumulatedChildDim = 0.0;

    IFC(VirtualizingStackPanelGenerated::get_Orientation(&orientation));

    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

    if (m_iFirstVisibleChildIndex > -1 && nCount > 0)
    {
        for (INT childIndex = m_iFirstVisibleChildIndex - m_iBeforeTrail; childIndex < m_iVisibleCount + m_iFirstVisibleChildIndex; childIndex++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild;
            IFC(spRealizedChildren->GetAt(childIndex, &spChild));
            if (spChild)
            {
                IFC(spChild->get_DesiredSize(&childDesiredSize));

                if (orientation == xaml_controls::Orientation_Horizontal)
                {
                    cumulatedChildDim += childDesiredSize.Width;
                }
                else
                {
                    cumulatedChildDim += childDesiredSize.Height;
                }
            }
        }
        nCount = m_iVisibleCount + m_iBeforeTrail;
    }

Cleanup:
    RRETURN(hr);
}

// Finds the focused child along with the previous and next focusable children.  Used only when recycling containers;
// the standard mode has a different cleanup algorithm
_Check_return_
HRESULT
VirtualizingStackPanel::FindFocusedChildInRealizedChildren(
    _Out_ INT& focusedChild,
    _Out_ INT& previousFocusable,
    _Out_ INT& nextFocusable)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

    focusedChild = previousFocusable = nextFocusable = -1;

    UINT nCount = 0;
    IFC(get_RealizedChildren(&spRealizedChildren));
    IFC(spRealizedChildren->get_Size(&nCount));

    for (UINT i = 0; i < nCount; i++)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;
        ctl::ComPtr<xaml::IFrameworkElement> spFE;

        IFC(spRealizedChildren->GetAt(i, &spChild));

        spFE = spChild.AsOrNull<xaml::IFrameworkElement>();

        if (spFE)
        {
            BOOLEAN hasFocus = FALSE;
            IFC(spFE.Cast<FrameworkElement>()->HasFocus(&hasFocus));

            if (hasFocus)
            {
                focusedChild = i;
                previousFocusable = i - 1;
                nextFocusable = i + 1;
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
VirtualizingStackPanel::put_IsVirtualizing(_In_ BOOLEAN isVirtualizing)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemsHost = FALSE;

    IFC(get_IsItemsHost(&isItemsHost));
    isVirtualizing = isVirtualizing && isItemsHost;

    if (!isVirtualizing)
    {
        m_tpRealizedChildren.Clear();
    }

    m_bIsVirtualizing = isVirtualizing;

Cleanup:
    RRETURN(hr);
}

// Logical Orientation override
_Check_return_ HRESULT VirtualizingStackPanel::get_LogicalOrientation(
    _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    *pValue = orientation;

Cleanup:
    RRETURN(hr);
}

// Physical Orientation override
_Check_return_ HRESULT VirtualizingStackPanel::get_PhysicalOrientation(
     _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    *pValue = orientation;

Cleanup:
    RRETURN(hr);
}

// Get the closest element information to the point.
_Check_return_ HRESULT VirtualizingStackPanel::GetClosestElementInfo(
    _In_ wf::Point position,
    _Out_ xaml_primitives::ElementInfo* returnValue)
{
    HRESULT hr = S_OK;

    IFC(VirtualizingStackPanelGenerated::GetClosestElementInfo(position, returnValue));
    IFC(GetClosestOrInsertionIndex(position, FALSE, &returnValue->m_childIndex));

Cleanup:
    RRETURN(hr);
}

// Get the index where an item should be inserted if it were dropped at
// the given position.  This will be used by live reordering.
_Check_return_ HRESULT VirtualizingStackPanel::GetInsertionIndex(
    _In_ wf::Point position,
    _Out_ INT* returnValue)
{
    HRESULT hr = S_OK;
    IFC(VirtualizingStackPanelGenerated::GetInsertionIndex(position, returnValue));
    IFC(GetClosestOrInsertionIndex(position, TRUE, returnValue));

Cleanup:
    RRETURN(hr);
}

 // Gets a series of BOOLEAN values indicating whether a given index is
// positioned on the leftmost, topmost, rightmost, or bottommost
// edges of the layout.  This can be useful for both determining whether
// to tilt items at the edges of rows or columns as well as providing
// data for portal animations.
_Check_return_ HRESULT VirtualizingStackPanel::IsLayoutBoundary(
    _In_ INT index,
    _Out_ BOOLEAN* pIsLeftBoundary,
    _Out_ BOOLEAN* pIsTopBoundary,
    _Out_ BOOLEAN* pIsRightBoundary,
    _Out_ BOOLEAN* pIsBottomBoundary)
{
    HRESULT hr = S_OK;
    bool isHorizontal = true;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFC(VirtualizingStackPanelGenerated::IsLayoutBoundary(index, pIsLeftBoundary, pIsTopBoundary, pIsRightBoundary, pIsBottomBoundary));

    IFC(get_PhysicalOrientation(&orientation));
    isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    IFC(VirtualizingStackPanel::ComputeLayoutBoundary(
        index,
        m_nItemsCount,
        isHorizontal,
        pIsLeftBoundary,
        pIsTopBoundary,
        pIsRightBoundary,
        pIsBottomBoundary));

Cleanup:
    RRETURN(hr);
}

 // Gets a series of BOOLEAN values indicating whether a given index is
// positioned on the leftmost, topmost, rightmost, or bottommost
// edges of the layout.  This can be useful for both determining whether
// to tilt items at the edges of rows or columns as well as providing
// data for portal animations.
_Check_return_ HRESULT VirtualizingStackPanel::ComputeLayoutBoundary(
    _In_ INT index,
    _In_ INT itemCount,
    _In_ bool isHorizontal,
    _Out_ BOOLEAN* pIsLeftBoundary,
    _Out_ BOOLEAN* pIsTopBoundary,
    _Out_ BOOLEAN* pIsRightBoundary,
    _Out_ BOOLEAN* pIsBottomBoundary)
{
    HRESULT hr = S_OK;

    IFCPTR(pIsLeftBoundary);
    IFCPTR(pIsTopBoundary);
    IFCPTR(pIsRightBoundary);
    IFCPTR(pIsBottomBoundary);

    if(isHorizontal)
    {
        *pIsLeftBoundary = (index == 0);
        *pIsBottomBoundary = TRUE;
        *pIsTopBoundary = TRUE;
        *pIsRightBoundary = (index == itemCount - 1);
    }
    else
    {
        *pIsLeftBoundary = TRUE;
        *pIsBottomBoundary = (index == itemCount - 1);
        *pIsTopBoundary = (index == 0);
        *pIsRightBoundary = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// This method is used for getting closest as well as Insertion Index
_Check_return_ HRESULT VirtualizingStackPanel::GetClosestOrInsertionIndex(
    _In_ wf::Point position,
    _In_ bool isInsertionIndex,
    _Out_ INT* returnValue)
{
    BOOLEAN isHorizontal = TRUE;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    wf::Size totalSize = {0, 0}; // itemSize which include Justification Size
    DOUBLE firstItemOffset = 0.0;
    INT visibleStartIndex = 0;
    INT insertionOrClosestIndex = 0;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;
    UINT32 nCount = 0;
    wf::Size childDesiredSize = {};
    ctl::ComPtr<IScrollOwner> spOwner;

    IFCPTR_RETURN(returnValue);
    *returnValue = -1;

    IFC_RETURN(m_ScrollData.get_ScrollOwner(&spOwner));

    // Get Orientation
    IFC_RETURN(get_PhysicalOrientation(&orientation));
    isHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    // Take in to account DM's offset.
    DOUBLE offset = 0.0;
    FLOAT zoomFactor = 1.0;
    IFC_RETURN(ComputePixelOffset(isHorizontal, offset));
    IFC_RETURN(GetZoomFactor(&zoomFactor));
    ASSERT(zoomFactor == m_fZoomFactor);

    // ComputePixelOffset multiples by the zoom factor as a last step. We don't want that, so undo it here.
    offset /= zoomFactor;

    if (isHorizontal)
    {
        position.X -= (FLOAT)offset;
    }
    else
    {
        position.Y -= (FLOAT)offset;
    }

    // Calculate visibleStartIndex
    visibleStartIndex = MAX(-1, ComputeIndexOfFirstVisibleItem(isHorizontal, firstItemOffset) * static_cast<INT>(m_itemsPerLine));

    //get realised children and count
    IFC_RETURN(get_RealizedChildren(&spRealizedChildren));
    IFC_RETURN(spRealizedChildren->get_Size(&nCount));

    // Gothrough all children start from first visible realised child

    // Our visible children should not extend past the end of the realized child list. The error condition may occur because our collection was
    // modified (item removed) in the past, but we haven't had an arrange pass before calling this method. This causes m_iFirstVisibleChildIndex
    // and possibly m_iVisibleCount to be out of sync. Because of the way we use this method, this shouldn't happen. The MIN() inside the for loop's
    // condition ensures we don't crash in retail bits, but instead return an index that is off by one if the given point is right at the bottom/right
    // edge of the view. Since apps can't call this method, we should be safe from app-land.
    ASSERT((INT)nCount >= m_iFirstVisibleChildIndex + m_iVisibleCount);
    for (INT childIndex = m_iFirstVisibleChildIndex;  childIndex < MIN((INT)nCount, m_iFirstVisibleChildIndex + m_iVisibleCount); childIndex++)
    {
        ctl::ComPtr<xaml::IUIElement> spChild;

        IFCEXPECT_RETURN(spRealizedChildren);
        IFC_RETURN(spRealizedChildren->GetAt(childIndex, &spChild));
        if (spChild)
        {
            IFC_RETURN(spChild->get_DesiredSize(&childDesiredSize));
        }

        if(isHorizontal)
        {
            // In case of first child, get only visible size from the item
            if(childIndex == m_iFirstVisibleChildIndex)
            {
                // If items are scrolled, firstItemOffset will be something like 0.254, the fractional part which got hidden
                // The following line gets the visible width of the first item
                childDesiredSize.Width = static_cast<FLOAT>((1 - firstItemOffset) * childDesiredSize.Width);
            }

            totalSize.Width += childDesiredSize.Width;
            if(DoubleUtil::LessThanOrClose(position.X, totalSize.Width))
            {
                insertionOrClosestIndex = childIndex;
                // Insertion case
                if(isInsertionIndex &&
                    DoubleUtil::GreaterThanOrClose(position.X, totalSize.Width - childDesiredSize.Width/2))
                {
                    insertionOrClosestIndex += 1;
                }
                break;
            }
        }
        else
        {
            // In case of first child, get only visible size from the item
            if(childIndex == m_iFirstVisibleChildIndex)
            {
                // If items are scrolled, firstItemOffset will be something like 0.254, the fractional part which got hidden
                // The following line gets the visible Height of the first item
                childDesiredSize.Height = static_cast<FLOAT>((1 - firstItemOffset) * childDesiredSize.Height);
            }

            totalSize.Height += childDesiredSize.Height;
            if(DoubleUtil::LessThanOrClose(position.Y, totalSize.Height))
            {
                insertionOrClosestIndex = childIndex;
                // Insertion case
                if(isInsertionIndex &&
                    DoubleUtil::GreaterThanOrClose(position.Y, totalSize.Height - childDesiredSize.Height/2))
                {
                    insertionOrClosestIndex += 1;
                }
                break;
            }
        }
    }

    // Special condition for the case when alignment is Left and there is extra space on the right
    if(isHorizontal && DoubleUtil::GreaterThanOrClose(position.X, totalSize.Width))
    {
        insertionOrClosestIndex = (isInsertionIndex) ? nCount : nCount - 1;
    }
    else if(!isHorizontal && DoubleUtil::GreaterThanOrClose(position.Y, totalSize.Height))
    {
        insertionOrClosestIndex = (isInsertionIndex) ? nCount : nCount - 1;
    }

    *returnValue = insertionOrClosestIndex - m_iFirstVisibleChildIndex + visibleStartIndex;

    return S_OK;
}

// Scroll the given ItemIndex into the view
// whem m_itemsPerLine > 1, to view the given Index into view, we need to find the line Index and scroll into that line
_Check_return_ HRESULT VirtualizingStackPanel::ScrollIntoView(
    _In_ UINT index,
    _In_ BOOLEAN isGroupItemIndex,
    _In_ xaml_controls::ScrollIntoViewAlignment alignment)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollOwner> spOwner;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bHorizontal = FALSE;
    DOUBLE groupIndex = index;
    DOUBLE scrollToOffset = 0;
    INT indexToEnsureInView = -1;

    if(!isGroupItemIndex)
    {
        IFC(GetIndexInGroupView(index, groupIndex));
    }

    // Get the scroll Owner
    IFC(m_ScrollData.get_ScrollOwner(&spOwner));

    // Get Orientation
    IFC(get_PhysicalOrientation(&orientation));
    bHorizontal = (orientation == xaml_controls::Orientation_Horizontal);

    if(!bHorizontal)
    {
        switch (alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            {
                IFC(spOwner->ScrollToVerticalOffsetImpl(groupIndex));
                break;
            }

        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            {
                DOUBLE verticalOffset = 0;
                DOUBLE viewportHeight = 0;

                IFC(get_VerticalOffset(&verticalOffset));
                IFC(get_ViewportHeight(&viewportHeight));
                // If scrollIndex is above the Vertical offset, bring it into view
                if (verticalOffset - groupIndex > 0 || viewportHeight < 1)
                {
                    IFC(spOwner->ScrollToVerticalOffsetImpl(groupIndex));
                }
                // if ScrollIndex is below the viewport Size
                else if(verticalOffset + viewportHeight < groupIndex + 1)
                {
                    IFC(GetEstimatedOffsetForScrollIntoView(groupIndex, viewportHeight, bHorizontal, scrollToOffset, indexToEnsureInView));
                    IFC(spOwner->ScrollToVerticalOffsetImpl(scrollToOffset));
                    m_IndexToEnsureInView = indexToEnsureInView;
                }
                break;
            }
        }
    }
    else
    {
        switch (alignment)
        {
        case xaml_controls::ScrollIntoViewAlignment_Leading:
            {
                IFC(spOwner->ScrollToHorizontalOffsetImpl(groupIndex));
                break;
            }

        case xaml_controls::ScrollIntoViewAlignment_Default:
        default:
            {
                DOUBLE viewportWidth = 0;
                DOUBLE horizontalOffset = 0;

                IFC(get_HorizontalOffset(&horizontalOffset));
                IFC(get_ViewportWidth(&viewportWidth));

                // If ScrollIndex is on the left side of current offset, bring it into view
                if (horizontalOffset - groupIndex > 0 || viewportWidth < 1)
                {
                    IFC(spOwner->ScrollToHorizontalOffsetImpl(groupIndex));
                }
                // if Item is not inside the current offset, then only bring it into the view
                else if(horizontalOffset + viewportWidth < groupIndex + 1)
                {
                    IFC(GetEstimatedOffsetForScrollIntoView(groupIndex, viewportWidth, bHorizontal, scrollToOffset, indexToEnsureInView));
                    IFC(spOwner->ScrollToHorizontalOffsetImpl(scrollToOffset));
                    m_IndexToEnsureInView = indexToEnsureInView;
                }
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// ScrollIntoView calls this method to get correct estimated offset when the item being scrolled into view is below viewport.
// If the item being scrolled into view is in realized list we will walk up to find the correct offset
// Else we return indexToEnsureInView to the item being scrolled into view and Measure will ensure that item is in view.
_Check_return_ HRESULT VirtualizingStackPanel::GetEstimatedOffsetForScrollIntoView(
    _In_ DOUBLE index,
    _In_ DOUBLE viewportSize,
    _In_ BOOLEAN bHorizontal,
    _Out_ DOUBLE& scrollToOffset,
    _Out_ INT& indexToEnsureInView)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {};
    wf::Size childDesiredSize = {};
    FLOAT availableSize = bHorizontal ? m_LastSetAvailableSize.Width : m_LastSetAvailableSize.Height;
    FLOAT childSize = 0;
    INT logicalIndex = static_cast<INT>(index);
    UINT nCount = 0;
    UINT currentRealizedIndex = 0;
    scrollToOffset = index - viewportSize + 1;
    indexToEnsureInView = -1;

    // If item is realized, measure the offset based on pixel sizes.
    IFC(GetItemContainerGenerator(&spGenerator, NULL));
    if (spGenerator)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spRealizedChildren;

        IFC(get_RealizedChildren(&spRealizedChildren));
        IFC(spRealizedChildren->get_Size(&nCount));
        if (logicalIndex < static_cast<INT>(nCount))
        {
            IFC(spGenerator->GeneratorPositionFromIndex(logicalIndex, &position));
            currentRealizedIndex = static_cast<UINT>(position.Index);
            while (position.Offset == 0 && currentRealizedIndex < nCount && availableSize > childSize ) // All items are realized
            {
                ctl::ComPtr<xaml::IUIElement> spChild;

                availableSize -= childSize;
                IFC(spRealizedChildren->GetAt(currentRealizedIndex, &spChild));
                IFC(spChild->get_DesiredSize(&childDesiredSize));
                childSize = bHorizontal ? childDesiredSize.Width : childDesiredSize.Height;
                logicalIndex--;
                IFC(spGenerator->GeneratorPositionFromIndex(logicalIndex, &position));
                currentRealizedIndex = static_cast<UINT>(position.Index);
            }

            logicalIndex++; // +1 to get to last measured item.
        }

        if (availableSize <= childSize)
        {
            scrollToOffset = logicalIndex + (childSize-availableSize)/childSize;
        }
        else // Atleast 1 item above was not realized
        {
            indexToEnsureInView = static_cast<INT>(index);
        }
    }

Cleanup:
    RRETURN(hr);
}

// If ScrollIntoView has set and item to be ensured in view
// We will realize backwards from that item to top of viewport such that bottom of that item aligns with bottom of viewport.
_Check_return_ HRESULT VirtualizingStackPanel::CorrectOffsetForScrollIntoView(
    _In_ DOUBLE viewportSize,
    _In_ BOOLEAN bHorizontal,
    _In_ xaml_controls::IItemsControl* pItemsControl,
    _In_ wf::Size layoutSlotSize,
    _Out_ DOUBLE& firstItemOffset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    xaml_primitives::GeneratorPosition position = {};
    wf::Size childDesiredSize = {};
    INT currentIndex=0;
    INT childIndex=0;
    DOUBLE usedSize = 0;
    BOOLEAN visualOrderChanged = FALSE;
    BOOLEAN newlyRealized = FALSE;

    ASSERT(m_IndexToEnsureInView >= 0, L"Index to ensure in View should be greater than 0.");

    currentIndex = m_IndexToEnsureInView < static_cast<INT>(m_nItemsCount) ? m_IndexToEnsureInView : m_nItemsCount - 1;

    if (currentIndex == -1)
    {
        return S_OK;
    }

    m_IndexToEnsureInView = -1; // reset index to be scrolled into view.
    IFC(GetItemContainerGenerator(&spGenerator, pItemsControl));
    while(usedSize < viewportSize && currentIndex >= 0)
    {
        ctl::ComPtr<xaml::IUIElement> spUIChild;
        ctl::ComPtr<xaml::IDependencyObject> spChildAsDO;

        IFC(IndexToGeneratorPositionForStart(pItemsControl, currentIndex, childIndex, position));
        // We generate forward because backward generation has issues.
        // This should be changed to backward generation to improve performance alongside GeneratePerviousChild method.
        IFC(spGenerator->StartAt(position, xaml_primitives::GeneratorDirection_Forward, TRUE));
        IFC(spGenerator->GenerateNext(&newlyRealized, &spChildAsDO));

        if (!ctl::is<xaml::IUIElement>(spChildAsDO.Get()))
        {
            ASSERT(!newlyRealized, L"The generator realized a null value.");
            // We reached the end of the items (because of a group)
            break;
        }

        IFC(spChildAsDO.As(&spUIChild));
        IFC(AddContainerFromGenerator(pItemsControl, childIndex, spUIChild.Get(), newlyRealized, visualOrderChanged));
        IFC(MeasureChild(spUIChild.Get(), layoutSlotSize, &childDesiredSize));
        if (bHorizontal)
        {
            usedSize += childDesiredSize.Width;
        }
        else
        {
            usedSize += childDesiredSize.Height;
        }

        currentIndex--;
        IFC(spGenerator->Stop());
    }

    currentIndex++; // +1 to get to last measured item.
    if (usedSize >= viewportSize) // We have realized all items above to fill the viewable region.
    {
        m_iVisibleStart = currentIndex;
        if (bHorizontal && childDesiredSize.Width > 0)
        {
            IFC(m_ScrollData.put_OffsetX(currentIndex + (usedSize - viewportSize)/childDesiredSize.Width));
            firstItemOffset = DoubleUtil::Fractional(m_ScrollData.get_OffsetX());
        }
        else if (childDesiredSize.Height > 0)
        {
            IFC(m_ScrollData.put_OffsetY(currentIndex + (usedSize - viewportSize)/childDesiredSize.Height));
            firstItemOffset = DoubleUtil::Fractional(m_ScrollData.get_OffsetY());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VirtualizingStackPanel::MeasureChildForItemsChanged(
    _In_ xaml::IUIElement* pChild)
{
    HRESULT hr = S_OK;
    wf::Size desiredSize = {};

    IFC(MeasureChild(pChild, m_LastSetChildLayoutSlotSize, &desiredSize));

Cleanup:
    RRETURN(hr);
}
