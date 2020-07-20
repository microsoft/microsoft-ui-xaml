// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NumberBoxTextBox.h"
#include "NumberBoxAutomationPeer.h"
#include "ResourceAccessor.h"

NumberBoxTextBox::NumberBoxTextBox()
{
    SetDefaultStyleKey(this);
}

winrt::AutomationPeer NumberBoxTextBox::OnCreateAutomationPeer()
{
    return winrt::make<NumberBoxAutomationPeer>(*this);
}
