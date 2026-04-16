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
        class ElementStateChangedTests : public BaseTestClass<ElementStateChangedTests>
        {
        public:
            BEGIN_TEST_CLASS(ElementStateChangedTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            TEST_METHOD(VerifyPreviousErrorResolves)
            TEST_METHOD(VerifyVSMErrorPath)
            TEST_METHOD(VerifyResourceDictionaryPath)
            TEST_METHOD(VerifyResourceDictionaryPathStyle)
            TEST_METHOD(VerifyResourceDictionaryPathImplicitStyle)
            TEST_METHOD(VerifyPathWithCustomType)
            TEST_METHOD(VerifyPathWithUIElementParent)
            TEST_METHOD(VerifyMergedDictionaryPath)
            TEST_METHOD(VerifyPathToBindingProperty)
            TEST_METHOD(VerifyRemovedApplicationResourceError)
            TEST_METHOD(VerifyStateChangeWhenNotInTree)
            TEST_METHOD(VerifyPathWithNestedStyles)
            BEGIN_TEST_METHOD(VerifyPathToItemsControlItemContainerStyle)
                TEST_METHOD_PROPERTY(L"UAP:AppXManifest", L"AppxManifest.DesignMode.xml")
            END_TEST_METHOD()
            TEST_METHOD(VerifyPathToFlyoutPresenterStyle)
        };
    }
} } } } }
