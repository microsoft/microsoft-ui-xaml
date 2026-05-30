// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ComboBoxLightDismiss.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ComboBoxLightDismiss)
    {
    public:
        HRESULT AutomationClick();

        _Check_return_ HRESULT put_Owner(_In_ xaml_controls::IComboBox* pOwner);
    protected:
        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

    private:
        ctl::WeakRefPtr m_wrComboBox;
    };
}
