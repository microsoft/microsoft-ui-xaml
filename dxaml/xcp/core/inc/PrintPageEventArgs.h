// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      EventArgs used for printing.
//      Used by the PrintPage event to enable app authors to print a
//      UIElement by assigning it to PageVisual and whether there are
//      more pages to print or not by setting HasMorePages among other
//      things.

class CPrintPageEventArgs final : public CEventArgs
{
    friend class CPrintDocument;

public:
    CPrintPageEventArgs(_In_ CCoreServices* pCore)
    {
        m_pCore = pCore;
        m_pPageVisual = NULL;
        m_fHasMorePages = FALSE;
        m_sPrintableArea.width = 0.0f;
        m_sPrintableArea.height = 0.0f;
    }

    ~CPrintPageEventArgs() override;

    _Check_return_ HRESULT get_PageVisual(_Outptr_ CUIElement** ppPageVisual);
    _Check_return_ HRESULT put_PageVisual(_In_opt_ CUIElement* pPageVisual);

    bool HasMorePages()
    {
        return m_fHasMorePages;
    }

    virtual _Check_return_ HRESULT ClearPageVisual();

    _Check_return_ HRESULT SetPrintableArea(_In_ XFLOAT fWidth, _In_ XFLOAT fHeight);

    _Check_return_ HRESULT SetPageMargins(_In_ XTHICKNESS pageMargins);

public:
    CUIElement *m_pPageVisual;
    bool       m_fHasMorePages;
    XSIZEF      m_sPrintableArea;
    XTHICKNESS  m_tPageMargins{};

private:
    CCoreServices* m_pCore;
};

class CPaginateEventArgs final : public CEventArgs
{
public:
    CPaginateEventArgs()
    {
    }

    ~CPaginateEventArgs() override {}
};

class CAddPagesEventArgs final : public CEventArgs
{
public:
    CAddPagesEventArgs()
    {
    }

    ~CAddPagesEventArgs() override {}
};

class CGetPreviewPageEventArgs final : public CEventArgs
{
public:
    CGetPreviewPageEventArgs()
    {
        m_pageNumber = 0;
    }

    ~CGetPreviewPageEventArgs() override {}

    XINT32 m_pageNumber;
};
