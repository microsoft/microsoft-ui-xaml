// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wex.common.h>
#include <wextestclass.h>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Lifetime {

    class LoadUnloadTests
    {
    public:
        BEGIN_TEST_CLASS(LoadUnloadTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)

        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)
        
        BEGIN_TEST_METHOD(VerifyEventOrdering)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Loading, Loaded and Unloaded event ordering")
        END_TEST_METHOD()

        // Below two tests on Popup is to verify some special behavior for Loaded/Unloaded event on Popup, since
        // popup gets special treatment in XAML, for example it does not remove event listener when going
        // out of tree. Any change in Loaded/Unloaded event should be careful and does not break existing 
        // assumption, otherwise it will introduce app compat risks.

        BEGIN_TEST_METHOD(VerifyEventOrderingForPopup)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Loading, Loaded and Unloaded event ordering for Popup")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyEventOrderingForPopupAddAndRemove)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Loading, Loaded and Unloaded event ordering for Popup")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ReproWeakRefCrash)
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method") // The main validation for this test is that it doesn't crash.
        END_TEST_METHOD()
    };
} } } } } }
