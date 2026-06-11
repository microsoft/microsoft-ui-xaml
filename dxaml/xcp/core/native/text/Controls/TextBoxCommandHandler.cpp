// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxCommandHandler.h"
#include "localizedResource.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
TextBoxCommandHandler::TextBoxCommandHandler(_In_ CTextBoxBase *pTextBox) :
    m_pTextBox(pTextBox)
{
    AddRefInterface(pTextBox);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
TextBoxCommandHandler::~TextBoxCommandHandler()
{
    ReleaseInterface(m_pTextBox);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Invoke method to handle the context menu command execution.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT STDMETHODCALLTYPE TextBoxCommandHandler::Invoke(_In_opt_ wup::IUICommand *pCommand)
{
    XINT32 id;

    IFC_RETURN(GetCommandId(pCommand, &id));

    switch (id)
    {
        case TEXT_CONTEXT_MENU_CUT:
            IFC_RETURN(m_pTextBox->Cut());
            break;

        case TEXT_CONTEXT_MENU_COPY:
            IFC_RETURN(m_pTextBox->Copy());
            break;

        case TEXT_CONTEXT_MENU_PASTE:
            IFC_RETURN(m_pTextBox->Paste());
            break;

        case TEXT_CONTEXT_MENU_UNDO:
            IFC_RETURN(m_pTextBox->Undo());
            break;

        case TEXT_CONTEXT_MENU_REDO:
            IFC_RETURN(m_pTextBox->Redo());
            break;

        case TEXT_CONTEXT_MENU_SELECT_ALL:
            IFC_RETURN(m_pTextBox->SelectAll());
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
_Check_return_ HRESULT STDMETHODCALLTYPE TextBoxCommandHandler::Invoke(
        _In_ wf::IAsyncOperation<wup::IUICommand*> *pAsyncOperation,
        _In_ wf::AsyncStatus status)
{
    Microsoft::WRL::ComPtr<wup::IUICommand> spInvokedCommand;

    if (m_pTextBox->GetView())
    {
        IFC_RETURN(m_pTextBox->GetView()->OnContextMenuDismiss());
    }

    // SetPopupMenuCommandInvokedEvent is for testing purposed only. Ignore possible errors.
    IGNOREHR(pAsyncOperation->GetResults(&spInvokedCommand));
    if (spInvokedCommand != nullptr)
    {
        IGNOREHR(m_pTextBox->GetContext()->SetPopupMenuCommandInvokedEvent());
    }

    return S_OK;
}
