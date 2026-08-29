// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCommonPrintingData : public CXcpObjectBase <IPALPrintingData2>
{
public:
    ////////////////////////////////
    // IPALPrintingData Members
    ////////////////////////////////
    CCommonPrintingData();

    void  SetDidPrintingStart(bool fPrintingStarted) final;
    _Check_return_ bool GetDidPrintingStart() final;

    _Check_return_ HRESULT SetDocumentName(
        _In_ const XINT32  cLength,
        _In_reads_((cLength+1)) const WCHAR *pDocumentName) final;
    _Check_return_ WCHAR* GetDocumentName() final;

    _Check_return_ HRESULT SetScaleX(XFLOAT fScaleX) final;
    _Check_return_ XFLOAT GetScaleX() final;

    _Check_return_ HRESULT SetScaleY(XFLOAT fScaleY) final;
    _Check_return_ XFLOAT GetScaleY() final;

    _Check_return_ HRESULT SetPageRangeFrom(XINT32 nPageRangeFrom) final;
    _Check_return_ XINT32 GetPageRangeFrom() final;

    _Check_return_ HRESULT SetPageRangeTo(XINT32 nPageRangeTo) final;
    _Check_return_ XINT32 GetPageRangeTo() final;

    _Check_return_ HRESULT SetPrintableAreaWidth(XUINT32 nPrintableAreaWidth) final;
    _Check_return_ XUINT32 GetPrintableAreaWidth() final;

    _Check_return_ HRESULT SetPrintableAreaHeight(XUINT32 nPrintableAreaHeight) final;
    _Check_return_ XUINT32 GetPrintableAreaHeight() final;

    _Check_return_ HRESULT SetPageMargins(XTHICKNESS tPageMargins) final;
    _Check_return_ XTHICKNESS GetPageMargins() final;

    _Check_return_ bool GetSupportsPostscript() final;
    void SetSupportsPostscript(bool value) final;

    bool GetIsPrintFormatPostscript() final;
    void SetPrintFormatToPostscript(bool value) final;

protected:
    ~CCommonPrintingData() override;

protected:
    bool m_fPrintingStarted;

    WCHAR*  m_pszDocumentName;

    XFLOAT  m_fScaleX;
    XFLOAT  m_fScaleY;

    XINT32  m_nPageRangeFrom;
    XINT32  m_nPageRangeTo;

    XUINT32 m_nPrintableAreaWidth;
    XUINT32 m_nPrintableAreaHeight;

    XTHICKNESS m_tPageMargins;
    bool m_fSupportsPostscript;
    bool m_fIsPrintFormatPostscript;
};

