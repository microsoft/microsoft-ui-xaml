// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarPanel.g.h"
#include "CalendarLayoutStrategy.g.h"
#include "ScrollViewer.g.h"
#include "ScrollContentPresenter.g.h"
#include "calendarviewGeneratorHost.h"
#include "CalendarViewBaseItem.g.h"
#include "calendarview_Partial.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

#undef min
#undef max

_Check_return_ HRESULT CalendarPanel::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CalendarLayoutStrategy> spCalendarLayoutStrategy;

    // Initalize the base class first.
    IFC(CalendarPanelGenerated::Initialize());

    IFC(ctl::make<CalendarLayoutStrategy>(&spCalendarLayoutStrategy));
    IFC(SetLayoutStrategyBase(spCalendarLayoutStrategy.Get()));

Cleanup:
    return hr;
}

IFACEMETHODIMP CalendarPanel::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* pDesired)
{
    if (m_type != CalendarPanelType::CalendarPanelType_Invalid)
    {
        // for secondary panel, we don't care about the biggest item size, because this type means
        // we have an explict dimensions set, in this case we'll always honor the dimensions.
        if (m_type != CalendarPanelType::CalendarPanelType_Secondary && !m_isBiggestItemSizeDetermined)
        {
            ctl::ComPtr<CalendarViewGeneratorHost> spOwner;

            IFC_RETURN(GetOwner(&spOwner));

            if (spOwner)
            {
                wf::Size biggestItemSize = {};

                IFC_RETURN(DetermineTheBiggestItemSize(spOwner.Get(), availableSize, &biggestItemSize));

                if (biggestItemSize.Width != m_biggestItemSize.Width || biggestItemSize.Height != m_biggestItemSize.Height)
                {
                    m_biggestItemSize = biggestItemSize;

                    // for primary panel, we should notify the CalendarView, so CalendarView can update
                    // the size for other template parts.
                    if (m_type == CalendarPanelType::CalendarPanelType_Primary)
                    {
                        IFC_RETURN(SetItemMinimumSize(biggestItemSize));
                        IFC_RETURN(spOwner->OnPrimaryPanelDesiredSizeChanged());
                    }
                }
            }

            m_isBiggestItemSizeDetermined = true;
        }
    }
    // else CalendarPanel::SetPanelType has not been called yet.

    IFC_RETURN(CalendarPanelGenerated::MeasureOverride(availableSize, pDesired));

    return S_OK;
}

IFACEMETHODIMP CalendarPanel::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    bool needsRemeasure = false;

    if (m_type != CalendarPanelType::CalendarPanelType_Invalid)
    {
        ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;
        wf::Size viewportSize{ 0.0, 0.0 };

        // When we begin to arrange, we are good to know the ScrollViewer viewport size.
        // We need to check if the viewport can perfectly fit Rows x Cols items,
        // if not we need to change the items size and remeasure the panel.
        IFC_RETURN(GetViewportSize(&viewportSize));

        ASSERT(viewportSize.Height != 0.0 && viewportSize.Width != 0.0);

        // and the size of biggest item has been determined if the Panel is not Secondary Panel.
        ASSERT(m_type == CalendarPanelType::CalendarPanelType_Secondary ||
            (m_isBiggestItemSizeDetermined && m_biggestItemSize.Width != 0.0 && m_biggestItemSize.Height != 0.0));

        if (m_type == CalendarPanelType::CalendarPanelType_Secondary_SelfAdaptive)
        {
            int effectiveCols = static_cast<int>(viewportSize.Width / m_biggestItemSize.Width);
            int effectiveRows = static_cast<int>(viewportSize.Height / m_biggestItemSize.Height);

            effectiveCols = std::max(1, std::min(effectiveCols, m_suggestedCols));
            effectiveRows = std::max(1, std::min(effectiveRows, m_suggestedRows));

            IFC_RETURN(SetPanelDimension(effectiveCols, effectiveRows));
        }

        IFC_RETURN(GetLayoutStrategy(&spCalendarLayoutStrategy));

        // tell layout strategy that the new viewport size so it can compute the
        // arrange bound for items correctly.

        spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->SetViewportSize(viewportSize, &needsRemeasure);
    }
    // else CalendarPanel::SetPanelType has not been called yet.

    // when we need remeasure, we could skip the current arrange.
    if (needsRemeasure)
    {
        IFC_RETURN(InvalidateMeasure());
    }
    else
    {
        IFC_RETURN(CalendarPanelGenerated::ArrangeOverride(finalSize, returnValue));
    }

    return S_OK;
}


_Check_return_ HRESULT
CalendarPanel::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(CalendarPanelGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::CalendarPanel_Orientation:
    {
        xaml_controls::Orientation orientation = static_cast<xaml_controls::Orientation>(args.m_pNewValue->AsEnum());
        ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;

        IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));

        IFC(CacheFirstVisibleElementBeforeOrientationChange());

        // CalendarPanel orientation is the stacking direction. Which is the opposite of the
        // virtualization direction.
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->SetVirtualizationDirection(xaml_controls::Orientation_Vertical);
        }
        else
        {
            spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->SetVirtualizationDirection(xaml_controls::Orientation_Horizontal);
        }
        // let the base know
        IFC(ProcessOrientationChange());

    }
        break;
    case KnownPropertyIndex::CalendarPanel_Rows:
    {
        INT32 rows = 0;
        ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;

        IFC(args.m_pNewValue->GetSigned(rows));
        IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
        ASSERT(rows > 0);   // guaranteed to be positive number
        spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->SetRows(rows);
        IFC(OnRowsOrColsChanged(xaml_controls::Orientation::Orientation_Vertical));
    }
        break;
    case KnownPropertyIndex::CalendarPanel_Cols:
    {
        INT32 cols = 0;
        ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;

        IFC(args.m_pNewValue->GetSigned(cols));
        IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
        ASSERT(cols > 0);   // guaranteed to be positive number
        spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->SetCols(cols);
        IFC(OnRowsOrColsChanged(xaml_controls::Orientation::Orientation_Horizontal));
    }
        break;

    case KnownPropertyIndex::CalendarPanel_CacheLength:
    {
        DOUBLE newCacheLength = args.m_pNewValue->AsDouble();
        IFC(ModernCollectionBasePanel::put_CacheLengthBase(newCacheLength));
    }
        break;
    case KnownPropertyIndex::CalendarPanel_StartIndex:
    {
        ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;
        INT32 startIndex = 0;

        IFC(args.m_pNewValue->GetSigned(startIndex));

        ASSERT(startIndex >= 0);

        IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
        auto& table = spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->GetIndexCorrectionTable();
        table.SetCorrectionEntryForElementStartAt(startIndex);

        IFC(InvalidateMeasure());
    }
        break;

    default:
        break;
    }

Cleanup:
    return hr;
}

// Logical Orientation override
_Check_return_ HRESULT CalendarPanel::get_LogicalOrientation(
    _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    IFC(get_Orientation(pValue));

Cleanup:
    return hr;
}

// Physical Orientation override
_Check_return_ HRESULT CalendarPanel::get_PhysicalOrientation(
    _Out_ xaml_controls::Orientation* pValue)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    if (orientation == xaml_controls::Orientation_Vertical)
    {
        *pValue = xaml_controls::Orientation_Horizontal;
    }
    else
    {
        *pValue = xaml_controls::Orientation_Vertical;
    }

Cleanup:
    return hr;
}

// Virtual helper method to get the ItemsPerPage that can be overridden by derived classes.
_Check_return_ HRESULT CalendarPanel::GetItemsPerPageImpl(
    _In_ wf::Rect window,
    _Out_ DOUBLE* pItemsPerPage)
{
    return E_NOTIMPL;
}

#pragma region Special elements overrides

_Check_return_ HRESULT CalendarPanel::NeedsSpecialItem(_Out_ bool* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;
    *pResult = false;

    IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
    *pResult = spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->NeedsSpecialItem();

Cleanup:
    return hr;
}

_Check_return_ HRESULT
CalendarPanel::GetSpecialItemIndex(_Out_ int* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;
    *pResult = -1;

    IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
    *pResult = spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->GetSpecialItemIndex();

Cleanup:
    return hr;
}

#pragma endregion

_Check_return_ HRESULT CalendarPanel::GetDesiredViewportSize(_Out_ wf::Size* pSize)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;

    pSize->Width = 0;
    pSize->Height = 0;

    IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
    *pSize = spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->GetDesiredViewportSize();

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarPanel::SetItemMinimumSize(_In_ wf::Size size)
{
    HRESULT hr = S_OK;
    bool needsRemeasure = false;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;

    IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
    spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->SetItemMinimumSize(size, &needsRemeasure);

    if (needsRemeasure)
    {
        IFC(InvalidateMeasure());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarPanel::SetSnapPointFilterFunction(
    _In_ std::function<HRESULT(_In_ int itemIndex, _Out_ bool* pHasSnapPoint)> func)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;

    IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
    IFC(spCalendarLayoutStrategy.Cast<CalendarLayoutStrategy>()->SetSnapPointFilterFunction(func));

Cleanup:
    return hr;
}

// when Rows or Cols changed, we'll invalidate measure on panel itself.
// however this might not change the panel's desired size, especially when we
// change the dimension on the scrolling direction. To make sure the viewport size being adjusted
// correctly, we need to invalidate measure on the parent SCP.
_Check_return_ HRESULT CalendarPanel::OnRowsOrColsChanged(_In_ xaml_controls::Orientation orientation)
{
    HRESULT hr = S_OK;

    if (m_type == CalendarPanelType::CalendarPanelType_Primary)
    {
        // Primary Panel, we should InvalidateMeasure our parent to grant the new size.
        ctl::ComPtr<xaml_controls::ILayoutStrategy> spCalendarLayoutStrategy;
        xaml_controls::Orientation virtualizationDirection = xaml_controls::Orientation::Orientation_Horizontal;

        IFC(GetLayoutStrategy(&spCalendarLayoutStrategy));
        spCalendarLayoutStrategy->GetVirtualizationDirection(&virtualizationDirection);

        if (virtualizationDirection == orientation)
        {
            ctl::ComPtr<IDependencyObject> spParent;
            ctl::ComPtr<IScrollContentPresenter> spParentAsSCP;

            VisualTreeHelper::GetParentStatic(this, spParent.ReleaseAndGetAddressOf());
            spParentAsSCP = spParent.AsOrNull<IScrollContentPresenter>();
            if (spParentAsSCP)
            {
                IFC(spParentAsSCP.Cast<ScrollContentPresenter>()->InvalidateMeasure());
            }
        }
    }
    else
    {
        // Secondary and Secondary_SelfAdaptive, we won't affect the whole size, just need a new Arrange pass
        IFC(InvalidateArrange());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT CalendarPanel::SetOwner(_In_ CalendarViewGeneratorHost* pOwner)
{
    return ctl::AsWeak(ctl::as_iinspectable(pOwner), &m_wrGeneartorHostOwner);
}

_Check_return_ HRESULT CalendarPanel::GetOwner(_Outptr_result_maybenull_ CalendarViewGeneratorHost** ppOwner)
{
    ctl::ComPtr<IGeneratorHost> spOwner;

    *ppOwner = nullptr;

    IFC_RETURN(m_wrGeneartorHostOwner.As(&spOwner));
    *ppOwner = static_cast<CalendarViewGeneratorHost*>(spOwner.Detach());

    return S_OK;
}

_Check_return_ HRESULT CalendarPanel::SetNeedsToDetermineBiggestItemSize()
{
    m_isBiggestItemSizeDetermined = false;
    return InvalidateMeasure();
}

_Check_return_ HRESULT CalendarPanel::DetermineTheBiggestItemSize(
    _In_ CalendarViewGeneratorHost* pOwner,
    _In_ wf::Size availableSize,
    _Out_ wf::Size* pSize)
{
    ctl::ComPtr<IDependencyObject> spChildAsIDO;
    ctl::ComPtr<ICalendarViewBaseItem> spItemAsI;
    ctl::ComPtr<CalendarViewBaseItem> spCalendarViewBaseItem;
    wrl_wrappers::HString mainText;
    pSize->Height = 0.0;
    pSize->Width = 0.0;

    // we'll try to change all the possible strings on the first item and see the biggest desired size.
    IFC_RETURN(ContainerFromIndex(GetContainerManager().StartOfContainerVisualSection(), &spChildAsIDO));
    if (!spChildAsIDO)
    {
        wf::Size ignored = {};
        // no children yet, call base::MeasureOverride to generate at least one anchor item
        IFC_RETURN(CalendarPanelGenerated::MeasureOverride(availableSize, &ignored));

        IFC_RETURN(ContainerFromIndex(GetContainerManager().StartOfContainerVisualSection(), &spChildAsIDO));
        ASSERT(spChildAsIDO);
    }

    // there is at least one item in Panel, and the item has entered visual tree
    // we are good to measure it to get the desired size.
    spItemAsI = spChildAsIDO.AsOrNull<ICalendarViewBaseItem>();

    if (spItemAsI)
    {
        spCalendarViewBaseItem = spItemAsI.Cast<CalendarViewBaseItem>();
        // save the maintext
        IFC_RETURN(spCalendarViewBaseItem->GetMainText(mainText.GetAddressOf()));

        const std::vector<wrl_wrappers::HString>* pStrings = nullptr;
        IFC_RETURN(pOwner->GetPossibleItemStrings(&pStrings));

        ASSERT(!pStrings->empty());

        // try all the possible string and find the biggest desired size.
        for (auto& string : *pStrings)
        {
            wf::Size desiredSize = {};

            IFC_RETURN(spCalendarViewBaseItem->UpdateMainText(string.Get()));
            IFC_RETURN(spCalendarViewBaseItem->InvalidateMeasure());
            IFC_RETURN(spCalendarViewBaseItem->Measure(availableSize));
            IFC_RETURN(spCalendarViewBaseItem->get_DesiredSize(&desiredSize));
            pSize->Width = std::max(pSize->Width, desiredSize.Width);
            pSize->Height = std::max(pSize->Height, desiredSize.Height);
        }

        // restore the maintext
        IFC_RETURN(spCalendarViewBaseItem->UpdateMainText(mainText.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarPanel::SetPanelType(_In_ CalendarPanelType type)
{
    ASSERT(type != CalendarPanelType::CalendarPanelType_Invalid);

    if (m_type != type)
    {
        // we don't allow the type to be changed dynamically, only expect if
        // we change the type from Secondary_SelfAdaptive to Secondary.
        // the scenario is: by default YearPanel and DecadePanel are Secondary_SelfAdaptive, but once
        // developer calls SetYearDecadeDisplayDimensions, we'll change the type to Secondary (and never change back).
        ASSERT(m_type == CalendarPanelType::CalendarPanelType_Invalid ||
            (m_type == CalendarPanelType::CalendarPanelType_Secondary_SelfAdaptive &&
            type == CalendarPanelType::CalendarPanelType_Secondary));

        m_type = type;

        if (m_type == CalendarPanelType::CalendarPanelType_Primary || m_type == CalendarPanelType::CalendarPanelType_Secondary)
        {
            // for Primary and Secondary Panels, we don't need to adjust the dimension based on actual viewport size,
            // so the suggested dimensions are the actual dimensions

            if (m_suggestedCols != -1 && m_suggestedRows != -1)
            {
                IFC_RETURN(SetPanelDimension(m_suggestedCols, m_suggestedRows));
            }
        }
        // for Secondary_SelfAdaptive panel, we'll determine the exact dimension in Arrange pass whe we know the exact viewport size.
    }
    return S_OK;
}

_Check_return_ HRESULT CalendarPanel::SetSuggestedDimension(_In_ int cols, _In_ int rows)
{
    if (m_type == CalendarPanelType::CalendarPanelType_Primary || m_type == CalendarPanelType::CalendarPanelType_Secondary)
    {
        // for Primary or Secondary Panels, the suggested dimensions are the exact dimensions
        IFC_RETURN(SetPanelDimension(cols, rows));
    }
    else  //if (m_type == CalendarPanelType::CalendarPanelType_Invalid || m_type == CalendarPanelType::Secondary_SelfAdaptive)
    {
        // we'll determine the rows and cols later, when
        // 1. PanelType is set
        // 2. for Secondary_SelfAdaptive, in the Arrange pass when we know the exact viewport size.
        m_suggestedRows = rows;
        m_suggestedCols = cols;
    }

    return S_OK;
}

_Check_return_ HRESULT CalendarPanel::SetPanelDimension(_In_ int col, _In_ int row)
{
    int actualRows = 0;
    int actualCols = 0;

    IFC_RETURN(get_Rows(&actualRows));
    IFC_RETURN(get_Cols(&actualCols));

    if (row != actualRows || col != actualCols)
    {
        IFC_RETURN(put_Rows(row));
        IFC_RETURN(put_Cols(col));

        ctl::ComPtr<CalendarViewGeneratorHost> spOwner;

        IFC_RETURN(GetOwner(&spOwner));

        // dimension changed, we should check if we need to update the snap point filter function.
        if (spOwner)
        {
            bool canPanelShowFullScope = false;

            IFC_RETURN(CalendarView::CanPanelShowFullScope(spOwner.Get(), &canPanelShowFullScope));

            // If the current dimension setting allows us to show a full scope,
            // we'll have irregular snap point on each scope. Otherwise we'll
            // have regular snap point (on each row).

            if (!canPanelShowFullScope)
            {
                // We have not enough space, remove the customize function to get default regular snap point behavior.
                IFC_RETURN(SetSnapPointFilterFunction(nullptr));
            }
            else
            {
                // We have enough space, so we'll use irregular snap point
                // and we use the customize function to filter the snap point
                // so we only put a snap point on the first item of each scope.
                auto pHost = spOwner.Get();
                IFC_RETURN(SetSnapPointFilterFunction(
                    [pHost](_In_ int itemIndex, _Out_ bool* pHasSnapPoint)
                {
                    return pHost->GetIsFirstItemInScope(itemIndex, pHasSnapPoint);
                }));
            }
        }

    }
    return S_OK;
}
