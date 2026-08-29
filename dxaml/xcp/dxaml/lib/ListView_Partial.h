// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      ListView displays a a rich, interactive collection of vertically stacked
//      items.

#pragma once

#include "ListView.g.h"

namespace DirectUI
{
    // ListView displays a a rich, interactive collection of vertically stacked
    // items.
    PARTIAL_CLASS(ListView)
    {
        public:
            ListView();
            bool ItemAllowFocusFromUIA() const { return m_itemsAcceptFocusFromUIA; }
            void SetAllowItemFocusFromUIA(bool itemsAcceptFocusFromUIA) { m_itemsAcceptFocusFromUIA = itemsAcceptFocusFromUIA; }
                
        protected: 
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;
            
            // Determines if the specified item is (or is eligible to be) its
            // own container.
            IFACEMETHOD(IsItemItsOwnContainerOverride)(
                _In_ IInspectable* item, 
                _Out_ BOOLEAN* returnValue)
                override;
            
            // Creates or identifies the element that is used to display the
            // given item.
            IFACEMETHOD(GetContainerForItemOverride)(
                _Outptr_ xaml::IDependencyObject** returnValue)
                override;

            // Creates or identifies the element that is used to display the
            // given group. This is only valid for new style flat panel grouping
            _Check_return_ HRESULT GetHeaderForGroupOverrideImpl(
                _Outptr_ xaml::IDependencyObject** returnValue) override;

        private:
            bool m_itemsAcceptFocusFromUIA;
    };
}
