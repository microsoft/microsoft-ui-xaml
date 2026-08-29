// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef TEXT_RANGE_ADAPTER_H
#define TEXT_RANGE_ADAPTER_H

// this struct defines a node with latest TextElement to work with
// alongwith the childrenIndex needs to be targeted next in Format tree
// walking algorithm.
struct TextElementNodeInfo
{
    CTextElement* pNode;
    XINT32 currentChildindex;
    TextElementNodeInfo()
    {
        TextElementNodeInfo(nullptr, 0);
    }

    TextElementNodeInfo(_In_opt_ CTextElement* pElement, _In_ XINT32 index)
    {
        pNode = pElement;
        currentChildindex = index;
    }
};

static const DirectUI::AutomationTextAttributesEnum AttributeIdInfo[] = {
    DirectUI::AutomationTextAttributesEnum::CapStyleAttribute,
    DirectUI::AutomationTextAttributesEnum::CultureAttribute,
    DirectUI::AutomationTextAttributesEnum::FontNameAttribute,
    DirectUI::AutomationTextAttributesEnum::FontSizeAttribute,
    DirectUI::AutomationTextAttributesEnum::FontWeightAttribute,
    DirectUI::AutomationTextAttributesEnum::ForegroundColorAttribute,
    DirectUI::AutomationTextAttributesEnum::HorizontalTextAlignmentAttribute,
    DirectUI::AutomationTextAttributesEnum::IndentationFirstLineAttribute,
    DirectUI::AutomationTextAttributesEnum::IsHiddenAttribute,
    DirectUI::AutomationTextAttributesEnum::IsItalicAttribute,
    DirectUI::AutomationTextAttributesEnum::IsReadOnlyAttribute,
    DirectUI::AutomationTextAttributesEnum::IsSubscriptAttribute,
    DirectUI::AutomationTextAttributesEnum::IsSuperscriptAttribute,
    DirectUI::AutomationTextAttributesEnum::MarginBottomAttribute,
    DirectUI::AutomationTextAttributesEnum::MarginLeadingAttribute,
    DirectUI::AutomationTextAttributesEnum::MarginTopAttribute,
    DirectUI::AutomationTextAttributesEnum::MarginTrailingAttribute };

//---------------------------------------------------------------------------
//
//  CTextRangeAdapter
//
//---------------------------------------------------------------------------
class CTextRangeAdapter final : public CDependencyObject
{
public:
    DECLARE_CREATE(CTextRangeAdapter);

    CTextRangeAdapter(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
        ASSERT(GetContext());
    }

    ~CTextRangeAdapter() override;

    _Check_return_ HRESULT Initialize(
        _In_ CTextAdapter *pTextAdapter,
        _In_ CTextPointerWrapper *pStartTextPointer,
        _In_ CTextPointerWrapper *pEndTextPointer,
        _In_ CDependencyObject *pTextOwner
        )
    {
        m_pTextAdapter = pTextAdapter;
        AddRefInterface(m_pTextAdapter);

        m_pStartTextPointer = pStartTextPointer;
        AddRefInterface(m_pStartTextPointer);

        m_pEndTextPointer = pEndTextPointer;
        AddRefInterface(m_pEndTextPointer);

        m_attributeValueInfoTable = new CValue [ARRAY_SIZE(AttributeIdInfo)];
        m_pTextOwner = pTextOwner;

        return S_OK;//RRETURN_REMOVAL
    }

    _Check_return_ HRESULT Clone(_Outptr_ CTextRangeAdapter** ppCloneObject);
    _Check_return_ HRESULT Compare(_In_ CTextRangeAdapter* pTargetRange, _Out_ bool* pbRetVal);
    _Check_return_ HRESULT CompareEndpoints(_In_ UIAXcp::TextPatternRangeEndpoint endPoint,
        _In_ CTextRangeAdapter* pTargetTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint,
        _Out_ XINT32* pReturnValue);
    _Check_return_ HRESULT GetEnclosingElement(_Outptr_ CAutomationPeer** ppRetVal);
    _Check_return_ HRESULT ExpandToEnclosingUnit(_In_ UIAXcp::TextUnit unit);
    _Check_return_ HRESULT GetAttributeValue(_In_ DirectUI::AutomationTextAttributesEnum attributeID, _Out_ CValue* pRetVal);
    _Check_return_ HRESULT GetText(_In_ XUINT32 maxLength, _Out_ xstring_ptr* pstrRetVal);
    _Check_return_ HRESULT GetBoundingRectangles(_Outptr_result_buffer_all_(*pnCount) XDOUBLE** ppRectangles, _Out_ XINT32* pnCount);
    _Check_return_ HRESULT Move(_In_ UIAXcp::TextUnit unit, _In_ XINT32 count, _Out_ XINT32* pReturnValue);
    _Check_return_ HRESULT MoveEndpointByUnit(_In_ UIAXcp::TextPatternRangeEndpoint endPoint,
        _In_ UIAXcp::TextUnit unit,
        _In_ XINT32 count,
        _Out_ XINT32* pReturnValue,
        _Out_opt_ bool* pbIsEmptyLine = nullptr /*To be utilized only for Line Unit by Move Method*/);
    _Check_return_ HRESULT MoveEndpointByRange(_In_ UIAXcp::TextPatternRangeEndpoint endPoint,
        _In_ CTextRangeAdapter* pTargetTextRangeProvider,
        _In_ UIAXcp::TextPatternRangeEndpoint targetEndPoint);
    _Check_return_ HRESULT Select();
    _Check_return_ HRESULT AddToSelection();
    _Check_return_ HRESULT RemoveFromSelection();
    _Check_return_ HRESULT ScrollIntoView();
    _Check_return_ HRESULT GetChildren(_Outptr_result_buffer_all_(*pnCount) CAutomationPeer ***pppChildren, _Out_ XINT32* pnCount);

    _Check_return_ HRESULT TraverseTextElementTreeForFormat(_In_ CTextElement *pTextElementStart, _In_ XINT32 direction, _Outptr_ CTextElement** pAfterBoundaryElement, _Outptr_ CTextElement** pBeforeBoundaryElement);
    _Check_return_ HRESULT IsFormatBreaker(_In_ CTextElement *pContainingElementNext, _Out_ bool* returnValue);
    _Check_return_ HRESULT GetAttributeValueFromTextElement(_In_ DirectUI::AutomationTextAttributesEnum attributeID, _In_ CTextElement* pTextElement, _Out_ CValue* pRetValue, _In_opt_ CParagraph* pParagraph = nullptr);
    _Check_return_ HRESULT AttributeValueComparer(_In_ CValue value1, _In_ CValue value2, _Out_ bool* pRetValue);
    _Check_return_ HRESULT GetCValueStringFromCValueObject(_In_ CValue valueObject, _Out_ CValue* pValueString);
    _Check_return_ HRESULT IsHiddenRunOrLineBreak(_In_ CTextElement* pElement, _Out_ bool* isHidden);
    _Check_return_ HRESULT EnsureAttributeIdInfoForStartElement(_In_ CTextElement* pElement);
    _Check_return_ HRESULT IsAtEmptyLine(_Out_ bool* pbIsEmptyLine);

public:
// Plain text creation code path for TextBlock/RichTextBlock.
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *pCore,
        _In_ CTextAdapter *pTextAdapter,
        _In_ CTextPointerWrapper *pStartTextPointer,
        _In_ CTextPointerWrapper *pEndTextPointer,
        _In_ CDependencyObject *pTextOwner,
        _Outptr_ CTextRangeAdapter **ppTextRangeAdapter
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextRangeAdapter>::Index;
    }

    static _Check_return_ HRESULT Normalize(_In_ CTextRangeAdapter* pTextRangeAdapter);
    static _Check_return_ HRESULT ValidateAndAdjust(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ const CPlainTextPosition& nextPosition, _In_ CTextRangeAdapter* pTextRangeAdapter, _Out_opt_ bool* doAgain = nullptr);
    static _Check_return_ HRESULT ExpandToCharacter(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count, _Out_ XUINT32* pnCount);
    static _Check_return_ HRESULT ExpandToFormat(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count, _Out_ XUINT32* pnCount);
    static _Check_return_ HRESULT ExpandToWord(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count, _Out_ XUINT32* pnCount);
    static _Check_return_ HRESULT ExpandToLine(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count, _Out_ XUINT32* pnCount, _Out_opt_ bool* pbIsEmptyLine = nullptr);
    static _Check_return_ HRESULT ExpandToParagraph(_In_ UIAXcp::TextPatternRangeEndpoint endPoint, _In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count, _Out_ XUINT32* pnCount, _Out_ bool* doExecuteForNextUnit);
    static _Check_return_ HRESULT ExpandToPage(_In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count);
    static _Check_return_ HRESULT ExpandToDocument(_In_ CTextRangeAdapter* pTextRangeAdapter, _In_ XINT32 count);

    static _Check_return_ CParagraph* GetParagraphFromTextElement(_In_ CTextElement* pElement);

    static _Check_return_ HRESULT MoveByCharacter(_In_ int count, _Out_ uint32_t* movedCount, _Inout_ CPlainTextPosition* plainTextPosition);
    static _Check_return_ HRESULT MoveByWord(_In_ int count, _Out_ uint32_t* movedCount, _Inout_ CPlainTextPosition* plainTextPosition);
    static _Check_return_ HRESULT MoveToLine(_In_ bool isMovingToEnd, _In_ CTextRangeAdapter* pTextRangeAdapter, _Inout_ CPlainTextPosition* textPosition);


protected:
    CTextRangeAdapter(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CDependencyObject(pCore)
    {
        ASSERT(value.GetType() == valueObject);
        m_pTextOwner = value.AsObject();
        ASSERT(GetContext());
    }

private:
    CTextAdapter *m_pTextAdapter                = nullptr;
    CTextPointerWrapper *m_pStartTextPointer    = nullptr;
    CTextPointerWrapper *m_pEndTextPointer      = nullptr;
    CDependencyObject *m_pTextOwner             = nullptr;
    CValue* m_attributeValueInfoTable           = nullptr;

    _Check_return_ HRESULT GetLinkChildrenWithinRange(
        _Inout_ xvector<CAutomationPeer*> *pAPCollection,
        _In_ uint32_t startOffset,
        _In_ uint32_t endOffset);
    _Check_return_ HRESULT GetFocusableChildrenInRange(
        _Inout_ xvector<CAutomationPeer*> *pAPCollection,
        _In_opt_ CDOCollection *focusableChildren,
        _In_ uint32_t startOffset,
        _In_ uint32_t endOffset);
    bool IsLinkWithinRange(
        _In_ CInline* link,
        _In_ uint32_t startOffset,
        _In_ uint32_t endOffset);
    bool RangeIsInLink(_Outptr_result_maybenull_ CInline **ppLink);
};

#endif // TEXT_RANGE_ADAPTER_H
