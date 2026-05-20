// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <Collection.h>

#include <limits>

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Markup;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class PanelsHelper
    {
    public:
        template <class TPanel>
        static TPanel^ AddPanelWithContent(Platform::Collections::Vector<UIElement^>^ contentVector, Orientation orientation)
        {
            TPanel^ panel = nullptr;
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Panel, Loaded);
            auto loadedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Adding the %s to the visual tree with %d items.", GetClassName<TPanel>()->Data(), contentVector->Size);
                panel = ref new TPanel();

                panel->Width = 300;
                panel->Height = 300;
                panel->Orientation = orientation;

                for (auto contentItem : contentVector)
                {
                    panel->Children->Append(contentItem);
                }

                loadedRegistration.Attach(panel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = panel;
            });

            LOG_OUTPUT(L"Waiting for the %s to be loaded...", GetClassName<TPanel>()->Data());
            loadedEvent->WaitForDefault();
            LOG_OUTPUT(L"%s loaded.", GetClassName<TPanel>()->Data());

            return panel;
        }

        static Platform::Collections::Vector<UIElement^>^ CreateDefaultPanelContent(int numItemsToAdd)
        {
            Platform::Collections::Vector<UIElement^>^ itemsVector = nullptr;

            RunOnUIThread([&]()
            {
                itemsVector = ref new Platform::Collections::Vector<UIElement^>;

                for (int i = 0; i < numItemsToAdd; i++)
                {
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                    SolidColorBrush^ redBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                    SolidColorBrush^ blueBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                    switch (i % 2)
                    {
                    case 0:
                        rect->Fill = redBrush;
                        break;
                    case 1:
                        rect->Fill = blueBrush;
                        break;
                    }

                    rect->Width = 100;
                    rect->Height = 100;

                    itemsVector->Append(rect);
                }
            });

            return itemsVector;
        }

        template <class TItemsControl, class TPanel>
        static TItemsControl^ CreateItemsControlWithPanel(Platform::String^ xamlPanelProperties = L"")
        {
            TItemsControl^ itemsControl = nullptr;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating a %s with a %s as its panel.", GetClassName<TItemsControl>()->Data(), GetClassName<TPanel>()->Data());

                itemsControl = ref new TItemsControl();
                itemsControl->Width = 300;
                itemsControl->Height = 300;

                itemsControl->ItemsPanel =
                    dynamic_cast<ItemsPanelTemplate^>(XamlReader::Load(
                        L"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><" + GetClassName<TPanel>() + L" " + xamlPanelProperties + "/></ItemsPanelTemplate>"));

                ApplyContainerStyle<TItemsControl>(itemsControl);
            });

            return itemsControl;
        }

        template <class TItemsControl, class TPanel>
        static TItemsControl^ AddItemsControlWithPanel(int numItemsToAdd, Platform::String^ xamlPanelProperties = L"")
        {
            TItemsControl^ itemsControl = CreateItemsControlWithPanel<TItemsControl, TPanel>(xamlPanelProperties);
            auto panelContent = CreateDefaultPanelContent(numItemsToAdd);
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ItemsControl, Loaded);
            auto loadedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Adding the %s to the visual tree with %d items.", GetClassName<TItemsControl>()->Data(), numItemsToAdd);

                itemsControl->ItemsSource = panelContent;

                loadedRegistration.Attach(itemsControl, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = itemsControl;
            });

            LOG_OUTPUT(L"Waiting for %s to be loaded...", GetClassName<TItemsControl>()->Data());
            loadedEvent->WaitForDefault();
            LOG_OUTPUT(L"%s loaded.", GetClassName<TItemsControl>()->Data());

            return itemsControl;
        }

        static void VerifyItemPositions(Panel^ panel, Platform::Collections::Vector<wf::Point>^ expectedPositions)
        {
            RunOnUIThread([&]()
            {
                VERIFY_IS_LESS_THAN_OR_EQUAL(expectedPositions->Size, panel->Children->Size);

                for (unsigned int i = 0; i < expectedPositions->Size; i++)
                {
                    auto container = dynamic_cast<UIElement^>(panel->Children->GetAt(i));
                    auto transform = container->TransformToVisual(panel);
                    wf::Point actualPosition = transform->TransformPoint(wf::Point(0, 0));
                    wf::Point expectedPosition = expectedPositions->GetAt(i);

                    VERIFY_ARE_EQUAL(Round(expectedPosition.X), Round(actualPosition.X));
                    VERIFY_ARE_EQUAL(Round(expectedPosition.Y), Round(actualPosition.Y));
                }
            });
        }

        static void VerifyItemPositions(ItemsControl^ itemsControl, Platform::Collections::Vector<wf::Point>^ expectedPositions)
        {
            RunOnUIThread([&]()
            {
                VERIFY_IS_LESS_THAN_OR_EQUAL(expectedPositions->Size, itemsControl->Items->Size);

                for (unsigned int i = 0; i < expectedPositions->Size; i++)
                {
                    auto container = dynamic_cast<UIElement^>(itemsControl->ContainerFromIndex(i));
                    auto transform = container->TransformToVisual(itemsControl);
                    wf::Point actualPosition = transform->TransformPoint(wf::Point(0, 0));
                    wf::Point expectedPosition = expectedPositions->GetAt(i);

                    VERIFY_ARE_EQUAL(Round(expectedPosition.X), Round(actualPosition.X));
                    VERIFY_ARE_EQUAL(Round(expectedPosition.Y), Round(actualPosition.Y));
                }
            });
        }

        static void VerifyPanelDesiredSize(Panel^ panel, double expectedWidth, double expectedHeight)
        {
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(Round(expectedWidth), Round(panel->DesiredSize.Width));
                VERIFY_ARE_EQUAL(Round(expectedHeight), Round(panel->DesiredSize.Height));
            });
        }

    private:
        static int Round(double value)
        {
            return (int)(value + 0.5);
        }

        template <class TItemsControl>
        static void ApplyContainerStyle(TItemsControl^ itemsControl);

        template <>
        static void ApplyContainerStyle<xaml_controls::ItemsControl>(xaml_controls::ItemsControl^ itemsControl)
        {
            // There's no container style to apply for ItemsControls, so there's nothing for us to do here.
            // This implementation exists because the method that calls this is called for ItemsControls,
            // so we need to provide this implementation.
        }

        template <>
        static void ApplyContainerStyle<xaml_controls::ListView>(xaml_controls::ListView^ itemsControl)
        {
            itemsControl->ItemContainerStyle =
                dynamic_cast<Style^>(XamlReader::Load(
                    L"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='ListViewItem'>"
                    L"    <Setter Property='Margin' Value='0'/>"
                    L"    <Setter Property='BorderThickness' Value='0'/>"
                    L"    <Setter Property='Template'>"
                    L"        <Setter.Value>"
                    L"            <ControlTemplate TargetType='ListViewItem'>"
                    L"                <ListViewItemPresenter"
                    L"                    PointerOverBackgroundMargin='0'"
                    L"                    ContentMargin='0' />"
                    L"            </ControlTemplate>"
                    L"        </Setter.Value>"
                    L"    </Setter>"
                    L"</Style>"));
        }
    };

} } } } }
