// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EventArgs.h"

class CHasValidationErrorsChangedEventArgs : public CEventArgs
{
public:
    CHasValidationErrorsChangedEventArgs(bool newValue);

    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT get_NewValue(_Out_ BOOLEAN* newValue);
    _Check_return_ HRESULT put_NewValue(BOOLEAN newValue);

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** peer) override;
private:

    bool m_newValue;
};
