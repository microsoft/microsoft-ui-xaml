// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wex.common.h>
#include <wextestclass.h>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Settings {

    class XamlOptionalChangesTests
    {
    public:
        BEGIN_TEST_CLASS(XamlOptionalChangesTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanEnableAndQueryBeforeLock)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies EnableChange/DisableChange/IsChangeEnabled work before Lock")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(LockReturnsTrueOnFirstCall)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies Lock() returns true first time, false thereafter")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(EnableChangeFailsAfterLock)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies EnableChange returns E_ILLEGAL_STATE_CHANGE after Lock for any changeId")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DisableChangeFailsAfterLock)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies DisableChange returns E_ILLEGAL_STATE_CHANGE after Lock for any changeId")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(IsChangeEnabledWorksAfterLock)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies IsChangeEnabled still returns correct value after Lock")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnrecognizedValueContract)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies unrecognized values: pre-lock Enable/Disable returns FALSE, post-lock both throw E_ILLEGAL_STATE_CHANGE")
        END_TEST_METHOD()
    };

} } } } } }
