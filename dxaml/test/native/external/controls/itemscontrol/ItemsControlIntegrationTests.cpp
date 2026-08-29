// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ItemsControlIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

#include <ItemsControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ItemsControl {

    bool ItemsControlIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ItemsControlIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ItemsControlIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ItemsControlIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ItemsControl>::CanInstantiate();
    }

    void ItemsControlIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ItemsControl>::CanEnterAndLeaveLiveTree();
    }

    void ItemsControlIntegrationTests::CanAddItemsDirectly()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ItemsControl^ itemsControl = ItemsControlHelper::AddItemsControl<xaml_controls::ItemsControl>(true /* addItemsDirectly */, s_itemCount);
        ItemsControlHelper::VerifyContainersAndItems<xaml_controls::ItemsControl, ContentPresenter>(itemsControl, s_itemCount);
    }

    void ItemsControlIntegrationTests::CanAddItemsFromSource()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ItemsControl^ itemsControl = ItemsControlHelper::AddItemsControl<xaml_controls::ItemsControl>(false /* addItemsDirectly */, s_itemCount);
        ItemsControlHelper::VerifyContainersAndItems<xaml_controls::ItemsControl, ContentPresenter>(itemsControl, s_itemCount);
    }

    void ItemsControlIntegrationTests::CanGetContainersFromItems()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = ItemsControlHelper::AddItemsControl<xaml_controls::ItemsControl>(false /* addItemsDirectly */, s_itemCount);
        auto containersVector = ItemsControlHelper::GetContainersFromVisualTree<xaml_controls::ItemsControl>(itemsControl);

        RunOnUIThread([&]()
        {
            for (unsigned int i = 0; i < containersVector->Size; i++)
            {
                auto itemString = ref new Platform::String(L"Item ");
                itemString += i;

                VERIFY_ARE_EQUAL(containersVector->GetAt(i), itemsControl->ContainerFromItem(itemString));
            }
        });
    }

    void ItemsControlIntegrationTests::CanGetContainersFromIndexes()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = ItemsControlHelper::AddItemsControl<xaml_controls::ItemsControl>(false /* addItemsDirectly */, s_itemCount);
        auto containersVector = ItemsControlHelper::GetContainersFromVisualTree<xaml_controls::ItemsControl>(itemsControl);

        RunOnUIThread([&]()
        {
            for (unsigned int i = 0; i < containersVector->Size; i++)
            {
                VERIFY_ARE_EQUAL(containersVector->GetAt(i), itemsControl->ContainerFromIndex(i));
            }
        });
    }

    void ItemsControlIntegrationTests::CanGetItemsFromContainers()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = ItemsControlHelper::AddItemsControl<xaml_controls::ItemsControl>(false /* addItemsDirectly */, s_itemCount);
        auto containersVector = ItemsControlHelper::GetContainersFromVisualTree<xaml_controls::ItemsControl>(itemsControl);

        RunOnUIThread([&]()
        {
            for (unsigned int i = 0; i < containersVector->Size; i++)
            {
                Platform::String^ expectedItemString = "Item ";
                expectedItemString += i;

                Platform::String^ actualItemString = dynamic_cast<Platform::String^>(itemsControl->ItemFromContainer(containersVector->GetAt(i)));

                VERIFY_IS_NOT_NULL(actualItemString);
                VERIFY_ARE_EQUAL(expectedItemString, actualItemString);
            }
        });
    }

    void ItemsControlIntegrationTests::CanGetIndexesFromContainers()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = ItemsControlHelper::AddItemsControl<xaml_controls::ItemsControl>(false /* addItemsDirectly */, s_itemCount);
        auto containersVector = ItemsControlHelper::GetContainersFromVisualTree<xaml_controls::ItemsControl>(itemsControl);

        RunOnUIThread([&]()
        {
            for (unsigned int i = 0; i < containersVector->Size; i++)
            {
                VERIFY_ARE_EQUAL((int)i, itemsControl->IndexFromContainer(containersVector->GetAt(i)));
            }
        });
    }

    void ItemsControlIntegrationTests::CanLayoutWithoutItemContainerGenerator()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            xaml_controls::StackPanel^ rootPanel = ref new xaml_controls::StackPanel();
            scrollViewer = ref new xaml_controls::ScrollViewer();
            rootPanel->Children->Append(scrollViewer);

            loadedRegistration.Attach(
                scrollViewer,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"ScrollViewer.Loaded event.");
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for ScrollViewer.Loaded event.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::GridView^ gridView = ref new xaml_controls::GridView();
            scrollViewer->Content = gridView;
            scrollViewer->UpdateLayout();
            gridView->ItemsPanel = safe_cast<ItemsPanelTemplate^>(xaml_markup::XamlReader::LoadWithInitialTemplateValidation(
                LR"(<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <WrapGrid/>
                    </ItemsPanelTemplate>)"));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ItemsControl
