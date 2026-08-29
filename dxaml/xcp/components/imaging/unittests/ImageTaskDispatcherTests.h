// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

class ImageTaskDispatcher;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Imaging
{
    class ImageTaskDispatcherTests : public WEX::TestClass<ImageTaskDispatcherTests>
    {
    public:
        BEGIN_TEST_CLASS(ImageTaskDispatcherTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ProcessTasksWithRequestId)
            TEST_METHOD_PROPERTY(L"Description", L"Verify tasks with request Id do not duplicate")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ProcessTasksWithoutRequestId)
            TEST_METHOD_PROPERTY(L"Description", L"Verify all tasks without request Id get executed")
        END_TEST_METHOD()

    private:
        xref_ptr<ImageTaskDispatcher> m_dispatcher;
    };

}}}}}}
