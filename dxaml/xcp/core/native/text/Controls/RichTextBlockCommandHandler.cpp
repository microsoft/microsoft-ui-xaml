// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockCommandHandler.h"
#include "TextSelectionManager.h"
#include "localizedResource.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
RichTextBlockCommandHandler::RichTextBlockCommandHandler(_In_ CRichTextBlock *pRichTextBlock) :
    m_pRichTextBlock(pRichTextBlock)
{
    AddRefInterface(pRichTextBlock);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
RichTextBlockCommandHandler::~RichTextBlockCommandHandler()
{
    ReleaseInterface(m_pRichTextBlock);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Invoke method to handle the context menu command execution.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT STDMETHODCALLTYPE RichTextBlockCommandHandler::Invoke(_In_opt_ wup::IUICommand *pCommand)
{
    XINT32 id;

    IFC_RETURN(GetCommandId(pCommand, &id));

    switch (id)
    {
        case TEXT_CONTEXT_MENU_SELECT_ALL:
            IFC_RETURN(m_pRichTextBlock->SelectAll());
            m_pRichTextBlock->InvalidateRender();
            break;

        case TEXT_CONTEXT_MENU_COPY:
            if (m_pRichTextBlock->GetSelectionManager() != NULL)
            {
                IFC_RETURN(m_pRichTextBlock->GetSelectionManager()->CopySelectionToClipboard());
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
_Check_return_ HRESULT STDMETHODCALLTYPE RichTextBlockCommandHandler::Invoke(
    _In_ wf::IAsyncOperation<wup::IUICommand*> *pAsyncOperation,
    _In_ wf::AsyncStatus status)
{
    Microsoft::WRL::ComPtr<wup::IUICommand> spInvokedCommand;

    if (m_pRichTextBlock->GetSelectionManager() != NULL)
    {
        IFC_RETURN(m_pRichTextBlock->GetSelectionManager()->OnContextMenuDismiss());
    }

    // SetPopupMenuCommandInvokedEvent is for testing purposed only. Ignore possible errors.
    IGNOREHR(pAsyncOperation->GetResults(&spInvokedCommand));
    if (spInvokedCommand != nullptr)
    {
        IGNOREHR(m_pRichTextBlock->GetContext()->SetPopupMenuCommandInvokedEvent());
    }

    return S_OK;
}
