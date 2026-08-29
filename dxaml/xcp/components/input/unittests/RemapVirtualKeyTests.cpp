// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLogging.h>
#include "RemapVirtualKey.h"
#include "RemapVirtualKeyTests.h"

using namespace Microsoft::WRL;
using namespace WEX::TestExecution;
using namespace InputUtility;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Input {

struct Entry { wsy::VirtualKey raw; wsy::VirtualKey mapped; };

static Entry ExpectedMappingTable[] =
{
    { wsy::VirtualKey::VirtualKey_GamepadA, wsy::VirtualKey::VirtualKey_Space },
    { wsy::VirtualKey::VirtualKey_GamepadB, wsy::VirtualKey::VirtualKey_Escape },
    { wsy::VirtualKey::VirtualKey_GamepadDPadUp, wsy::VirtualKey::VirtualKey_Up },
    { wsy::VirtualKey::VirtualKey_GamepadDPadDown, wsy::VirtualKey::VirtualKey_Down },
    { wsy::VirtualKey::VirtualKey_GamepadDPadLeft, wsy::VirtualKey::VirtualKey_Left },
    { wsy::VirtualKey::VirtualKey_GamepadDPadRight, wsy::VirtualKey::VirtualKey_Right },
    { wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickUp, wsy::VirtualKey::VirtualKey_Up },
    { wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickDown, wsy::VirtualKey::VirtualKey_Down },
    { wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickRight, wsy::VirtualKey::VirtualKey_Right },
    { wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickLeft, wsy::VirtualKey::VirtualKey_Left },
};

wsy::VirtualKey GetExpectedKey(wsy::VirtualKey key)
{
    for (unsigned int i = 0; i < _countof(ExpectedMappingTable); ++i)
    {
        if (key == ExpectedMappingTable[i].raw)
        {
            return ExpectedMappingTable[i].mapped;
        }
    }
    return key;
}

void RemapVirtualKeyTests::TestKeyMapping()
{
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    for (unsigned int key = 0; key < 300; ++key)
    {
        const wsy::VirtualKey vKey = static_cast<wsy::VirtualKey>(key);
        VERIFY_ARE_EQUAL(GetExpectedKey(vKey), RemapVirtualKey(vKey));
    }
}


} } } } }
