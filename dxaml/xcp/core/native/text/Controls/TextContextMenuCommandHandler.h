// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.ui.popups.h>
#include <windows.foundation.h>
#include <fwd/windows.ui.popups.h>

class CUIElement;

//---------------------------------------------------------------------------
//
//  Callback used to dispatch context menu selections on text elements and
//  controls.  Specific UIElements such as TextBlock have derived classes
//  that implement IUICommandInvokedHandler's Invoke method.
//
//---------------------------------------------------------------------------
class TextContextMenuCommandHandler : public wup::IUICommandInvokedHandler, public wf::IAsyncOperationCompletedHandler<wup::IUICommand*>
{
public:
    TextContextMenuCommandHandler();

    // IUnknown overrides.
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void **ppObject
        );
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

protected:
    virtual ~TextContextMenuCommandHandler();

    static _Check_return_ HRESULT GetCommandId(
        _In_  wup::IUICommand *pCommand,
        _Out_ XINT32 *pId
        );

private:
    XUINT32 m_refCount;
};
