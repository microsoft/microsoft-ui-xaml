// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageTaskDispatcherTests.h"

#include <ImageProviderInterfaces.h>
#include <ImageTaskDispatcher.h>
#include <refcounting.h>

namespace
{
    class TestTask : public CXcpObjectBase<IImageTask>
    {
    public:
        uint64_t requestId = 0;
        int executeCount = 0;
        std::function<HRESULT()> onExecute;

        uint64_t GetRequestId() const override
        {
            return requestId;
        }

        _Check_return_ HRESULT Execute() override
        {
            executeCount++;
            return S_OK;
        }
    };
}

using namespace ::Windows::UI::Xaml::Tests::Foundation::Imaging;

bool ImageTaskDispatcherTests::TestSetup()
{
    m_dispatcher = make_xref<ImageTaskDispatcher>(nullptr /* core */);
    return true;
}

bool ImageTaskDispatcherTests::TestCleanup()
{
    m_dispatcher.reset();
    return true;
}

void ImageTaskDispatcherTests::ProcessTasksWithRequestId()
{
    auto task0 = make_xref<TestTask>();
    task0->requestId = 42;
    VERIFY_SUCCEEDED(m_dispatcher->QueueTask(task0));

    auto task1 = make_xref<TestTask>();
    task1->requestId = 42;
    VERIFY_SUCCEEDED(m_dispatcher->QueueTask(task1));

   VERIFY_SUCCEEDED( m_dispatcher->Execute());

    VERIFY_ARE_EQUAL(0, task0->executeCount);
    VERIFY_ARE_EQUAL(1, task1->executeCount);
}

void ImageTaskDispatcherTests::ProcessTasksWithoutRequestId()
{
    auto task0 = make_xref<TestTask>();
    VERIFY_SUCCEEDED(m_dispatcher->QueueTask(task0));

    auto task1 = make_xref<TestTask>();
    VERIFY_SUCCEEDED(m_dispatcher->QueueTask(task1));

    VERIFY_SUCCEEDED(m_dispatcher->Execute());

    VERIFY_ARE_EQUAL(1, task0->executeCount);
    VERIFY_ARE_EQUAL(1, task1->executeCount);
}
