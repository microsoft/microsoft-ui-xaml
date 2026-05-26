// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {
    namespace ScreenCapture {
        // This isn't supported because taking a screenshot in an isolated test doesn't make much sense
        // because currently these tests don't render anything to the screen. If this changes, maybe we
        // find a way to share the actual implementation that's in WindowingHelper.cpp (or even better, see
        // if TAEF has a single API that supports screenshots across platforms and get rid of ours).
        void TakeScreenshot(const wchar_t*)
        {
            WEX::Logging::Log::Warning(L"Trying to take screenshot in an isolated test is not supported");
        }
    }
} } } } }


