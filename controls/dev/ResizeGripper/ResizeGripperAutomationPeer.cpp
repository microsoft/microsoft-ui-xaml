// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResizeGripperAutomationPeer.h"
#include "ResourceAccessor.h"
#include "Utils.h"

#include <cwctype>
#include <string>

#include "ResizeGripperAutomationPeer.properties.cpp"

namespace
{
    // Read from the declared OwnerName property, never from Tag and never from an ancestor: Tag is a
    // general-purpose slot a host may already be using for its own bookkeeping, and a primitive that
    // walks up into whatever tree it happens to sit in would publish whatever it found to assistive
    // technology. A silent wrong name is an accessibility bug.
    winrt::hstring GetOwningHeaderText(winrt::ResizeGripper const& gripper)
    {
        return gripper.OwnerName();
    }
}

ResizeGripperAutomationPeer::ResizeGripperAutomationPeer(winrt::ResizeGripper const& owner) :
    ReferenceTracker(owner)
{
}

hstring ResizeGripperAutomationPeer::GetClassNameCore()
{
    // Fixed string rather than hstring_name_of<>: the type lives in Microsoft.UI.Private.Controls,
    // and an internal namespace should not surface to assistive technology.
    return L"ResizeGripper";
}

hstring ResizeGripperAutomationPeer::GetNameCore()
{
    // An explicit name wins outright; composing it with a Tag qualifier would corrupt it.
    if (auto const explicitName = __super::GetNameCore(); !explicitName.empty())
    {
        return explicitName;
    }

    // ResourceMap::GetValue throws when the control's PRI is not merged into the host app, and
    // nothing may escape a UIA callback.
    auto const localized = [](auto const& resourceId, winrt::hstring const& fallback)
    {
        try
        {
            if (auto const value = ResourceAccessor::GetLocalizedStringResource(resourceId); !value.empty())
            {
                return value;
            }
        }
        catch (...) {}
        return fallback;
    };

    const auto name = localized(SR_ResizeGripperName, winrt::hstring{ L"Resize gripper" });

    if (auto const gripper = Owner().try_as<winrt::ResizeGripper>())
    {
        const auto headerName = GetOwningHeaderText(gripper);
        if (!headerName.empty())
        {
            const auto nameFormat = localized(SR_ResizeGripperNameFormat, winrt::hstring{ L"%1!s!, %2!s!" });
            if (auto const formatted = StringUtil::FormatString(nameFormat, headerName.c_str(), name.c_str());
                !formatted.empty())
            {
                return formatted;
            }
            // A mistranslated format string yields nothing; the bare name still identifies the control.
        }
    }

    return name;
}

winrt::hstring ResizeGripperAutomationPeer::GetAutomationIdCore()
{
    if (auto const automationId = __super::GetAutomationIdCore(); !automationId.empty())
    {
        return automationId;
    }

    // Empty rather than a constant: AutomationId must be unique among siblings, and a host
    // stamping one gripper per column would otherwise publish the same id on every one.
    return {};
}

winrt::AutomationControlType ResizeGripperAutomationPeer::GetAutomationControlTypeCore()
{
    // Not a Slider: the gripper reports drag distance and owns no value or range. The column
    // header is the focusable element that represents the resizable thing.
    return winrt::AutomationControlType::Thumb;
}
