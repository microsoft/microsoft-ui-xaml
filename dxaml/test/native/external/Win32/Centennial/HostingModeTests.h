// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <HostingModeTestClass.h>

namespace Microsoft::UI::Xaml::Tests::HostingMode {

class HostingModeDefaultTests : public WEX::TestClass<HostingModeDefaultTests>
{
public:
    BEGIN_TEST_CLASS(HostingModeDefaultTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_CLASS_HOSTING_MODE_DEFAULT()
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(CreatesDeclaredHostWithoutParameter)
};

class HostingModeUapTests : public WEX::TestClass<HostingModeUapTests>
{
public:
    BEGIN_TEST_CLASS(HostingModeUapTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_CLASS_HOSTING_MODE(UAP)
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(CreatesDeclaredHostWithoutParameter)
};

class HostingModeWin32ExplicitTests : public WEX::TestClass<HostingModeWin32ExplicitTests>
{
public:
    BEGIN_TEST_CLASS(HostingModeWin32ExplicitTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_CLASS_HOSTING_MODE(Win32Explicit)
    END_TEST_CLASS()

    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    TEST_METHOD(CreatesDeclaredHostWithoutParameter)

private:
    ATOM m_windowClassAtom = 0;
    HWND m_window = nullptr;
};

}
