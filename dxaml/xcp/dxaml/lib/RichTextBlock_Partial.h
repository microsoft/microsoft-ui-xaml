// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RichTextBlock.g.h"
#include "ContextMenuEventArgs.g.h"

namespace DirectUI
{
    // Represents the RichTextBlock
    PARTIAL_CLASS(RichTextBlock)
    {
        public:
            // Initializes a new instance of the RichTextBlock class.
            RichTextBlock();
            ~RichTextBlock() override;

            _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

            _Check_return_ HRESULT get_ContentStartImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
            _Check_return_ HRESULT get_ContentEndImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
            _Check_return_ HRESULT get_SelectionStartImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
            _Check_return_ HRESULT get_SelectionEndImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
            _Check_return_ HRESULT get_BaselineOffsetImpl(_Out_ DOUBLE* pValue);

            _Check_return_ HRESULT SelectImpl(_In_ xaml_docs::ITextPointer* start, _In_ xaml_docs::ITextPointer* end);
            _Check_return_ HRESULT GetPositionFromPointImpl(_In_ wf::Point point, _Outptr_ xaml_docs::ITextPointer** returnValue);
            _Check_return_ HRESULT CopySelectionToClipboardImpl();

            static _Check_return_ HRESULT OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeRichTextBlock, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);
            static _Check_return_ HRESULT QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeTextBox);

            _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

        protected:
            
            IFACEMETHOD(OnDisconnectVisualChildren)() override;

        private:
            _Check_return_ HRESULT GetContentEdge(
                _In_ CTextPointerWrapper::ElementEdge::Enum edge,
                _Outptr_ xaml_docs::ITextPointer** pValue
                );

            _Check_return_ HRESULT GetSelectionEdge(
                _In_ CTextPointerWrapper::ElementEdge::Enum edge,
                _Outptr_ xaml_docs::ITextPointer** pValue
                );
    };
}
