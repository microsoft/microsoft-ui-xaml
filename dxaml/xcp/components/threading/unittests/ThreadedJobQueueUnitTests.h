// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <ThreadedJobQueue.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Threading {

        class ThreadedJobQueueUnitTests : public WEX::TestClass<ThreadedJobQueueUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ThreadedJobQueueUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            TEST_METHOD(QueueSingleJob)
            TEST_METHOD(QueueMultipleJobs)
            TEST_METHOD(QueueMultipleJobsTerminateEarly)
            TEST_METHOD(QueueMultipleJobsThreadRestart)
            TEST_METHOD(QueueMultipleJobsSTA)
            TEST_METHOD(QueueMultipleJobsMTA)
            TEST_METHOD(QueueJobExtendsShutdown)

        private:
            void QueueMultipleJobsHelper(
                ThreadedJobQueue::ThreadingModel threadingModel,
                bool terminateOnInactivity);
        };
    }}
} } } }
