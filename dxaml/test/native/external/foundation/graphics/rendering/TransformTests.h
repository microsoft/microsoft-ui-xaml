// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class TransformTests : public WEX::TestClass<TransformTests>
{
public:
    BEGIN_TEST_CLASS(TransformTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;3192b2bd-30c5-4c19-a6c1-9856b940df63;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(TransformGroupDirtyFlags)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CompositeTransformDirtyFlagsWUC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;
};

} } } } } }

