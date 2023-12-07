// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextAdapter.g.h"

namespace DirectUI
{
    // Represents the TextAdapter
    PARTIAL_CLASS(TextAdapter),
        public ITextAdapter
    {
        public:
            // Initializes a new instance of the ItemInvokeAdapter class.
            TextAdapter();
            ~TextAdapter() override;

            // this implementation is hidden from IDL
            _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

            _Check_return_ HRESULT put_Owner(_In_ xaml::IUIElement* pOwner);

            // Customized properties.
            virtual _Check_return_ HRESULT get_DocumentRangeImpl(_Outptr_ xaml_automation::Provider::ITextRangeProvider** pValue);
            virtual _Check_return_ HRESULT get_SupportedTextSelectionImpl(_Out_ xaml_automation::SupportedTextSelection* pValue);

            // Customized methods.
            virtual _Check_return_ HRESULT GetSelectionImpl(_Out_ UINT* returnValueCount, _Outptr_ xaml_automation::Provider::ITextRangeProvider*** returnValue);
            virtual _Check_return_ HRESULT GetVisibleRangesImpl(_Out_ UINT* returnValueCount, _Outptr_ xaml_automation::Provider::ITextRangeProvider*** returnValue);
            virtual _Check_return_ HRESULT RangeFromChildImpl(_In_ xaml_automation::Provider::IIRawElementProviderSimple* childElement, _Outptr_ xaml_automation::Provider::ITextRangeProvider** returnValue);
            virtual _Check_return_ HRESULT RangeFromPointImpl(_In_ wf::Point screenLocation, _Outptr_ xaml_automation::Provider::ITextRangeProvider** returnValue);

            // Invalidates the owner to avoid any dangling ptrs, this gets called by owner's dtor.
            void InvalidateOwner();

            // This needn't be a TrackerPtr; it's a raw reference and not ref-counted.
            xaml::IUIElement* m_pOwnerNoRef;
    };
}
