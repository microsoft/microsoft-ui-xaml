// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlDiagnosticsTestBase.h"

using namespace Microsoft::UI::Xaml::Controls;
namespace wrl = Microsoft::WRL;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {
        class SourceInfoTests : public BaseTestClass<SourceInfoTests>
        {
        public:
            BEGIN_TEST_CLASS(SourceInfoTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)


            BEGIN_TEST_METHOD(TestSourceInfo)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully call into XamlDiagnostics and get source info.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // issue with adding Canvas to island and Enter does not gets called
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSourceInfoOnEmptyElements)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get source info on empty elements.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSourceInfoOnGridWithBrush)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get source info on a grid and a button with a style.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSourceInfoOnCustomWriters)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get source info on our custom writers (ResourceDictionary, Style, VSM).")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSourceInfoOnInlineStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get source info on an inline style.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSourceInfoOnNestedStyles)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get source info with nested styles.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSourceInfoOnBasedOnStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get source info on a style based on another style.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSourceInfoOnRootOfXamlDoc)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the source information on the root object of a XAML document is correctly linked.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyImplicitResourceDictionaryHasSourceInfo)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we have source info on a ResourceDictionary that is not explictily defined")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifySourceInfoDesignModeV2)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"UAP:AppXManifest", L"AppxManifest.DesignMode.xml")
            END_TEST_METHOD()

        };
    }
} } } } }
