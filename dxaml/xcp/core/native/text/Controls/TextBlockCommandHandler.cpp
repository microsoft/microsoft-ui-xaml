// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBlockCommandHandler.h"
#include "TextSelectionManager.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
TextBlockCommandHandler::TextBlockCommandHandler(_In_ CTextBlock *pTextBlock) :
    m_pTextBlock(pTextBlock)
{
    AddRefInterface(pTextBlock);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
TextBlockCommandHandler::~TextBlockCommandHandler()
{
    ReleaseInterface(m_pTextBlock);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Invoke method to handle the context menu command execution.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT STDMETHODCALLTYPE TextBlockCommandHandler::Invoke(_In_opt_ wup::IUICommand *pCommand)
{
    XINT32 id;

    IFC_RETURN(GetCommandId(pCommand, &id));

    switch (id)
    {
        case TEXT_CONTEXT_MENU_SELECT_ALL:
            IFC_RETURN(m_pTextBlock->SelectAll());
            m_pTextBlock->InvalidateRender();
            break;

        case TEXT_CONTEXT_MENU_COPY:
            if (m_pTextBlock->GetSelectionManager() != NULL)
            {
                IFC_RETURN(m_pTextBlock->GetSelectionManager()->CopySelectionToClipboard());
            }
            break;

        default:
            ASSERT(FALSE); // Unexpected command id.
            break;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Invoke method to handle dismissing the context menu.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT STDMETHODCALLTYPE TextBlockCommandHandler::Invoke(
    _In_ wf::IAsyncOperation<wup::IUICommand*> *pAsyncOperation,
    _In_ wf::AsyncStatus status)
{
    Microsoft::WRL::ComPtr<wup::IUICommand> spInvokedCommand;

    if (m_pTextBlock->GetSelectionManager() != NULL)
    {
        IFC_RETURN(m_pTextBlock->GetSelectionManager()->OnContextMenuDismiss());
    }

    // SetPopupMenuCommandInvokedEvent is for testing purposed only. Ignore possible errors.
    IGNOREHR(pAsyncOperation->GetResults(&spInvokedCommand));
    if (spInvokedCommand != nullptr)
    {
        IGNOREHR(m_pTextBlock->GetContext()->SetPopupMenuCommandInvokedEvent());
    }

    return S_OK;
}
