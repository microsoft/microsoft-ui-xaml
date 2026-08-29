// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CFrameworkInputPaneHandler : public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
                                   public IFrameworkInputPaneHandler
{
public:
    CFrameworkInputPaneHandler()
    {
    }

    ~CFrameworkInputPaneHandler()
    {
    }

    DECLARE_NOT_AGGREGATABLE(CFrameworkInputPaneHandler)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFrameworkInputPaneHandler)
        COM_INTERFACE_ENTRY(IFrameworkInputPaneHandler)
    END_COM_MAP()

    // IFrameworkInputPaneHandler
    _Check_return_ HRESULT __stdcall Showing(
        _In_ RECT *occludedRectangle,
        _In_ BOOL ensureFocusedElementInView) override;
    _Check_return_ HRESULT __stdcall Hiding(
        _In_ BOOL ensureFocusedElementInView) override;
        
    // CFrameworkInputPaneHandler
    _Check_return_ HRESULT __stdcall HidingWithEditFocusRemoval(
        _In_ BOOL ensureFocusedElementInView);

    _Check_return_ HRESULT __stdcall HidingWithEditControlNotify(
        _In_ BOOL ensureFocusedElementInView);

    // Process InputPane handler
    _Check_return_ HRESULT SetInputPaneHandler(
        _In_ IXcpInputPaneHandler* pInputPaneHandler);

private:
    IXcpInputPaneHandler* m_pInputPaneHandler{};
};
