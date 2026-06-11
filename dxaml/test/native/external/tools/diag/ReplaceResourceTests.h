// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <memory>
#include <map>
#include <tuple>
#include <wrl\module.h>
#include <wil\resource.h>
#include "XamlOM.WinUI.h"
#include <TestCleanupWrapper.h>
#include "XamlDiagnosticsHelper.h"
#include "XamlDiagnosticsTestBase.h"

using namespace Microsoft::UI::Xaml::Controls;
namespace wrl = Microsoft::WRL;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {
        class ReplaceResourceTests : public BaseTestClass<ReplaceResourceTests>
        {
        public:
            BEGIN_TEST_CLASS(ReplaceResourceTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
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

            BEGIN_TEST_METHOD(TestReplaceResourceBasic)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ReplaceResource works when a built-in Xaml object/property has references a built-in resource in a dictionary.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReplaceResourceInDictionary)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ReplaceResource works when the static resource being replaced is referenced within the same dictionary.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReplaceResourceCustomObjectProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ReplaceResource works on a custom user control with a custom property referencing a StaticResource")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReplaceResourceOptimizedSetterDependency)
                TEST_METHOD_PROPERTY(L"Description", L"Validates ReplaceResource works on a setter which uses a StaticResource in an optimized style")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReplaceResourceStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that ReplaceResource works on a Style in a ResourceDictionary, and objects using the style are updated")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestUnregisterStaticResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that after changing a property which referenced a StaticResource to a local property, and then changing the old StaticResource's value, the property still refers to the locally set value")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReplaceResourceApplicationImplicit)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies replacing a resource in Application.Resources when a ResourceDictionary tag isn't specified works correctly")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestReplaceResourceMergedDictionaries)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies replacing a resource in a merged dictionary works correctly")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceStaticResourceInThemeDictionary)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that someone having a StaticResource reference to an object in a theme dictionary can be replaced")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace a theme resource and still get theme changess")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceNonActiveThemeResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that property doesn't change when replacing non-active theme")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceLanguagePrimitives)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace language primitives")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceLanguagePrimitivesInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace language primitives in a control template")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceLanguagePrimitivesInControlTemplateSetterValue)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace language primitives in a control template that is in a setter")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceLanguagePrimitivesInControlTemplateSetterValue_Optimized)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace language primitives in a control template that is in a setter")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceBrushInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace a DO in a control template")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReplaceThemeResourceSetToSetterValue)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplaceResourceUpdatesFallbackValue)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplaceResourceUpdatesTargetNullValue)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplaceResourceOfStyleDoesntAffectResourceResolution)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplacingValueTypesDoesntCrash)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
             END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplacingValueTypesDoesntCrashImplicitStyle)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
             END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplaceInvalidResourceClearsErrors)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplaceNonDP)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace a StaticResource referenced by StaticResources present at compile time or added at runtime for non-dependency properties")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyReplaceResourceUpdatesItemInDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we can replace a StaticResource referenced inside a data template and things properly update")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
        private:
            //Helper method for creating a Style
            void TestReplaceThemeResource(InstanceHandle rootElement, const std::wstring& themeDictionaryKey);
            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper LoadXamlFromFunction(const std::function<UIElement ^ ()> func, wrl::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback);
            
            struct ReplacePrimitiveData
            {
                std::wstring Type;
                std::wstring Key;
                std::wstring Property;
                std::wstring BeforeValue;
                std::wstring AfterValue;
            };

            void TestReplaceLanguagePrimitives(Platform::String^ markup);
            void TestReplaceLanguagePrimitives(::Windows::Foundation::Uri^ componentLocation);
            void ReplaceAllLanguagePrimitives(const wrl::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback);
            void ReplaceLanguagePrimitive(const wrl::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback, const ReplacePrimitiveData& replaceData);
        };
    }
} } } } }
