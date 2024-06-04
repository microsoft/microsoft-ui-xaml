// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectorItemAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the SelectorItemAutomationPeer
    PARTIAL_CLASS(SelectorItemAutomationPeer)
    {
        public:
            // Initializes a new instance of the SelectorItemAutomationPeer class.
            SelectorItemAutomationPeer();
            ~SelectorItemAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue);

            // ISelectItemProvider
            virtual _Check_return_ HRESULT SelectImpl();
            virtual _Check_return_ HRESULT AddToSelectionImpl();
            virtual _Check_return_ HRESULT RemoveFromSelectionImpl();
            virtual _Check_return_ HRESULT get_IsSelectedImpl(_Out_ BOOLEAN* pValue);
            virtual _Check_return_ HRESULT get_SelectionContainerImpl(_Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue);

        protected:
            // IScrollItemProvider implementation common to {List,Grid}ViewItemDataAutomationPeer. Typically, this implementation would be provided via simple inheritance to the two automation peers
            // by having this class implement IScrollItemProvider.
            // However, since this class's public interface is hidden from public IDL (via SLOM.xasd), we can't do this (otherwise it would seem that {List,Grid}ViewItemDataAutomationPeer do not
            // implement IScrollItemProvider). We must have each automation peer implement IScrollItemProvider separately. These separate implementations then directly call ScrollIntoViewCommon.
            _Check_return_ HRESULT ScrollIntoViewCommon();
    };
}
