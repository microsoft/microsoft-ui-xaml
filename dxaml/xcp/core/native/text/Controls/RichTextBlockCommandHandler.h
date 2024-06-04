// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextContextMenuCommandHandler.h"
#include <fwd/windows.ui.popups.h>

class CRichTextBlock;

//---------------------------------------------------------------------------
//
//  Callback used to dispatch context menu selections on CRichTextBlock.
//
//---------------------------------------------------------------------------
class RichTextBlockCommandHandler : public TextContextMenuCommandHandler
{
public:
    RichTextBlockCommandHandler(_In_ CRichTextBlock *pRichTextBlock);
    virtual ~RichTextBlockCommandHandler();
    virtual _Check_return_ HRESULT STDMETHODCALLTYPE Invoke(_In_opt_ wup::IUICommand *pCommand);
    virtual _Check_return_ HRESULT STDMETHODCALLTYPE Invoke(
        _In_ wf::IAsyncOperation<wup::IUICommand*> *pAsyncOperation,
        _In_ wf::AsyncStatus status);

private:
    CRichTextBlock *m_pRichTextBlock;
};