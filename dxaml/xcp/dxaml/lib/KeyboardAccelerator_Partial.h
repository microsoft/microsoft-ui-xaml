// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "KeyboardAccelerator.g.h"

namespace DirectUI
{

    PARTIAL_CLASS(KeyboardAccelerator)
    {
    public:
        static _Check_return_ HRESULT RaiseKeyboardAcceleratorInvoked(
            _In_ CKeyboardAccelerator* pNativeAccelerator,
            _In_ CDependencyObject* pElement,
            _Out_ BOOLEAN *pIsHandled);

        static _Check_return_ HRESULT SetToolTip(
            _In_ CKeyboardAccelerator* pNativeAccelerator,
            _In_ CDependencyObject* pParentElement);

        static _Check_return_ HRESULT GetStringRepresentationForUIElement(_In_ DirectUI::UIElement* uiElement, _Outptr_result_maybenull_ HSTRING *stringRepresentation);
        _Check_return_ HRESULT GetStringRepresentation(_Out_ HSTRING *stringRepresentation);

    protected:
        ~KeyboardAccelerator() override {}

    private:
        _Check_return_ HRESULT ConcatVirtualKey(_In_ wsy::VirtualKey key, _Inout_ wrl_wrappers::HString& keyboardAcceleratorString);
    };
}
