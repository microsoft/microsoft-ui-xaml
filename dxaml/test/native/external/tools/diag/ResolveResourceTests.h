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
        class ResolveResourceTests : public BaseTestClass<ResolveResourceTests>
        {
        public:
            BEGIN_TEST_CLASS(ResolveResourceTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ResolveThemeResource)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveStaticResource)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveStaticResourceInThemeDictionary)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveStaticResourceInMergedDictionary)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveThemeResourceInMergedDictionary)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ValidateResourceResolutionLogic)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveThemeResourceInStyle)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveStaticResourceInStyle)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveThemeResourceOnSetter)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveStaticResourceOnSetter)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(CorrectlyResolvesStaticResourceInTemplate)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveResourceOnDictioaryItemCorrectlyUpdatesReferences)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveStaticResourceInVisualState)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveStaticResourceStyleInParentDictionary)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(CanResolveAppAndSystemResources)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveResourceFromElementStyleProperty)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(ResolveResourceFromElementStylePropertyInStyle)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(CanResolveEnumTypes)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(DontCrashResolvingNonDO)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveFallbackValueUpdatesTarget)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveTargetNullValueUpdatesTarget)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveConverterUpdatesTarget)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveNestedStyle)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveCustomPropertyWithCustomType)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(VerifyResolveDataTemplateSelector)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDontThrowUnhandledExceptionOnInvalidResource)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanResolveNullExtension)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(CanResolveColorToBrush)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanResolveStringToBrush)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CorrectlyResolvesStaticResourceInRuntimeBuiltTemplate)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestNonDPResolveResource)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can resolve a resource used by a non-DP")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(ResolveResourceOutsideDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can resolve a resource declared outside a DataTemplate")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
        private:
            //Helper method for creating a Style
            Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper LoadXamlFromFunction(const std::function<UIElement ^ ()> func, wrl::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback);
            void ResolveResourceInStyle(ResourceType type);
            void ResolveResourceOnSetter(ResourceType type);
        };
    }
} } } } }
