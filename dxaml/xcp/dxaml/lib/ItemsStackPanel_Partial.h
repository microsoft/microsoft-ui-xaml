// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsStackPanel.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ItemsStackPanel)
    {
    protected:
        ItemsStackPanel();
        ~ItemsStackPanel() override;
        _Check_return_ HRESULT Initialize() override;

        // Handle the custom property changed event and call the
        // OnPropertyChanged methods. 
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
        
        // Only ItemsStackPanel and CalendarPanel support the maintain viewport behavior.
        ItemsUpdatingScrollMode GetItemsUpdatingScrollMode() const override { return static_cast<ItemsUpdatingScrollMode>(m_itemsUpdatingScrollMode); }

        //
        // Special elements overrides
        //
        _Check_return_ HRESULT NeedsSpecialItem(_Out_ bool* pResult) override;
        _Check_return_ HRESULT NeedsSpecialGroup(_Out_ bool* pResult) override;
        _Check_return_ HRESULT GetSpecialItemIndex(_Out_ int* pResult) override;
        _Check_return_ HRESULT GetSpecialGroupIndex(_Out_ int* pResult) override;
        _Check_return_ HRESULT RegisterSpecialContainerSize(int itemIndex, wf::Size containerDesiredSize) override;
        _Check_return_ HRESULT RegisterSpecialHeaderSize(int groupIndex, wf::Size headerDesiredSize) override;

        // Notify the layout strategy of the new header placement.
        _Check_return_ HRESULT SetGroupHeaderStrategy(_In_ GroupHeaderStrategy strategy) override;

    public:
        // implementation of IOrientedPanel
        IFACEMETHOD(get_LogicalOrientation)(_Out_ xaml_controls::Orientation* pValue) override;
        IFACEMETHOD(get_PhysicalOrientation)(_Out_ xaml_controls::Orientation* pValue) override;

        _Check_return_ HRESULT get_FirstCacheIndexImpl(_Out_ INT* pValue) { RRETURN(get_FirstCacheIndexBase(pValue)); }
        _Check_return_ HRESULT get_FirstVisibleIndexImpl(_Out_ INT* pValue) { RRETURN(get_FirstVisibleIndexBase(pValue)); }
        _Check_return_ HRESULT get_LastVisibleIndexImpl(_Out_ INT* pValue) { RRETURN(get_LastVisibleIndexBase(pValue)); }
        _Check_return_ HRESULT get_LastCacheIndexImpl(_Out_ INT* pValue) { RRETURN(get_LastCacheIndexBase(pValue)); }
        _Check_return_ HRESULT get_ScrollingDirectionImpl(_Out_ xaml_controls::PanelScrollingDirection* pValue) { RRETURN(get_PanningDirectionBase(pValue)); }

    private:
        xaml_controls::ItemsUpdatingScrollMode m_itemsUpdatingScrollMode;
    };
}
