// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CalendarPanel.g.h"

namespace DirectUI
{

    class CalendarViewGeneratorHost;
    PARTIAL_CLASS(CalendarPanel)
    {

    public:
        enum class CalendarPanelType
        {
            CalendarPanelType_Invalid,
            CalendarPanelType_Primary,                  // desired size is natural size
            CalendarPanelType_Secondary,                // desired size is {0, 0}, children will be arranged by given dimension, they will be clipped if won't fit.
            CalendarPanelType_Secondary_SelfAdaptive    // desired size is {0, 0}, children will be arranged based on their desired sizes, Panel will adjust the dimensions to make all of them can fit.
        };

    protected:

        CalendarPanel() 
            : m_type(CalendarPanelType::CalendarPanelType_Invalid)
            , m_isBiggestItemSizeDetermined(false)
            , m_biggestItemSize{}
            , m_suggestedRows(-1)
            , m_suggestedCols(-1)
        {

        }

        _Check_return_ HRESULT Initialize() override;

        IFACEMETHOD(MeasureOverride)(
            _In_ wf::Size pAvailableSize,
            _Out_ wf::Size* pDesired) override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size arrangeSize,
            _Out_ wf::Size* pReturnValue) override;

        // Handle the custom property changed event and call the
        // OnPropertyChanged methods. 
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Only ItemsStackPanel and CalendarPanel support the maintain viewport behavior.
        ItemsUpdatingScrollMode GetItemsUpdatingScrollMode() const override { return static_cast<ItemsUpdatingScrollMode>(xaml_controls::ItemsUpdatingScrollMode_KeepItemsInView); }        

        // a wrapgrid is able to portal (AddDeleteTransition)
        _Check_return_ BOOLEAN IsPortallingSupported() override { return TRUE; }

        // Virtual helper method to get the ItemsPerPage that can be overridden by derived classes.
        _Check_return_ HRESULT GetItemsPerPageImpl(_In_ wf::Rect window, _Out_ DOUBLE* pItemsPerPage) override;
        
        //
        // Special elements overrides
        //
        _Check_return_ HRESULT NeedsSpecialItem(_Out_ bool* pResult) override;
        _Check_return_ HRESULT GetSpecialItemIndex(_Out_ int* pResult) override;

    public:
        _Check_return_ HRESULT GetDesiredViewportSize(_Out_ wf::Size* pSize);
        _Check_return_ HRESULT SetItemMinimumSize(_In_ wf::Size size);

        _Check_return_ HRESULT SetSnapPointFilterFunction(_In_ std::function<HRESULT(_In_ int itemIndex, _Out_ bool* pHasSnapPoint)> func);

        _Check_return_ HRESULT SetOwner(_In_ CalendarViewGeneratorHost* pOwner);

        _Check_return_ HRESULT SetNeedsToDetermineBiggestItemSize();

        _Check_return_ HRESULT get_FirstCacheIndexImpl(_Out_ INT* pValue) { RRETURN(get_FirstCacheIndexBase(pValue)); }
        _Check_return_ HRESULT get_FirstVisibleIndexImpl(_Out_ INT* pValue) { RRETURN(get_FirstVisibleIndexBase(pValue)); }
        _Check_return_ HRESULT get_LastVisibleIndexImpl(_Out_ INT* pValue) { RRETURN(get_LastVisibleIndexBase(pValue)); }
        _Check_return_ HRESULT get_LastCacheIndexImpl(_Out_ INT* pValue) { RRETURN(get_LastCacheIndexBase(pValue)); }
        _Check_return_ HRESULT get_ScrollingDirectionImpl(_Out_ xaml_controls::PanelScrollingDirection* pValue) { RRETURN(get_PanningDirectionBase(pValue)); }

        _Check_return_ HRESULT SetPanelType(_In_ CalendarPanelType type);

        CalendarPanelType GetPanelType()
        {
            return m_type;
        }

        _Check_return_ HRESULT SetSuggestedDimension(_In_ int cols, _In_ int rows);
    public:
        // implementation of IOrientedPanel
        _Check_return_ IFACEMETHOD(get_LogicalOrientation)(_Out_ xaml_controls::Orientation* pValue) override;
        _Check_return_ IFACEMETHOD(get_PhysicalOrientation)(_Out_ xaml_controls::Orientation* pValue) override;

    private:
        _Check_return_ HRESULT OnRowsOrColsChanged(_In_ xaml_controls::Orientation orientation);

        _Check_return_ HRESULT GetOwner(_Outptr_result_maybenull_ CalendarViewGeneratorHost** ppOwner);

        _Check_return_ HRESULT DetermineTheBiggestItemSize(
            _In_ CalendarViewGeneratorHost* pOwner,
            _In_ wf::Size availableSize, 
            _Out_ wf::Size* pSize);

        _Check_return_ HRESULT SetPanelDimension(_In_ int col, _In_ int row);

    private:
        ctl::WeakRefPtr m_wrGeneartorHostOwner;
        
        // primaryPanel will determine the whole CalendarView's size, e.g. MonthPanel
        // secondaryPanels will not, e.g. YearPanel and DecadePanel.

        // primaryPanel always reports it's real desired size to it's parent (SCP)
        // secondaryPanels always reports (0,0) as it's desired size to it's parent (SCP)

        CalendarPanelType m_type;

        bool m_isBiggestItemSizeDetermined;

        wf::Size m_biggestItemSize;

        int m_suggestedRows;
        int m_suggestedCols;
    };
}

