// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Base class of all text elements in TextBlock and RichTextBox's content model

#pragma once

#include "framework.h"

class CTextPointerWrapper;
struct ITextContainer;
class CPlainTextPosition;
struct ITextView;

enum ElementEdge
{
    ContentStart,
    ContentEnd,
    ElementStart,
    ElementEnd
};

#ifndef NO_HEADER_INCLUDES

#include "ITextElement.h"        // ITextElement

#endif

//---------------------------------------------------------------------------
//
//  TextElement
//
//---------------------------------------------------------------------------

class CTextElement : public CDependencyObject
{
public:
    CTextElement(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    ~CTextElement() override;

    virtual void GetPositionCount(_Out_ XUINT32 *pcPositions)
    {
        // Base class always returns 2 for start/end edges. Elements with different counts/content can
        // override.
        *pcPositions = 2;
    }

    virtual _Check_return_ HRESULT GetContainingElement(
        _In_ XUINT32 characterPosition,
        _Outptr_result_maybenull_ CTextElement **ppContainingElement
        )
    {
        XUINT32 cPositions = 0;

        GetPositionCount(&cPositions);
        IFCEXPECT_RETURN(characterPosition < cPositions);

        // 0 is viewed as the element start and is not technically contained in the element.
        // E.g. an empty TextElement has a position count of 2: 0<Element>1</Element>2
        // 0 is before the element start, 2 is after the end, and only 1 is contained. So offsets
        // > 0 and < position count are contained by the element.
        if (characterPosition > 0)
        {
            *ppContainingElement = this;
        }
        else
        {
            *ppContainingElement = nullptr;
        }

        return S_OK;
    }

    // CDependencyObject overrides
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT MarkInheritedPropertyDirty(
       _In_ const CDependencyProperty* pdp,
       _In_ const CValue* pValue) final;

    // Gets the offset corresponding to an element edge.
    _Check_return_ HRESULT GetOffsetForEdge(
        _In_ ElementEdge edge,
        _Out_ XUINT32 *pOffset
        );

    CAutomationPeer* OnCreateAutomationPeerInternal();

public:
    DECLARE_CREATE(CTextElement);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextElement>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // Inherited property support
    TextFormatting *m_pTextFormatting = nullptr;
    TextFormatting **GetTextFormattingMember() final { return &m_pTextFormatting; }
    _Check_return_ HRESULT PullInheritedTextFormatting() final;
    bool HasInheritedProperties() final { return true; }

    _Check_return_ HRESULT GetTextPointer(_In_ ElementEdge edge, _Outptr_ CTextPointerWrapper **ppTextPointerWrapper);

    _Check_return_ HRESULT GetContentStart(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper);
    _Check_return_ HRESULT GetContentEnd(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper);
    _Check_return_ HRESULT GetElementStart(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper);
    _Check_return_ HRESULT GetElementEnd(_Outptr_ CTextPointerWrapper **ppTextPointerWrapper);

    _Check_return_ HRESULT MarkDirty(_In_opt_ const CDependencyProperty *pdp);
    CFrameworkElement *GetContainingFrameworkElement();

    virtual CInlineCollection* GetInlineCollection()
    {
        return nullptr;
    }

    bool RaiseAccessKeyInvoked();
    void RaiseAccessKeyShown(_In_z_ const wchar_t* strPressedKeys);
    void RaiseAccessKeyHidden();

private:

    // TextPointer/CTextPosition retrieval.
    _Check_return_ HRESULT GetElementEdgePositionFromTextContainer(
        _In_ ElementEdge edge,
        _In_ ITextContainer *pTextContainer,
        _Out_ CPlainTextPosition *pPlainTextPosition
        );
};
