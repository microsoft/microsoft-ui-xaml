// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Stowed {

class StowedExceptionTests : public WEX::TestClass<StowedExceptionTests>
{
public:
    BEGIN_TEST_CLASS(StowedExceptionTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(VerifyFailFastOnErrors)
        TEST_METHOD_PROPERTY(L"Description", L"Verify FailFastOnErrors works correctly.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyMultipleStowedExceptions)
        TEST_METHOD_PROPERTY(L"Description", L"Verify multiple stowed exceptions get stowed and passed to RoFailFast.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NoFailFastOnErrorsInDesignMode)
        TEST_METHOD_PROPERTY(L"Description", L"Verify FailFastOnErrors is ignored in Design mode.")
        TEST_METHOD_PROPERTY(L"UAP:AppXManifest", L"AppxManifest.DesignMode.xml")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // No design mode currently
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ErrorInfoCreateRestrictedErrorInfo)
        TEST_METHOD_PROPERTY(L"Description", L"Verify ErrorInfo::CreateRestrictedErrorInfo uses an existing stowed exception when possible.")
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // The failfast leaves the UI thread in a broken state, so must terminate the process after this test.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReportUnhandledErrorNonRestrictedError)
        TEST_METHOD_PROPERTY(L"Description", L"Verify ReportUnhandledError works even when the thread error is not a RestrictedError.")
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // The failfast leaves the UI thread in a broken state, so must terminate the process after this test.
    END_TEST_METHOD()
};

} } } } } }
