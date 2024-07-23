// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Definition of block and block collection classes. This file is
//      named BlockTextElement and not Block because of a name conflict
//      with codecs class Block.h which redefines certain memutils functions,
//      like memcmp.

#pragma once

class CTextBackingStoreHost;
class CParagraphView;

#include "TextElement.h"
#include "TextElementCollection.h"
#include "TextContainer.h"

//------------------------------------------------------------------------
// Abstract class Block, acts as property holder for common block properties.
//------------------------------------------------------------------------
class CBlock : public CTextElement
{
public:
    CBlock(_In_ CCoreServices *pCore)
        : CTextElement(pCore)
    {}

    DECLARE_CREATE(CBlock);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBlock>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    virtual _Check_return_ HRESULT GetRun(
        _In_ XUINT32 characterPosition,
        _Out_opt_ const TextFormatting **ppTextFormatting,
        _Out_opt_ const InheritedProperties **ppInheritedProperties,
        _Out_opt_ TextNestingType *pNestingType,
        _Outptr_result_maybenull_ CTextElement **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_ XUINT32 *pcCharacters
        )
    {
        ASSERT(FALSE);
        return E_UNEXPECTED; // Implemented in derived classes of Block.
    }

    virtual _Check_return_ HRESULT GetElementEdgeOffset(
        _In_ CTextElement *pElement,
        _In_ ElementEdge edge,
        _Out_ XUINT32 *pOffset,
        _Out_ bool *pFound
    )
    {
        ASSERT(FALSE);
        return E_UNEXPECTED; // Implemented in derived classes of Block.
    }

    DirectUI::TextAlignment         m_textAlignment           = DirectUI::TextAlignment::Left;
    DirectUI::LineStackingStrategy  m_lineStackingStrategy    = DirectUI::LineStackingStrategy::MaxHeight;
    XFLOAT                          m_lineHeight              = 0.0f;
    XTHICKNESS                      m_margin                  = {};

private:
    bool ValidateMargin(_In_ const CValue *pValue);
};

//------------------------------------------------------------------------
// Paragraph class, it can have children of type Inline
//------------------------------------------------------------------------
class CParagraph final : public CBlock
{
public:
    CParagraph(_In_ CCoreServices *pCore)
        : CBlock(pCore)
    {}

    ~CParagraph() override;

    _Check_return_ static HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CParagraph>::Index;
    }

    _Check_return_ HRESULT GetValue(
        _In_  const CDependencyProperty *pdp,
        _Out_ CValue *pValue
        ) override;

    _Check_return_ HRESULT EnterImpl(
        _In_ CDependencyObject *pNamescopeOwner,
        EnterParams params
        ) override;

    // APIs for accessing the text in the Paragraph.
    void GetPositionCount(_Out_ XUINT32 *pcPositions) override;

    _Check_return_ HRESULT GetRun(
        _In_ XUINT32 characterPosition,
        _Out_opt_ const TextFormatting **ppTextFormatting,
        _Out_opt_ const InheritedProperties **ppInheritedProperties,
        _Out_opt_ TextNestingType *pNestingType,
        _Out_opt_ CTextElement **ppNestedElement,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_ XUINT32 *pcCharacters
        ) override;
    _Check_return_ HRESULT GetContainingElement(
        _In_ XUINT32 characterPosition,
        _Outptr_ CTextElement **ppContainingElement
        ) override;
    _Check_return_ HRESULT GetElementEdgeOffset(
        _In_ CTextElement *pElement,
        _In_ ElementEdge edge,
        _Out_ XUINT32 *pOffset,
        _Out_ bool *pFound
        ) override;

    _Check_return_ HRESULT Shutdown();

    CInlineCollection* GetInlineCollection() override
    {
        return m_pInlines;
    }

    CInlineCollection *m_pInlines   = nullptr;
    XFLOAT             m_textIndent = 0.0f;

private:
    _Check_return_ HRESULT CreateInlineCollection();
};

//------------------------------------------------------------------------
// Collection of block elements
//------------------------------------------------------------------------
class CBlockCollection final : public CTextElementCollection,
                         private ITextContainer
{
public:
    DECLARE_CREATE(CBlockCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBlockCollection>::Index;
    }

    // ITextContainer overrides.
    CCoreServices *GetCore() override
    {
        return GetContext(); // Forwarding to DependencyObject.GetContext()
    }

    CDependencyObject *GetAsDependencyObject() override
    {
        return this;
    }
    void GetPositionCount(_Out_ XUINT32 *pcPositions) override;

    _Check_return_ HRESULT GetRun(
        _In_ XUINT32 characterPosition,
        _Out_opt_ const TextFormatting **ppTextFormatting,
        _Out_opt_ const InheritedProperties **ppInheritedProperties,
        _Out_opt_ TextNestingType *pNestingType,
        _Out_opt_ CTextElement **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_ XUINT32 *pcCharacters) override;

    virtual _Check_return_ HRESULT GetText(
        _In_ XUINT32   charPosition1,
        _In_ XUINT32   charPosition2,
        _In_ bool     insertNewlines,
        _Out_ CString **ppString);

    _Check_return_ HRESULT GetText(
        _In_  bool    insertNewlines,
        _Out_ XUINT32 *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_                                 bool  *pIsTakeOwnership) override;

    _Check_return_ HRESULT GetText(
        _In_  XUINT32 iTextPosition1,
        _In_  XUINT32 iTextPosition2,
        _In_  bool   insertNewlines,
        _Out_ XUINT32 *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_                                 bool  *pIsTakeOwnership) override;

    _Check_return_ HRESULT GetContainingElement(
        _In_ XUINT32 characterPosition,
        _Outptr_ CTextElement **ppContainingElement) override;

    _Check_return_ HRESULT GetElementEdgeOffset(
        _In_ CTextElement *pElement,
        _In_ ElementEdge edge,
        _Out_ XUINT32 *pOffset,
        _Out_ bool *pFound) override;

    CUIElement *GetOwnerUIElement() override;

    // MarkDirty override, clears cached lengths.
    _Check_return_ HRESULT MarkDirty(
        _In_opt_ const CDependencyProperty *pdp) override;

    ITextContainer *GetTextContainer() { return this; }

private:
    // Lengths of nested blocks.
    XUINT32 *m_pLengths;

    // Collection's length, sum of all block lengths.
    XUINT32 m_length;

    CBlockCollection(_In_ CCoreServices *pCore);
    ~CBlockCollection() override;

    // Caches lengths of all blocks in the collection for quick lookup.
    void CacheLengths();
};



