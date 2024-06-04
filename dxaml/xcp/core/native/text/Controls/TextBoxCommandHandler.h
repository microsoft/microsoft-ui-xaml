// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextContextMenuCommandHandler.h"
#include <fwd/windows.ui.popups.h>

class CTextBoxBase;

//---------------------------------------------------------------------------
//
//  Callback used to dispatch context menu selections on CTextBoxBase.
//
//---------------------------------------------------------------------------
class TextBoxCommandHandler : public TextContextMenuCommandHandler
{
public:
    TextBoxCommandHandler(_In_ CTextBoxBase *pTextBox);
    virtual ~TextBoxCommandHandler();
    virtual _Check_return_ HRESULT STDMETHODCALLTYPE Invoke(_In_opt_ wup::IUICommand *pCommand);
    virtual _Check_return_ HRESULT STDMETHODCALLTYPE Invoke(
        _In_ wf::IAsyncOperation<wup::IUICommand*> *pAsyncOperation,
        _In_ wf::AsyncStatus status);

private:
    CTextBoxBase *m_pTextBox;
};