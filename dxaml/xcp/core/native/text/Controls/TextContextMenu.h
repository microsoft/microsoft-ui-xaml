// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class TextContextMenuCommandHandler;

class TextContextMenu
{
public:

    // Event handlers
    static _Check_return_ HRESULT OnSpellingSuggestionClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);
    static _Check_return_ HRESULT OnAddToDictionaryClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);
    static _Check_return_ HRESULT OnIgnoreAllClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);
    static _Check_return_ HRESULT OnDeleteRepeatedClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);
    static _Check_return_ HRESULT OnIgnoreOnceClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);
    static _Check_return_ HRESULT OnStopCorrectingClicked(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);

    // RichEdit's PopupMenu
    _Check_return_ static HRESULT Show(
        _In_ CUIElement *pOwnerUIElement,
        _In_ const XPOINTF &point,
        _In_ bool showCut,
        _In_ bool showCopy,
        _In_ bool showPaste,
        _In_ bool showUndo,
        _In_ bool showRedo,
        _In_ bool showSelectAll);

    static const wsy::VirtualKey ContextMenuKeyCode;

    static _Check_return_ HRESULT RaiseContextMenuOpeningEvent(
        _In_ EventHandle hEvent,
        _In_ CUIElement *pSender,
        _In_ XPOINTF point,
        _In_ bool showCut,
        _In_ bool showCopy,
        _In_ bool showPaste,
        _In_ bool showUndo,
        _In_ bool showRedo,
        _In_ bool showSelectAll);

private:
    static const XINT32 menuOptions[];

    static _Check_return_ HRESULT Show(
        _In_ CUIElement *pOwnerUIElement,
        _In_ TextContextMenuCommandHandler *pCommandHandler,
        _In_ const XPOINTF &point,
        _In_ bool showCut,
        _In_ bool showCopy,
        _In_ bool showPaste,
        _In_ bool showUndo,
        _In_ bool showRedo,
        _In_ bool showSelectAll);

    static _Check_return_ HRESULT InvokeProofingMenuItem(_In_ CDependencyObject* const sender, _In_ uint8_t menuOption);
};
