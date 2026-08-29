// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace Pointer {

class ReplayPointerUpdateTests : public WEX::TestClass<ReplayPointerUpdateTests>
{
public:
    BEGIN_TEST_CLASS(ReplayPointerUpdateTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(ReplayPointerUpdate)
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()


    BEGIN_TEST_METHOD(AutoPointerUpdateReplay)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD();

    BEGIN_TEST_METHOD(AfterKeyboardInput)
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD();

private:
    inline Platform::String^ GetResourcesPath() const;
};

} } } } } } }
