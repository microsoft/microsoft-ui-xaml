// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <memory>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace LayoutCausality { 
    class PropertyChangedEventTests : public WEX::TestClass<PropertyChangedEventTests>
    {
    public:
        BEGIN_TEST_CLASS(PropertyChangedEventTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(UpdateAffectsLayoutOnly)
            TEST_METHOD_PROPERTY(L"Description",
                L"Ensure update that only affects layout reports correctly")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UpdateAffectsRenderOnly)
            TEST_METHOD_PROPERTY(L"Description",
                L"Ensure update that only affects render reports correctly")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UpdateDoesntAffectLayout)
            TEST_METHOD_PROPERTY(L"Description",
                L"Ensure we don't fire events that don't affect layout")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DirtyElementFiresEvent)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verify that event if the element is already dirty that we still fire the event that affects layout")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

    };
} } } } } } } 
