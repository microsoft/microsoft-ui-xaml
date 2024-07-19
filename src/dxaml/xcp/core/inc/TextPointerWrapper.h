// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Base class of all text elements in TextBlock and RichTextBox's content model

#pragma once
class CPlainTextPosition;

#ifndef NO_HEADER_INCLUDES

#include "LogicalDirection.h"
#include "PlainTextPosition.h"

#endif

//---------------------------------------------------------------------------
//
//  TextPointerWrapper
//
//---------------------------------------------------------------------------

class CTextPointerWrapper final : public CDependencyObject
{
public:
    struct ElementEdge
    {
        enum Enum
        {
            ElementStart,
            ContentStart,
            ContentEnd,
            ElementEnd
        };
    };

    DECLARE_CREATE(CTextPointerWrapper);
    CTextPointerWrapper(_In_ CCoreServices *pCore);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextPointerWrapper>::Index;
    }

    // Plain text path for RichTextBlock/TextBlock.
    void SetPlainTextPosition(_In_ const CPlainTextPosition &plainTextPosition);
    const CPlainTextPosition& GetPlainTextPosition() const;

    // Plain text creation code path for TextBlock/RichTextBlock.
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *pCore,
        _In_ const CPlainTextPosition &plainTextPosition,
        _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
        );

    bool IsValid()
    {
        return CheckPositionAndContainerValid();
    }

    _Check_return_ HRESULT GetParent(_Outptr_ CDependencyObject **ppDependencyObject);
    _Check_return_ HRESULT GetVisualParent(_Outptr_ CFrameworkElement **ppParent);

    _Check_return_ HRESULT GetPositionAtOffset(
        _In_ XINT32                                     offset,
        _In_ RichTextServices::LogicalDirection::Enum   direction,
        _Outptr_ CTextPointerWrapper               **ppTextPointerWrapper
        );

    _Check_return_ HRESULT GetLogicalDirection(_Out_ XUINT32 *pDirectionEnumValue);
    _Check_return_ HRESULT GetCharacterRect(_In_ RichTextServices::LogicalDirection::Enum direction, _Out_ XRECTF *pRect);
    _Check_return_ HRESULT GetOffset(_Out_ XINT32 *pOffset);
    _Check_return_ HRESULT Clone(_In_ CDependencyObject *pTextOwner, _Outptr_ CTextPointerWrapper **ppTextPointerWrapper);

private:
    ~CTextPointerWrapper() override;

    bool CheckPositionAndContainerValid() const
    {
        return (m_pTextContainerRef.lock() &&
                m_plainTextPosition.IsValid());
    }

    CPlainTextPosition m_plainTextPosition;
    xref::weakref_ptr<CDependencyObject> m_pTextContainerRef;
};
