// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextContextMenuCommandHandler.h"
#include <fwd/windows.ui.popups.h>

class CTextBlock;

//---------------------------------------------------------------------------
//
//  Callback used to dispatch context menu selections on CTextBlock.
//
//---------------------------------------------------------------------------
class TextBlockCommandHandler : public TextContextMenuCommandHandler
{
public:
    TextBlockCommandHandler(_In_ CTextBlock *pTextBlock);
    virtual ~TextBlockCommandHandler();
    virtual _Check_return_ HRESULT STDMETHODCALLTYPE Invoke(_In_opt_ wup::IUICommand *pCommand);
    virtual _Check_return_ HRESULT STDMETHODCALLTYPE Invoke(
        _In_ wf::IAsyncOperation<wup::IUICommand*> *pAsyncOperation,
        _In_ wf::AsyncStatus status);

private:
    CTextBlock *m_pTextBlock;
};