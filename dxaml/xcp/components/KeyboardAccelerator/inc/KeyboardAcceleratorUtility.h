// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/Microsoft.UI.Xaml.input.h>

namespace KeyboardAcceleratorUtility {
    bool IsKeyValidForAccelerators(_In_ const wsy::VirtualKey originalKey, _In_ const XUINT32 modifierKeys);
    bool TextInputHasPriorityForKey(_In_ const wsy::VirtualKey originalKey, bool isCtrlPressed, bool isAltPressed);
    const XUINT32 MapVirtualKeyModifiersToIntegersModifiers(_In_ const wsy::VirtualKeyModifiers virtualKeyModifiers);


    // Checks whether a specific keyboard accelerator should be invoked - that it is
    // enabled and matches the key and modifiers that are being processed.
    bool ShouldRaiseAcceleratorEvent(
        _In_ wsy::VirtualKey originalKey,
        _In_ wsy::VirtualKeyModifiers keyModifiers,
        _In_ const CKeyboardAccelerator* const accelerator,
        _In_ CDependencyObject* const pParent);

    _Check_return_ HRESULT ProcessKeyboardAccelerators(
        _In_  const wsy::VirtualKey originalKey,
        _In_  const wsy::VirtualKeyModifiers keyModifiers,
        _In_  VectorOfKACollectionAndRefCountPair& allLiveAccelerators,
        _In_  CDependencyObject* const pElement,
        _Out_ BOOLEAN* pHandled,
        _Out_ BOOLEAN* pHandledShouldNotImpedeTextInput,
        _In_ const CDependencyObject* const pFocusedElement,
        _In_ bool isCallFromTryInvoke);

    // Processes the collection of accelerators defined directly on a specific element
    bool ProcessLocalAccelerators(
        _In_ wsy::VirtualKey originalKey,
        _In_ wsy::VirtualKeyModifiers keyModifiers,
        _In_ CDependencyObject* const pElement,
        _In_ const CDependencyObject* const pFocusedElement,
        _In_ bool isCallFromTryInvoke);

    // Processes the accelerators 'owned by' an element (those whith ScopeOwner == <that element>)
    bool ProcessOwnedAccelerators(
         _In_ wsy::VirtualKey originalKey,
         _In_ wsy::VirtualKeyModifiers keyModifiers,
         _In_ VectorOfKACollectionAndRefCountPair& allLiveAccelerators,
         _In_ CDependencyObject* const pElement,
         _In_ const CDependencyObject* const pFocusedElement,
         _In_ bool isCallFromTryInvoke);

    // Processes the accelerators that are global (ScopeOwner == null)
    bool ProcessGlobalAccelerators(
        _In_ wsy::VirtualKey originalKey,
        _In_ wsy::VirtualKeyModifiers keyModifiers,
        _In_ VectorOfKACollectionAndRefCountPair& allLiveAccelerators);
    
}