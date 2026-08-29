// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class ShutdownTests : public WEX::TestClass<ShutdownTests>
{
public:
    BEGIN_TEST_CLASS(ShutdownTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        // Disabling on OneCore due to: stale DComp tree associated with window after DComp device is recreated
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"Description", L"Shuts down Xaml.")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;3192b2bd-30c5-4c19-a6c1-9856b940df63;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)

    BEGIN_TEST_METHOD(ShutdownWithOutstandingDOs)
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
        TEST_METHOD_PROPERTY(L"Description", L"Shuts down Xaml while there's still a DO alive. We shouldn't crash.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Fails with empty MockDComp output on Catgates but not locally
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ClosePopupWithTreeReset)
        // We can't recover from resetting the visual tree, so run this test in its own process.
        TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing window background treatment & comp node
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
};

} } } } } }

