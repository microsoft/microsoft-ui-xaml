// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextContainer.h"
#include "TextElementCollection.h"

struct ITextView;

namespace RichTextServices
{
    class TextPosition;
    class TextSource;
};

//------------------------------------------------------------------------
//
//  Class:  CInlineCollection
//
//  Synopsis:
//      Holds a list of generic inline values
//
//------------------------------------------------------------------------

class CInlineCollection final : public CTextElementCollection, public ITextContainer
{
public:
// Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_     CREATEPARAMETERS   *pCreate
        );

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInlineCollection>::Index;
    }

// CCollection overrides

    _Check_return_ HRESULT Neat(_In_ bool bBreak) override
    {
        delete [] m_pPositionCounts;
        m_pPositionCounts = NULL;
        m_cCollectionPositions = 0;
        return CDOCollection::Neat(bBreak);
    }

    _Check_return_ HRESULT Insert(
        _In_ XUINT32            nIndex,
        _In_ CDependencyObject *pObject
    ) override;

    // Append an Inline or CString.
    _Check_return_ HRESULT Append(
        _In_ CDependencyObject *pObject,
        _Out_opt_ XUINT32 *pnIndex = NULL
    ) override;

    // Append a CValue representing an Inline, CString, or literal xstring_ptr.
    _Check_return_ HRESULT Append(
        _In_ CValue& value,
        _Out_opt_ XUINT32 *pnIndex = NULL
    ) override;

    // Append literal text to the InlineCollection by wrapping it in a new
    // default Run.
    _Check_return_ HRESULT AppendText(
        _In_ const xstring_ptr& strText,
        _Out_opt_ XUINT32 *pnIndex = NULL
    );

    _Check_return_ HRESULT MarkDirty(
        _In_opt_ const CDependencyProperty *pdp
    ) override;

    virtual void CachePositionCounts();

// ITextContainer methods for access to character content

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
        _In_                              XUINT32               characterPosition,
        _Out_opt_                   const TextFormatting      **ppTextFormatting,
        _Out_opt_                   const InheritedProperties **ppInheritedProperties,
        _Out_opt_                         TextNestingType      *pNestingType,
        _Out_opt_                         CTextElement        **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
        _Out_                             XUINT32              *pcCharacters
    ) override;

    // TODO: Evaluate if this and the next GetText() need to be virtual
    virtual _Check_return_ HRESULT GetText(
        _In_  bool     insertNewlines,
        _Out_ CString **pText);

    virtual _Check_return_ HRESULT GetText(
        _In_ XUINT32   iTextPosition1,
        _In_ XUINT32   iTextPosition2,
        _In_ bool     insertNewlines,
        _Out_ CString **ppString);

    _Check_return_ HRESULT GetText(
        _In_                                    bool      insertNewlines,
        _Out_                                   XUINT32   *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters)   const WCHAR **ppCharacters,
        _Out_                                   bool    *pIsTakeOwnership) override;

     _Check_return_ HRESULT GetText(
        _In_                                  XUINT32   iTextPosition1,
        _In_                                  XUINT32   iTextPosition2,
        _In_                                  bool     insertNewlines,
        _Out_                                 XUINT32  *pcCharacters,
        _Outptr_result_buffer_maybenull_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_                                 bool    *pIsTakeOwnership) override;

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

    CUIElement *GetOwnerUIElement() override;

// CInlineCollection specific methods

    CDependencyObject *GetOwnerDO() const;

    virtual ITextView *GetTextView()
    {
        // This method exists on ITextView interface for TextBox hit-testing needs.
        // Since TextBlock does not support hit-testing, this code should never be hit!
        ASSERT(FALSE);
        return NULL;
    }

    _Check_return_ HRESULT ValidateTextElement(_In_ CTextElement *pTextElement) override;

    ITextContainer *GetTextContainer() { return this; }

private:
    CInlineCollection(_In_ CCoreServices *pCore)
        : CTextElementCollection(pCore)
    {}

    ~CInlineCollection() override;

private:
    XUINT32*    m_pPositionCounts   = nullptr;  // Count of position covered by each nested inline
    XUINT32 m_cCollectionPositions  = 0;
};

//------------------------------------------------------------------------
//
//  Class:  CInline
//
//  Synopsis:
//      Created by XML parser to hold text runs and spans
//
//------------------------------------------------------------------------

class CInline : public CTextElement
{
protected:
    CInline(_In_ CCoreServices *pCore) : CTextElement(pCore)
    {
    }

    ~CInline() override
    {
    }

public:

    DECLARE_CREATE(CInline);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInline>::Index;
    }

    virtual _Check_return_ HRESULT GetRun(
        _In_                              XUINT32               characterPosition,
        _Out_opt_                   const TextFormatting      **ppTextFormatting,
        _Out_opt_                   const InheritedProperties **ppInheritedProperties,
        _Out_opt_                         TextNestingType      *pNestingType,
        _Outptr_result_maybenull_         CTextElement        **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR     **ppCharacters,
        _Out_                             XUINT32              *pcCharacters)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED; // Implemented in derived classes of Inline
    }

    virtual _Check_return_ HRESULT GetElementEdgeOffset(
        _In_ CTextElement *pElement,
        _In_ ElementEdge edge,
        _Out_ XUINT32 *pOffset,
        _Out_ bool *pFound
    );


};

//------------------------------------------------------------------------
//
//  Enum:  RunFlags
//
//  RunFlags record the context in which a run was parsed and are used
//  to control whitespace collapsing and/or removal.
//
//------------------------------------------------------------------------

enum RunFlags // Note: run flags are powers of 2 and may be combined
{
    RunFlagsNone         = 0,
    RunFlagsSpaceDefault = 1,
    RunFlagsRunElement   = 2,  // iff this text came explicitly from a run element
                               // (as opposed to xmlText content in the TextBlock)
    RunFlagsContent      = 4   // iff this text came from xmlText or xmlWhitespace
                               // (as opposed to a Text=".." property)
};


//------------------------------------------------------------------------
//
//  Class:  CRun
//
//  Synopsis:
//
//      Object created for <Run> tag.
//
//------------------------------------------------------------------------

class CRun final : public CInline
{
private:
    CRun(_In_ CCoreServices *pCore) : CInline(pCore)
    {
        m_flags          = RunFlagsRunElement | RunFlagsSpaceDefault;
    }

    ~CRun() override
    {
    }

    XUINT32 m_flags;

public:
    xstring_ptr m_strText;


    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,  _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRun>::Index;
    }

    _Check_return_ HRESULT GetValue(_In_ const CDependencyProperty *pdp, _Out_ CValue *pValue) override;
    bool IsRightToLeft() final;

    void GetPositionCount(_Out_ XUINT32 *pcPositions) override;

    _Check_return_ HRESULT GetRun(
        _In_                              XUINT32               characterPosition,
        _Out_opt_                   const TextFormatting      **ppTextFormatting,
        _Out_opt_                   const InheritedProperties **ppInheritedProperties,
        _Out_opt_                         TextNestingType      *pNestingType,
        _Outptr_result_maybenull_                   CTextElement        **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
        _Out_                             XUINT32              *pcCharacters
    ) override;

    virtual _Check_return_ HRESULT SetText(
        _In_ const xstring_ptr& strCharacters,
        _In_ XUINT32 flags
    );

    XUINT32 GetFlags() { return m_flags; }

    bool IsInsideHyperlink(_Outptr_result_maybenull_ CHyperlink** hyperlink);

private:
    // Helpers to pull text from backing store or m_strText
    XINT32 GetTextLength();

    _Check_return_ HRESULT GetText(
        _Outptr_result_buffer_(*pcCharacters) const WCHAR **ppCharacters,
        _Out_                             XUINT32      *pcCharacters);
};

//------------------------------------------------------------------------
//
//  Class:  CSpan
//
//  Synopsis:
//
//      Object created for <Span> tag.
//
//------------------------------------------------------------------------

class CSpan : public CInline
{
protected:
    CSpan(_In_ CCoreServices *pCore);
    ~CSpan() override;

    // Public properties
public:
    CInlineCollection *m_pInlines;

public:
    DECLARE_CREATE(CSpan);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSpan>::Index;
    }

    _Check_return_ HRESULT GetValue(
        _In_  const CDependencyProperty *pdp,
        _Out_ CValue              *pValue
    ) final;

    // Text content access APIs
    void GetPositionCount(_Out_ XUINT32 *pcPositions) final;

    _Check_return_ HRESULT GetRun(
        _In_                              XUINT32               characterPosition,
        _Out_opt_                   const TextFormatting      **ppTextFormatting,
        _Out_opt_                   const InheritedProperties **ppInheritedProperties,
        _Out_opt_                         TextNestingType      *pNestingType,
        _Outptr_result_maybenull_                   CTextElement        **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
        _Out_                             XUINT32              *pcCharacters
    ) final;

    _Check_return_ HRESULT GetContainingElement(
        _In_ XUINT32 characterPosition,
        _Outptr_ CTextElement **ppContainingElement
    ) final;
    _Check_return_ HRESULT GetElementEdgeOffset(
        _In_ CTextElement *pElement,
        _In_ ElementEdge edge,
        _Out_ XUINT32 *pOffset,
        _Out_ bool *pFound
    ) final;

    CInlineCollection* GetInlineCollection() final
    {
        return m_pInlines;
    }

protected:
    _Check_return_ HRESULT CreateInlines();
};

//------------------------------------------------------------------------
//
//  Class:  CUnderline
//
//  Synopsis:
//
//      Object created for <Underline> tag.
//
//------------------------------------------------------------------------

 class CUnderline : public CSpan
 {
 private:
     CUnderline(_In_ CCoreServices *pCore) : CSpan(pCore)
     {
     }

 public:
     static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate
        );

     KnownTypeIndex GetTypeIndex() const override
     {
         return DependencyObjectTraits<CUnderline>::Index;
     }
 };

 //------------------------------------------------------------------------
 //
 //  Class:  CItalic
 //
 //  Synopsis:
 //
 //      Object created for <Italic> tag.
 //
 //------------------------------------------------------------------------

 class CItalic : public CSpan
 {
 private:
     CItalic(_In_ CCoreServices *pCore) : CSpan(pCore)
     {
     }

 public:
     static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate
        );

     KnownTypeIndex GetTypeIndex() const override
     {
         return DependencyObjectTraits<CItalic>::Index;
     }
 };

 //------------------------------------------------------------------------
 //
 //  Class:  CBold
 //
 //  Synopsis:
 //
 //      Object created for <Bold> tag.
 //
 //------------------------------------------------------------------------

 class CBold : public CSpan
 {
 private:
     CBold(_In_ CCoreServices *pCore) : CSpan(pCore)
     {
     }

 public:
     static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate
        );

     KnownTypeIndex GetTypeIndex() const override
     {
         return DependencyObjectTraits<CBold>::Index;
     }
 };

//------------------------------------------------------------------------
//
//  Class:  CLineBreak
//
//  Synopsis:
//
//      Object created for <Run> tag.
//
//------------------------------------------------------------------------

class CLineBreak final : public CInline
{
private:
    CLineBreak(_In_ CCoreServices *pCore) : CInline(pCore) {}

public:
    DECLARE_CREATE(CLineBreak);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLineBreak>::Index;
    }

    // LineBreak's GetRun just returns a Unicode line separator character
    _Check_return_ HRESULT GetRun(
        _In_                              XUINT32               characterPosition,
        _Out_opt_                   const TextFormatting      **ppTextFormatting,
        _Out_opt_                   const InheritedProperties **ppInheritedProperties,
        _Out_opt_                         TextNestingType      *pNestingType,
        _Outptr_result_maybenull_                   CTextElement        **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR         **ppCharacters,
        _Out_                             XUINT32              *pcCharacters
    ) override;
};
