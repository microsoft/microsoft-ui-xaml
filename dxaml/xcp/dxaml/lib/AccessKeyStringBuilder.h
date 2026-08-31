// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FrameworkElementAutomationPeer.g.h"
#include "TextElement.g.h"
#include "DependencyObject.h"

namespace AccessKeyStringBuilder
{
    _Check_return_ HRESULT GetAccessKeyMessageFromElement(_In_ ctl::ComPtr <DirectUI::DependencyObject>& spOwner, _Out_ HSTRING *returnValue);
}

