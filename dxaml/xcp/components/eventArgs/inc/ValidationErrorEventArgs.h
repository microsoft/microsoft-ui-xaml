// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EventArgs.h"
#include "EnumDefs.g.h"

enum class KnownTypeIndex : UINT16;

class CInputValidationErrorEventArgs : public CEventArgs
{
public:
    CInputValidationErrorEventArgs(
        const DirectUI::InputValidationErrorEventAction action,
        _In_ xaml_controls::IInputValidationError* error);

    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT get_Action(_Out_ DirectUI::InputValidationErrorEventAction* action);
    _Check_return_ HRESULT put_Action(_In_ DirectUI::InputValidationErrorEventAction action);
    _Check_return_ HRESULT get_Error(_COM_Outptr_ IUnknown** error);
    _Check_return_ HRESULT put_Error(_In_ IUnknown* error);

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** peer) override;

    DirectUI::InputValidationErrorEventAction m_errorAction;
    Microsoft::WRL::ComPtr<xaml_controls::IInputValidationError> m_error;
};
