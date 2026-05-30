// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextRangeAdapter.g.h"

namespace DirectUI
{
    // Represents the TextRangeAdapter
    PARTIAL_CLASS(TextRangeAdapter),
        public ITextRangeAdapter
    {
        public:
            // Initializes a new instance of the TextRangeAdapter class.
            TextRangeAdapter();
            ~TextRangeAdapter() override;

            // this implementation is hidden from IDL
            _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

            _Check_return_ HRESULT put_Owner(_In_ TextAdapter* pOwner);

            // Customized methods.
            virtual _Check_return_ HRESULT CloneImpl(_Outptr_ xaml_automation::Provider::ITextRangeProvider** returnValue);
            virtual _Check_return_ HRESULT CompareImpl(_In_ xaml_automation::Provider::ITextRangeProvider* textRangeProvider, _Out_ BOOLEAN* returnValue);
            virtual _Check_return_ HRESULT CompareEndpointsImpl(_In_ xaml_automation::Text::TextPatternRangeEndpoint endpoint, _In_ xaml_automation::Provider::ITextRangeProvider* textRangeProvider, _In_ xaml_automation::Text::TextPatternRangeEndpoint targetEndpoint, _Out_ INT* returnValue);
            virtual _Check_return_ HRESULT ExpandToEnclosingUnitImpl(_In_ xaml_automation::Text::TextUnit unit);
            virtual _Check_return_ HRESULT FindAttributeImpl(_In_ INT attributeId, _In_ IInspectable* value, _In_ BOOLEAN backward, _Outptr_ xaml_automation::Provider::ITextRangeProvider** returnValue);
            virtual _Check_return_ HRESULT FindTextImpl(_In_ HSTRING text, _In_ BOOLEAN backward, _In_ BOOLEAN ignoreCase, _Outptr_ xaml_automation::Provider::ITextRangeProvider** returnValue);
            virtual _Check_return_ HRESULT GetAttributeValueImpl(_In_ INT attributeId, _Outptr_ IInspectable** returnValue);
            virtual _Check_return_ HRESULT GetBoundingRectanglesImpl(_Out_ UINT* returnValueCount, _Out_ DOUBLE** returnValue);
            virtual _Check_return_ HRESULT GetEnclosingElementImpl(_Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** returnValue);
            virtual _Check_return_ HRESULT GetTextImpl(_In_ INT maxLength, _Out_ HSTRING* returnValue);
            virtual _Check_return_ HRESULT MoveImpl(_In_ xaml_automation::Text::TextUnit unit, _In_ INT count, _Out_ INT* returnValue);
            virtual _Check_return_ HRESULT MoveEndpointByUnitImpl(_In_ xaml_automation::Text::TextPatternRangeEndpoint endpoint, _In_ xaml_automation::Text::TextUnit unit, _In_ INT count, _Out_ INT* returnValue);
            virtual _Check_return_ HRESULT MoveEndpointByRangeImpl(_In_ xaml_automation::Text::TextPatternRangeEndpoint endpoint, _In_ xaml_automation::Provider::ITextRangeProvider* textRangeProvider, _In_ xaml_automation::Text::TextPatternRangeEndpoint targetEndpoint);
            virtual _Check_return_ HRESULT SelectImpl();
            virtual _Check_return_ HRESULT AddToSelectionImpl();
            virtual _Check_return_ HRESULT RemoveFromSelectionImpl();
            virtual _Check_return_ HRESULT ScrollIntoViewImpl(_In_ BOOLEAN alignToTop);
            virtual _Check_return_ HRESULT GetChildrenImpl(_Out_ UINT* returnValueCount, _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** returnValue);



            // This needn't be a TrackerPtr; it's a raw reference and not ref-counted.
            TrackerPtr<xaml_automation::Provider::ITextProvider> m_tpTextPattern;
    };
}
