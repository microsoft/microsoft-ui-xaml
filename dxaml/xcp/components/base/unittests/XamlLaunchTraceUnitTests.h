// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifdef XAMLPROFILER_ENABLED

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {

    class XamlLaunchTraceUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlLaunchTraceUnitTests)
        END_TEST_CLASS()

        TEST_METHOD(LinearBoundaries);
        TEST_METHOD(RenderingBeforeCallbackReturn);
        TEST_METHOD(CallbackFailureDoesNotStopFrameObservations);
        TEST_METHOD(RetriesKeepBoundaryAndAttemptIdentity);
        TEST_METHOD(IndependentCoreLifetimes);
        TEST_METHOD(StartupScopeIsThreadLocal);
        TEST_METHOD(NestedStartupScopes);
        TEST_METHOD(CallbackReturnAfterCoreReplacement);
        TEST_METHOD(WarmActivationAndLaterFramesDoNotRestart);
        TEST_METHOD(DrawCleanupFailureIsPreserved);
    };

} } } } }

#endif // XAMLPROFILER_ENABLED
