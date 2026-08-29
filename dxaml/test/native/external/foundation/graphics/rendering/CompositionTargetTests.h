// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class CompositionTargetTests : public WEX::TestClass<CompositionTargetTests>
{
public:
    BEGIN_TEST_CLASS(CompositionTargetTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(RenderingEvent1)
        TEST_METHOD_PROPERTY(L"Description", L"Basic CompositionTarget.Rendering event tests, variation #1")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderingEvent2)
        TEST_METHOD_PROPERTY(L"Description", L"Basic CompositionTarget.Rendering event tests, variation #2")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderingEvent3)
        TEST_METHOD_PROPERTY(L"Description", L"Basic CompositionTarget.Rendering event tests, variation #3")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderedEventShouldNotRequestFrame)
        TEST_METHOD_PROPERTY(L"Description", L"Verify registering for CompositionTarget.Rendered event will not force rendering a frame.")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Passes first iteration but fails the rest - completedEvent was set and shouldn't be
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderedEvent1)
        TEST_METHOD_PROPERTY(L"Description", L"Basic CompositionTarget.Rendered event tests, variation #1")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderedEvent2)
        TEST_METHOD_PROPERTY(L"Description", L"Basic CompositionTarget.Rendered event tests, variation #2")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderedEvent3)
        TEST_METHOD_PROPERTY(L"Description", L"Basic CompositionTarget.Rendered event tests, variation #3")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderingDoesNotStarveInput)
    END_TEST_METHOD()
};

} } } } } }
