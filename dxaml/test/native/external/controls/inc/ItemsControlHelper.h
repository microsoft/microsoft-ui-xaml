// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <Collection.h>

#include <TreeHelper.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class ItemsControlHelper
    {
    public:
        template <class TItemsControl>
        static TItemsControl^ AddItemsControl(bool addItemsDirectly, int numItemsToAdd)
        {
            TItemsControl^ itemsControl = nullptr;
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ItemsControl, Loaded);
            auto loadedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating and adding the %s to the visual tree with %d items.", GetClassName<TItemsControl>()->Data(), numItemsToAdd);

                itemsControl = ref new TItemsControl();
                itemsControl->Width = 200;
                itemsControl->Height = 200;

                if (addItemsDirectly)
                {
                    for (int i = 0; i < numItemsToAdd; i++)
                    {
                        auto s = ref new Platform::String(L"Item ");
                        s += i;

                        itemsControl->Items->Append(s);
                    }
                }
                else
                {
                    auto itemsVector = ref new Platform::Collections::Vector<Platform::String^>();

                    for (int i = 0; i < numItemsToAdd; i++)
                    {
                        auto s = ref new Platform::String(L"Item ");
                        s += i;

                        itemsVector->Append(s);
                    }

                    itemsControl->ItemsSource = itemsVector;
                }

                loadedRegistration.Attach(itemsControl, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = itemsControl;
            });

            LOG_OUTPUT(L"Waiting for %s to be loaded...", GetClassName<TItemsControl>()->Data());
            loadedEvent->WaitForDefault();
            LOG_OUTPUT(L"%s loaded.", GetClassName<TItemsControl>()->Data());
            TestServices::WindowHelper->WaitForIdle();

            return itemsControl;
        }

        template <class TListItemsControl, class TItemsClass>
        static void ScrollToIndex(TListItemsControl^ itemsControl, unsigned int index)
        {
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
            auto viewChangedEvent = std::make_shared<Event>();

            RunOnUIThread([&] ()
            {
                scrollViewer = dynamic_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(itemsControl, L"ScrollViewer"));

                viewChangedRegistration.Attach(scrollViewer,
                    ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    if (args->IsIntermediate == false)
                    {
                        viewChangedEvent->Set();
                    }
                }));

                LOG_OUTPUT(L"Scrolling to index %d.", index);

                auto itemsVector = dynamic_cast<Platform::Collections::Vector<TItemsClass^>^>(itemsControl->ItemsSource);
                itemsControl->ScrollIntoView(itemsVector->GetAt(index));
            });

            LOG_OUTPUT(L"Waiting for scroll to complete...");
            viewChangedEvent->WaitForDefault();
            LOG_OUTPUT(L"Done.");
        }

        template <class TItemsControl>
        static Platform::Collections::Vector<xaml::DependencyObject^>^ GetContainersFromVisualTree(TItemsControl^ itemsControl, int expectedItemCount = 0)
        {
            auto containersVector = ref new Platform::Collections::Vector<DependencyObject^>();
            auto containerPanel = GetContainerPanelFromControl(itemsControl);

            RunOnUIThread([&]()
            {
                if (VerifyContainerPanel<TItemsControl>(containerPanel) == false)
                {
                    return;
                }

                int childCount = xaml_media::VisualTreeHelper::GetChildrenCount(containerPanel);

                if (expectedItemCount > 0 && childCount != expectedItemCount)
                {
                    return;
                }

                for (int i = 0; i < childCount; i++)
                {
                    containersVector->Append(xaml_media::VisualTreeHelper::GetChild(containerPanel, i));
                }
            });

            return containersVector;
        }

        template <class TItemsControl, class TExpectedContainerClass>
        static void VerifyContainersAndItems(TItemsControl^ itemsControl, int expectedItemCount)
        {
            auto containersVector = ItemsControlHelper::GetContainersFromVisualTree<TItemsControl>(itemsControl, expectedItemCount);

            RunOnUIThread([&]()
            {
                for (unsigned int i = 0; i < containersVector->Size; i++)
                {
                    auto object = containersVector->GetAt(i);
                    TExpectedContainerClass^ container = dynamic_cast<TExpectedContainerClass^>(object);

                    LOG_OUTPUT(L"Expected a %s child at index %d, received %s.", GetClassName<TExpectedContainerClass>()->Data(), i, GetClassName(object->GetType())->Data());
                    VERIFY_IS_NOT_NULL(container);

                    auto expectedString = ref new Platform::String(L"Item ");
                    expectedString += i;

                    LOG_OUTPUT(L"Got a %s at index %d. Verifying that its string content is '%s'.", GetClassName<TExpectedContainerClass>()->Data(), i, expectedString->Data());

                    auto actualString = dynamic_cast<Platform::String^>(container->Content);

                    if (actualString == nullptr)
                    {
                        LOG_OUTPUT(L"Content was not a string.");
                        VERIFY_FAIL();
                    }
                    else if (actualString != expectedString)
                    {
                        LOG_OUTPUT(L"Content was '%s' instead.", actualString->Data());
                        VERIFY_FAIL();
                    }
                    else
                    {
                        LOG_OUTPUT(L"Verified. String was '%s'.", expectedString->Data());
                    }
                }
            });
        }

        template <class TPanel>
        static TPanel^ GetPanelFromItemsControl(xaml_controls::ItemsControl^ itemsControl)
        {
            return dynamic_cast<TPanel^>(GetContainerPanelFromControl(itemsControl));
        }

    private:
        static xaml_controls::Panel^ GetContainerPanelFromControl(xaml_controls::ItemsControl^ itemsControl)
        {
            xaml_controls::Panel^ containerPanel = nullptr;
            xaml_controls::ItemsPresenter^ itemsPresenter = nullptr;

            RunOnUIThread([&]()
            {
                int childCount = 0;
                xaml::DependencyObject^ nextObject = nullptr;

                itemsPresenter = TreeHelper::GetVisualChildByType<xaml_controls::ItemsPresenter>(itemsControl);

                if (itemsPresenter == nullptr)
                {
                    return;
                }

                childCount = xaml_media::VisualTreeHelper::GetChildrenCount(itemsPresenter);

                if (childCount != 3)
                {
                    return;
                }

                nextObject = xaml_media::VisualTreeHelper::GetChild(itemsPresenter, 1);
                auto panel = dynamic_cast<xaml_controls::Panel^>(nextObject);

                if (panel == nullptr)
                {
                    return;
                }

                containerPanel = panel;
            });

            return containerPanel;
        }

        template <class TItemsControl>
        static bool VerifyContainerPanel(xaml_controls::Panel^ containerPanel);

        template <>
        static bool VerifyContainerPanel<xaml_controls::ItemsControl>(xaml_controls::Panel^ containerPanel)
        {
            auto stackPanel = dynamic_cast<xaml_controls::StackPanel^>(containerPanel);

            if (stackPanel == nullptr)
            {
                LOG_OUTPUT(L"Expected a StackPanel container panel, received %s.", GetClassName(containerPanel->GetType())->Data());
                VERIFY_FAIL();

                return false;
            }

            return true;
        }

        template <>
        static bool VerifyContainerPanel<xaml_controls::ListBox>(xaml_controls::Panel^ containerPanel)
        {
            auto virtualizingStackPanel = dynamic_cast<xaml_controls::VirtualizingStackPanel^>(containerPanel);

            if (virtualizingStackPanel == nullptr)
            {
                LOG_OUTPUT(L"Expected a VirtualizingStackPanel container panel, received %s.", GetClassName(containerPanel->GetType())->Data());
                VERIFY_FAIL();

                return false;
            }

            return true;
        }
    };

} } } } }
