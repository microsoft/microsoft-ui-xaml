// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ComTemplates.h>
#include "microsoft.ui.input.h"
#include <Microsoft.UI.Input.InputPreTranslateSource.Interop.h>

namespace DirectUI
{
    template <typename T>
    class PreTranslateHandler : public ctl::implements<mui::IInputPreTranslateKeyboardSourceHandler>
    {
    public:
        PreTranslateHandler(T* owner) :
            m_ownerNoRef(owner)
        {
        }

        // IInputPreTranslateKeyboardSourcePreTranslateHandler Methods
        
        IFACEMETHODIMP OnDirectMessage(
            mui::IInputPreTranslateKeyboardSourceInterop* source,
            const MSG* msg,
            UINT keyboardModifiers,
            bool* handled) override
        {
            if (m_ownerNoRef == nullptr)
            {
                return S_OK;
            }

            return m_ownerNoRef->PreTranslateMessage(
                source,
                msg,
                keyboardModifiers,
                true,
                handled);
        }

        IFACEMETHODIMP OnTreeMessage(
            mui::IInputPreTranslateKeyboardSourceInterop* source,
            const MSG* msg,
            UINT keyboardModifiers,
            bool* handled) override
        {
            if (m_ownerNoRef == nullptr)
            {
                return S_OK;
            }

            return m_ownerNoRef->PreTranslateMessage(
                source,
                msg,
                keyboardModifiers,
                false,
                handled);
        }

    private:
        T* m_ownerNoRef = nullptr;
    };
}
