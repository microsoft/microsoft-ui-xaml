// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpinputpanehandler.h"

struct IPALInputPaneInteraction;
class CFrameworkInputPaneHandler;
class CContentRoot;

class CInputPaneInteractionHelper : CXcpObjectBase<IPALInputPaneInteraction>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ IXcpInputPaneHandler* pInputPaneHandler,
        _Outptr_ CInputPaneInteractionHelper **ppInputPaneInteractionHelper);

    // IPALInputPaneService
    _Check_return_ HRESULT RegisterInputPaneHandler(
        _In_ XHANDLE hWindow, _In_ CContentRoot* contentRoot) override;
    _Check_return_ HRESULT UnregisterInputPaneHandler() override;

private:
    CInputPaneInteractionHelper(_In_ IXcpInputPaneHandler* pInputPaneHandler);
    ~CInputPaneInteractionHelper();

private:
    IXcpInputPaneHandler* m_pInputPaneHandler;
    Microsoft::WRL::ComPtr<IFrameworkInputPane> m_pInputPane;
    IFrameworkInputPaneHandler* m_pFrameworkInputPaneHandler;
    DWORD m_dwCookie;
};
