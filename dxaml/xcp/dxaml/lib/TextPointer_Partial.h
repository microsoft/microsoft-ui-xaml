// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextPointer.g.h"

namespace DirectUI
{
    // Represents the TextPointer.
    PARTIAL_CLASS(TextPointer)
    {
        private:
            TextPointerWrapper *m_pInternalPointer;

        public:
            // Initializes a new instance of the TextPointer class.
            TextPointer();

            // Destroys an instance of the TextPointer class.
            ~TextPointer() override;

            static _Check_return_ HRESULT CreateInstanceWithInternalPointer(_In_ DirectUI::TextPointerWrapper* internalPointer, _Outptr_ xaml_docs::ITextPointer** returnValue);

            _Check_return_ HRESULT get_InternalPointer(_Outptr_ DirectUI::TextPointerWrapper** pValue);
            _Check_return_ HRESULT put_InternalPointer(_In_ DirectUI::TextPointerWrapper* value);

            _Check_return_ HRESULT get_ParentImpl(_Outptr_ xaml::IDependencyObject** pValue);
            _Check_return_ HRESULT get_VisualParentImpl(_Outptr_ xaml::IFrameworkElement** pValue);
            _Check_return_ HRESULT get_LogicalDirectionImpl(_Out_ xaml_docs::LogicalDirection* pValue);
            _Check_return_ HRESULT get_OffsetImpl(_Out_ INT* pValue);

            _Check_return_ HRESULT GetCharacterRectImpl(_In_ xaml_docs::LogicalDirection direction, _Out_ wf::Rect* returnValue);
            _Check_return_ HRESULT GetPositionAtOffsetImpl(_In_ INT offset, _In_ xaml_docs::LogicalDirection direction, _Outptr_ xaml_docs::ITextPointer** returnValue);
    };
}
