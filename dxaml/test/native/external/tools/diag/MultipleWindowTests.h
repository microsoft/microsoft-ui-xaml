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
#include <XamlMetadataProviderOverrider.h>
#include "XamlDiagnosticsHelper.h"
#include "XamlDiagnosticsTestBase.h"

using namespace Microsoft::UI::Xaml::Controls;
namespace wrl = Microsoft::WRL;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {
        class MultipleWindowTests : public BaseTestClass<MultipleWindowTests>
        {
        public:
            BEGIN_TEST_CLASS(MultipleWindowTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")    // WPF_HOSTING_MODE_FAILURE - creates new UWP views.
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(BasicMultipleWindowTest_Notified)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get notified of a new window.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BasicMultipleWindowTest_Attach)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can attach to an app with multiple windows open.")
                TEST_METHOD_PROPERTY(L"Data:ShowMainView", L"{False, True}")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: MultipleWindowTests#metadataSet0::BasicMultipleWindowTest_Attach#metadataSet0 not reliably connecting the tree
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFailGettingWindowProperties)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we fail graciously trying to get the properties for a window.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(SetCoreDispatcherOnce)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we only set the core dispatcher once when running in single window mode in a "
                                                     L"multi-window app.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ProperlyHandleApplicationResources)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()
        };
    }
} } } } }
