// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CInputManager;

namespace ContentRootInput
{
    class ActivationManager
    {
    public:
        ActivationManager(_In_ CInputManager& inputManager);

        bool IsActive() const;

        _Check_return_ HRESULT ProcessWindowActivation(
            _In_ bool shiftPressed,
            _In_ bool fWndActive);

        _Check_return_ HRESULT ProcessFocusInput(
            _In_ InputMessage *pMsg,
            _Out_ XINT32 *handled);

    private:
        CInputManager& m_inputManager;

        // Default this to true just in case we never receive XCP_ACTIVATE
        bool m_fWindowActive = true;

        // We want to handle the first XCP_ACTIVATE even if m_fWindowActive is true.
        bool m_fWasWindowActive = false;

        // Default this to true just in case we never receive XCP_GOTFOCUS
        bool m_fWindowFocused = true;

        // We want to handle the first XCP_GOTFOCUS even if m_fWindowFocused is true.
        bool m_fWasWindowFocused = false;
    };
};