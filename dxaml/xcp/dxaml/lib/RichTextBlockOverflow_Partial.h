// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RichTextBlockOverflow.g.h"

namespace DirectUI
{
    // Represents the RichTextBlockOverflow
    PARTIAL_CLASS(RichTextBlockOverflow)
    {
        public:
            // Initializes a new instance of the TextBlock class.
            RichTextBlockOverflow();
            ~RichTextBlockOverflow() override;

            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;
            _Check_return_ HRESULT GetPositionFromPointImpl(_In_ wf::Point point, _Outptr_ xaml_docs::ITextPointer** returnValue);

            // Property getters.
            _Check_return_ HRESULT get_ContentSourceImpl(_Outptr_ xaml_controls::IRichTextBlock** pValue);
            _Check_return_ HRESULT get_ContentStartImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
            _Check_return_ HRESULT get_ContentEndImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
            _Check_return_ HRESULT get_BaselineOffsetImpl(_Out_ DOUBLE* pValue);

        private:
            _Check_return_ HRESULT GetContentEdge(
                _In_ CTextPointerWrapper::ElementEdge::Enum edge,
                _Outptr_ xaml_docs::ITextPointer** pValue
                );
    };
}
