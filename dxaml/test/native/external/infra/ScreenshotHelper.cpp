// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <windows.h>
#include <RuntimeParameters.h>
#include <TestEvent.h>
#include <NamespaceAliasesTest.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {  namespace ScreenCapture {
        void TakeScreenshot(const wchar_t* variation)
        {
            test_infra::TestServices::Utilities->CaptureScreen(ref new Platform::String(variation));
        }
    } }
} } } }