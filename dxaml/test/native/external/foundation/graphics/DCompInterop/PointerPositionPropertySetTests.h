// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class PointerPositionPropertySetTests : public WEX::TestClass<PointerPositionPropertySetTests>
{
public:
    BEGIN_TEST_CLASS(PointerPositionPropertySetTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(Basic)
        TEST_METHOD_PROPERTY(L"Description", L"Call GetPointerPositionPropertySet")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TouchUpdate)
        TEST_METHOD_PROPERTY(L"Description", L"Update the pointer position as the finger moves")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()
};


} } } } } }

