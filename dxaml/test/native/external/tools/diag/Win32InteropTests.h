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
namespace test_common = Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        class Win32InteropTests : public BaseTestClass<Win32InteropTests>
        {
        public:
            BEGIN_TEST_CLASS(Win32InteropTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\Microsoft.UI.Xaml.hosting.desktopwindowxamlsource.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")     // TODO: DCPP: lifted hwnd Xaml islands
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VerifyMutationEventsUsingDesktopWindowXamlSource)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"UAP:AppXManifest", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"UAP:Host", L"PackagedCwa")
                TEST_METHOD_PROPERTY(L"UAP:Praid", L"XamlNativeTAEFTests")
                TEST_METHOD_PROPERTY(L"ThreadingModel", L"STA")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UseDesktopWindowXamlSourceSystemBackdrop)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"UAP:Praid", L"XamlNativeTAEFTests")
                TEST_METHOD_PROPERTY(L"ThreadingModel", L"STA")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetDisconnectedDesktopWindowXamlSourceSystemBackdrop)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"UAP:Praid", L"XamlNativeTAEFTests")
                TEST_METHOD_PROPERTY(L"ThreadingModel", L"STA")
            END_TEST_METHOD()            
        };
    }
} } } } }