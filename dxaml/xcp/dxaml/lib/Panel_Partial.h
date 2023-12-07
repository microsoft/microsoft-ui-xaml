// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Panel.g.h"

namespace DirectUI
{
    // Represents a Panel.
    //
    PARTIAL_CLASS(Panel)
    {
        protected:
            Panel() {}
            ~Panel() override {}

        public:
            // does this panel support portalling
            _Check_return_ virtual BOOLEAN IsPortallingSupported() { return FALSE; }

            // FocusManager GetFirst/LastFocusableElementOverride
            _Check_return_ HRESULT GetFirstFocusableElementOverride(
                _Outptr_ DependencyObject** ppFirstFocusable) override;

            _Check_return_ HRESULT GetLastFocusableElementOverride(
                _Outptr_ DependencyObject** ppLastFocusable) override;

            _Check_return_ HRESULT SetItemsOwner(_In_opt_ xaml_controls::IItemsControl* pItemsOwner);

            _Check_return_ HRESULT GetItemsOwner(_Outptr_ xaml_controls::IItemsControl** ppItemsOwner);

            _Check_return_ HRESULT OnTreeParentUpdated(_In_opt_ CDependencyObject *pNewParent, _In_opt_ BOOLEAN isParentAlive) override;

            IFACEMETHOD(get_Children)(_Outptr_ wfc::IVector<xaml::UIElement*>** pValue) override;

        private:
            TrackerPtr<wfc::IVector<xaml::UIElement*>> m_tpChildren;
            ctl::WeakRefPtr m_wrItemsOwner;
    };
}
