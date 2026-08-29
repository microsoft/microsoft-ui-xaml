// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RegKeyHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class VisualElementTests : public WEX::TestClass<VisualElementTests>
{
public:
    BEGIN_TEST_CLASS(VisualElementTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_CLASS_CLEANUP(ClassCleanup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(BasicTest)
        TEST_METHOD_PROPERTY(L"Description", L"Creates a VisualInteractionSource from a UIElement")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(InteractionTrackerTest)
        TEST_METHOD_PROPERTY(L"Description", L"Exercises basic InteractionTracker scenario with UIElement as the source")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
    END_TEST_METHOD()
};


} } } } } }
