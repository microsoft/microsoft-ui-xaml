// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef TEXT_ADAPTER_H
#define TEXT_ADAPTER_H

class CTextBlock;
class PageNode;
class TextSelectionManager;

//---------------------------------------------------------------------------
//
//  CTextAdapter
//
//---------------------------------------------------------------------------

class CTextAdapter final : public CDependencyObject
{
public:
    CTextAdapter(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
        ASSERT(GetContext());
    }

    ~CTextAdapter() override;

public:
    // Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        )
    {
        HRESULT hr = S_OK;
        CTextAdapter *pObject = nullptr;

        IFCEXPECT(pCreate);
        if (pCreate->m_value.GetType() != valueObject)
        {
            IFC(E_NOTIMPL);
        }
        else
        {
            pObject = new CTextAdapter(pCreate->m_pCore, pCreate->m_value);
            IFC(ValidateAndInit(pObject, ppObject));

            // On success we've transferred ownership

            pObject = nullptr;
        }

    Cleanup:
        delete pObject;
        return hr;
    }

    static _Check_return_ HRESULT GetContentEndPointers(
        _In_ CDependencyObject* pObject,
        _Outptr_ CTextPointerWrapper **ppStartTextPointerWrapper,
        _Outptr_ CTextPointerWrapper **ppEndTextPointerWrapper);

    static _Check_return_ HRESULT GetSelectionEndPointers(
        _In_ CDependencyObject* pObject,
        _Outptr_ CTextPointerWrapper **ppStartTextPointerWrapper,
        _Outptr_ CTextPointerWrapper **ppEndTextPointerWrapper);

    static _Check_return_ ITextView* GetTextView(_In_ CDependencyObject* pObject);
    static _Check_return_ ITextContainer* GetTextContainer(_In_ CDependencyObject* pObject);
    static _Check_return_ CBlockCollection* GetBlockCollection(_In_ CDependencyObject* pObject);
    static _Check_return_ PageNode* GetPageNode(_In_ CDependencyObject* pObject);
    static _Check_return_ TextSelectionManager* GetSelectionManager(_In_opt_ CDependencyObject* pObject);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextAdapter>::Index;
    }

    _Check_return_ HRESULT GetDocumentRange(_Outptr_ CTextRangeAdapter **pTextRangeAdapter);
    _Check_return_ HRESULT GetSupportedTextSelection(_Out_ UIAXcp::SupportedTextSelection *pRetVal);
    _Check_return_ HRESULT GetSelection(_Outptr_result_buffer_all_(*pnCount) CTextRangeAdapter ***pppSelectedTextRangeAdapter, _Out_ XINT32* pnCount);
    _Check_return_ HRESULT GetVisibleRanges(_Outptr_result_buffer_all_(*pnCount) CTextRangeAdapter ***pppVisibleTextRangeAdapter, _Out_ XINT32* pnCount);
    _Check_return_ HRESULT RangeFromChild(_In_ CAutomationPeer* pAp, _Outptr_ CTextRangeAdapter **pTextRangeAdapter);
    _Check_return_ HRESULT RangeFromLink(_In_ CAutomationPeer* pAP, _In_ XPOINTF point, _Outptr_ CTextRangeAdapter **ppTextRangeAdapter);
    _Check_return_ HRESULT RangeFromPoint(_In_ XPOINTF point, _Outptr_ CTextRangeAdapter **pTextRangeAdapter);
    _Check_return_ HRESULT RangeFromInlineUIContainer(_In_ CInlineUIContainer* iuc, _Outptr_ CTextRangeAdapter **ppTextRangeAdapter);
    _Check_return_ HRESULT EnsureTextBlockView(); // Currently only normal TextBlock will support proper TextPattern features.

protected:
    CTextAdapter(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CDependencyObject(pCore)
    {
        ASSERT(value.GetType() == valueObject);
        m_pTextOwner = value.AsObject();
        VERIFYHR(EnsureTextBlockView());
        ASSERT(GetContext());
    }

private:
    CDependencyObject *m_pTextOwner = nullptr;

    CInlineUIContainer* GetParentInlineUIContainer(_In_ CAutomationPeer* pAP);
};

#endif // TEXT_ADAPTER_H
