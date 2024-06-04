// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextContextMenu.h"
#include "TextBlockCommandHandler.h"
#include "RichTextBlockCommandHandler.h"
#include "TextBoxCommandHandler.h"

#pragma warning(push)
#pragma warning(disable : 4255)
#include <winuser.h>
#pragma warning(pop)

#include <shobjidl_core.h> // IInitializeWithWindow
#include "ContextMenuEventArgs.h"
#include "RootScale.h"

using namespace Microsoft::WRL;

const wsy::VirtualKey TextContextMenu::ContextMenuKeyCode = wsy::VirtualKey::VirtualKey_Application; //  Applications key on a Microsoft Natural Keyboard
const XINT32 TextContextMenu::menuOptions[]      = { TEXT_CONTEXT_MENU_CUT, TEXT_CONTEXT_MENU_COPY, TEXT_CONTEXT_MENU_PASTE, TEXT_CONTEXT_MENU_UNDO, TEXT_CONTEXT_MENU_REDO, TEXT_CONTEXT_MENU_SELECT_ALL };

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Shows the context menu asynchronously.
//
//  Notes:
//      Avoid calling this method -- prefer the overload that takes an
//      explicit TextContextMenuCommandHandler.  This version of Show is a
//      compromise for core types like TextBlock that can't take direct
//      windows dependencies (practically speaking, can't include windows
//      headers in their build and thus can't reference TextContextMenuCommandHandler).
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextContextMenu::Show(
    _In_ CUIElement *pOwnerUIElement,
    _In_ const XPOINTF &point,
    _In_ bool showCut,
    _In_ bool showCopy,
    _In_ bool showPaste,
    _In_ bool showUndo,
    _In_ bool showRedo,
    _In_ bool showSelectAll
    )
{
    HRESULT hr = S_OK;
    TextContextMenuCommandHandler* pCommandHandler = NULL;

    switch (pOwnerUIElement->GetTypeIndex())
    {
    case KnownTypeIndex::TextBlock:
        pCommandHandler = new TextBlockCommandHandler(do_pointer_cast<CTextBlock>(pOwnerUIElement));
        break;

    case KnownTypeIndex::RichTextBlock:
        pCommandHandler = new RichTextBlockCommandHandler(do_pointer_cast<CRichTextBlock>(pOwnerUIElement));
        break;

    case KnownTypeIndex::TextBox:
        pCommandHandler = new TextBoxCommandHandler(do_pointer_cast<CTextBoxBase>(pOwnerUIElement));
        break;

    case KnownTypeIndex::RichEditBox:
    case KnownTypeIndex::PasswordBox:
        pCommandHandler = new TextBoxCommandHandler(do_pointer_cast<CTextBoxBase>(pOwnerUIElement));
        break;

    default:
        ASSERT(FALSE); // Unknown type.
        break;
    }

    IFC(Show(
        pOwnerUIElement,
        pCommandHandler,
        point,
        showCut,
        showCopy,
        showPaste,
        showUndo,
        showRedo,
        showSelectAll));

Cleanup:
    ReleaseInterface(pCommandHandler);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Shows the context menu asynchronously.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextContextMenu::Show(
    _In_ CUIElement *pOwnerUIElement,
    _In_ TextContextMenuCommandHandler *pCommandHandler,
    _In_ const XPOINTF &point,
    _In_ bool showCut,
    _In_ bool showCopy,
    _In_ bool showPaste,
    _In_ bool showUndo,
    _In_ bool showRedo,
    _In_ bool showSelectAll
    )
{
    XHANDLE hwnd;
    xstring_ptr strLocalizedStr;
    ComPtr<wup::IPopupMenu> spPopupMenu;
    ComPtr<wf::IAsyncOperation<wup::IUICommand*>> spAsyncInfo;
    ComPtr<wup::IUICommand> spMenuItem;
    ComPtr<wfc::IVector<wup::IUICommand*>> spCommands;
    ComPtr<IInitializeWithWindow> spInitializeWithWindow;
    ComPtr<wf::IPropertyValueStatics> spValueFactory;
    ComPtr<IInspectable> spId;
    bool showItem[] = { showCut, showCopy, showPaste, showUndo, showRedo, showSelectAll };

    ASSERT(showCut || showCopy || showPaste || showUndo || showRedo || showSelectAll);

    IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_Popups_PopupMenu).Get(), &spPopupMenu));
    spPopupMenu.As<IInitializeWithWindow>(&spInitializeWithWindow); // spInitializeWithWindow is null on mobile

    if (spInitializeWithWindow != nullptr)
    {
        hwnd = pOwnerUIElement->GetContext()->GetHostSite()->GetXcpControlWindow();
        IFC_RETURN(spInitializeWithWindow->Initialize(reinterpret_cast<HWND>(hwnd)));
    }

    IFC_RETURN(spPopupMenu->get_Commands(&spCommands));

    if (spCommands == nullptr) // nullptr return since Popup menu is not supported on IoT device
    {
        return S_OK;
    }

    IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), &spValueFactory));

    for (XINT32 i = 0; i < ARRAY_SIZE(menuOptions); i++)
    {
        if (showItem[i])
        {
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_Popups_UICommand).Get(), /*out*/ spMenuItem.ReleaseAndGetAddressOf()));
            IFC_RETURN(pOwnerUIElement->GetContext()->GetBrowserHost()->GetLocalizedResourceString(menuOptions[i], &strLocalizedStr));

            xruntime_string_ptr strLocalizedRuntimeStr;

            IFC_RETURN(strLocalizedStr.Promote(&strLocalizedRuntimeStr));

            IFC_RETURN(spMenuItem->put_Label(strLocalizedRuntimeStr.GetHSTRING()));

            IFC_RETURN(spMenuItem->put_Invoked(pCommandHandler));

            IFC_RETURN(spValueFactory->CreateInt32(menuOptions[i], spId.ReleaseAndGetAddressOf()));

            IFC_RETURN(spMenuItem->put_Id(spId.Get()));

            IFC_RETURN(spCommands->Append(spMenuItem.Get()));
        }
    }

    wf::Point invocationPoint;
    invocationPoint.X = point.x;
    invocationPoint.Y = point.y;

    IFC_RETURN(spPopupMenu->ShowAsync(invocationPoint, &spAsyncInfo));
    IFC_RETURN(spAsyncInfo->put_Completed(pCommandHandler));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   RaiseContextMenuOpeningEvent
//
//  Synopsis: Raises routed event for ContextMenuOpening.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextContextMenu::RaiseContextMenuOpeningEvent(
    _In_ EventHandle hEvent,
    _In_ CUIElement *pSender,
    _In_ XPOINTF point,
    _In_ bool showCut,
    _In_ bool showCopy,
    _In_ bool showPaste,
    _In_ bool showUndo,
    _In_ bool showRedo,
    _In_ bool showSelectAll)
{
    CEventManager *pEventManager = NULL;
    ASSERT(pSender != NULL);

    // Do not show the CM if there are no menu items to show
    if (!showCut
        && !showCopy
        && !showPaste
        && !showUndo
        && !showRedo
        && !showSelectAll)
    {
        return S_OK;
    }

    auto core = pSender->GetContext();
    IFCEXPECT_ASSERT_RETURN(core);

    pEventManager = core->GetEventManager();
    if (pEventManager)
    {
        // Convert to DIPS by adjusting for plateau scale
        const float zoomScale = RootScale::GetRasterizationScaleForElement(pSender);
        point /= zoomScale;

        // Create the DO that represents the event args.
        xref_ptr<CContextMenuEventArgs> pArgs;
        pArgs.attach(new CContextMenuEventArgs());

        pArgs->m_cursorLeft    = point.x;
        pArgs->m_cursorTop     = point.y;
        pArgs->m_pUIElement    = pSender;
        pArgs->m_showCut       = showCut;
        pArgs->m_showCopy      = showCopy;
        pArgs->m_showPaste     = showPaste;
        pArgs->m_showUndo      = showUndo;
        pArgs->m_showRedo      = showRedo;
        pArgs->m_showSelectAll = showSelectAll;

        // Raise event async.
        pEventManager->RaiseRoutedEvent(hEvent, pSender, pArgs);
    }

    return S_OK;
}

_Check_return_ HRESULT TextContextMenu::InvokeProofingMenuItem(_In_ CDependencyObject* const sender, _In_ uint8_t menuOption)
{
    auto contactPopup = CPopup::GetClosestPopupAncestor(do_pointer_cast<CUIElement>(sender));
    auto flyout = contactPopup ? contactPopup->GetAssociatedFlyoutNoRef() : nullptr;
    auto textBoxBase = flyout ? do_pointer_cast<CTextBoxBase>(flyout->GetParent()) : nullptr;

    FAIL_FAST_ASSERT(textBoxBase);

    // The following conditions must be true or we no-op:
    // 1. Spell check is enabled
    // 2. Selection has not changed since menu creation
    // 3. Text box is enabled
    if (!(textBoxBase->m_isSpellCheckEnabled
        && textBoxBase->IsProofingMenuValid()
        && textBoxBase->IsEnabled()))
    {
        return S_OK;
    }

    if (menuOption == SCI_REPLACE)
    {
        CValue value;
        IFC_RETURN(sender->GetValueByIndex(KnownPropertyIndex::MenuFlyoutItem_Text, &value));

        xstring_ptr suggestion;
        value.GetString(suggestion);

        // First WPARAM is supposed to be cp, but it is not used.
        IFC_RETURN(textBoxBase->GetTextServices()->TxSendMessage(
            EM_SPELLCHECKINVOKE,
            MAKEWPARAM(-1, SCI_REPLACE),
            reinterpret_cast<LPARAM>(suggestion.GetBuffer()),
            nullptr));
    }
    else
    {
        // First WPARAM is supposed to be cp, but it is not used.
        IFC_RETURN(textBoxBase->GetTextServices()->TxSendMessage(EM_SPELLCHECKINVOKE, MAKEWPARAM(-1, menuOption), 0, nullptr));
    }

    return S_OK;
}

_Check_return_ HRESULT TextContextMenu::OnSpellingSuggestionClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    IFC_RETURN(InvokeProofingMenuItem(sender, SCI_REPLACE));
    return S_OK;
}

_Check_return_ HRESULT TextContextMenu::OnAddToDictionaryClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    IFC_RETURN(InvokeProofingMenuItem(sender, SCI_ADDTODICTIONARY));
    return S_OK;
}

_Check_return_ HRESULT TextContextMenu::OnIgnoreAllClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    IFC_RETURN(InvokeProofingMenuItem(sender, SCI_IGNORE));
    return S_OK;
}

_Check_return_ HRESULT TextContextMenu::OnDeleteRepeatedClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    IFC_RETURN(InvokeProofingMenuItem(sender, SCI_DELETEREPEATED));
    return S_OK;
}

_Check_return_ HRESULT TextContextMenu::OnIgnoreOnceClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    IFC_RETURN(InvokeProofingMenuItem(sender, SCI_IGNOREONCE));
    return S_OK;
}

_Check_return_ HRESULT TextContextMenu::OnStopCorrectingClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs)
{
    IFC_RETURN(InvokeProofingMenuItem(sender, SCI_STOPCORRECTING));
    return S_OK;
}
