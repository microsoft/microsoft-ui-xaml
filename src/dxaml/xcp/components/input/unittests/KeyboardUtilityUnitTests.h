// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Input { namespace Keyboard {

class KeyboardUtilityUnitTests : public WEX::TestClass < KeyboardUtilityUnitTests >
{
public:
    BEGIN_TEST_CLASS(KeyboardUtilityUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(VerifyEditKeyMappings)
    TEST_METHOD(VerifyCorrectPackingOfKeyEventArgs)
    TEST_METHOD(VerifyDManipForwarding)

private:
    UINT32 LParamPack(
        _In_ int repeatCount,
        _In_ int scanCode,
        _In_ bool isExtendedKey,
        _In_ bool isMenuKeyDown,
        _In_ bool wasKeyDown,
        _In_ bool isKeyReleased,
        _In_ bool isSysKey);
};

class MockKeyEventArgs : public wrl::RuntimeClass<mui::IKeyEventArgs>
{
public:
    MockKeyEventArgs(wsy::VirtualKey key, mui::PhysicalKeyStatus keyStatus);

    IFACEMETHOD(get_Handled)(_Out_ boolean* pHandled) { *pHandled = true; return S_OK; }
    IFACEMETHOD(put_Handled)(boolean) { return S_OK; }
    IFACEMETHOD(get_KeyStatus)(_Out_ mui::PhysicalKeyStatus* keyStatus);
    IFACEMETHOD(get_Timestamp)(_Out_ UINT64* pTimeStamp) { *pTimeStamp = 0; return S_OK; }
    IFACEMETHOD(get_VirtualKey)(_Out_ wsy::VirtualKey* key);

private:
    wsy::VirtualKey m_virtualKey;
    mui::PhysicalKeyStatus m_keyStatus;
};

} } } } } }
