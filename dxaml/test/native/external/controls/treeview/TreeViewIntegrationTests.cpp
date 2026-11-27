// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TreeViewIntegrationTests.h"

#include <thread>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TreeView {

bool TreeViewIntegrationTests::ClassSetup()
{
    return true;
}

bool TreeViewIntegrationTests::TestSetup()
{
    return true;
}

bool TreeViewIntegrationTests::TestCleanup()
{
    return true;
}

void TreeViewIntegrationTests::RemoveAtEndRemovesLastChild()
{
    HRESULT result = S_OK;
    uint32_t sizeAfterFirstRemove = 0;
    uint32_t sizeAfterSecondRemove = 0;
    uint32_t finalSize = 0;
    bool firstRemoveKeptExpectedNodes = false;
    bool secondRemoveKeptExpectedNode = false;
    bool removedNodesWereDetached = false;

    std::thread uiThread([&]()
    {
        result = ::RoInitialize(RO_INIT_SINGLETHREADED);
        if (FAILED(result))
        {
            return;
        }

        try
        {
            auto dispatcherQueueController =
                Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
            auto windowsXamlManager =
                Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

            {
                auto parent = ref new xaml_controls::TreeViewNode();
                auto first = ref new xaml_controls::TreeViewNode();
                auto second = ref new xaml_controls::TreeViewNode();
                auto third = ref new xaml_controls::TreeViewNode();
                auto children = parent->Children;

                children->Append(first);
                children->Append(second);
                children->Append(third);

                children->RemoveAtEnd();
                sizeAfterFirstRemove = children->Size;
                firstRemoveKeptExpectedNodes =
                    children->GetAt(0) == first &&
                    children->GetAt(1) == second &&
                    third->Parent == nullptr &&
                    third->Depth == -1;

                children->RemoveAtEnd();
                sizeAfterSecondRemove = children->Size;
                secondRemoveKeptExpectedNode =
                    children->GetAt(0) == first &&
                    second->Parent == nullptr &&
                    second->Depth == -1;

                children->RemoveAtEnd();
                finalSize = children->Size;
                removedNodesWereDetached = first->Parent == nullptr && first->Depth == -1;
            }

            windowsXamlManager = nullptr;
            dispatcherQueueController->ShutdownQueue();
        }
        catch (Platform::Exception^ e)
        {
            result = e->HResult;
        }

        ::RoUninitialize();
    });
    uiThread.join();

    VERIFY_SUCCEEDED(result);
    VERIFY_ARE_EQUAL(2u, sizeAfterFirstRemove);
    VERIFY_IS_TRUE(firstRemoveKeptExpectedNodes);
    VERIFY_ARE_EQUAL(1u, sizeAfterSecondRemove);
    VERIFY_IS_TRUE(secondRemoveKeptExpectedNode);
    VERIFY_ARE_EQUAL(0u, finalSize);
    VERIFY_IS_TRUE(removedNodesWereDetached);
}

} } } } } }
