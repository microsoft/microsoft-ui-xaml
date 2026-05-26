// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class PointAnimationTests : public WEX::TestClass<PointAnimationTests>
{
public:
    BEGIN_TEST_CLASS(PointAnimationTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"9882729e-eea8-4b89-99e7-92145be50e76;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(PointAnimationBasic)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PauseSeekResume)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(NegativeTests)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
};

} } } } } }

