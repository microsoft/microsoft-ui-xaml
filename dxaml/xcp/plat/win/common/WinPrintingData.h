// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CommonPrintingData.h"

class CWinPrintingData final : public CCommonPrintingData
{
public:
    CWinPrintingData();

    virtual _Check_return_ HRESULT SetPrinterHandle(HDC hPrinter);
    virtual _Check_return_ HDC GetPrinterHandle();

    virtual               void  SetShouldStretch(bool fStretch);
    virtual _Check_return_ bool GetShouldStretch();

    virtual _Check_return_ HRESULT SetPrinterWidthInPixels(XUINT32 nPrinterWidthInPixels);
    virtual _Check_return_ XUINT32 GetPrinterWidthInPixels();

    virtual _Check_return_ HRESULT SetPrinterHeightInPixels(XUINT32 nPrinterHeightInPixels);
    virtual _Check_return_ XUINT32 GetPrinterHeightInPixels();

    void SetSupportedExtEscapePassthroughMode(int passthroughMode);
    int GetSupportedExtEscapePassthroughMode();

protected:
    ~CWinPrintingData() override;

private:
    HDC     m_hPrinter;
    bool   m_fStretch;
    XUINT32 m_nPrinterWidthInPixels;
    XUINT32 m_nPrinterHeightInPixels;
    int     m_passthroughMode;
};

