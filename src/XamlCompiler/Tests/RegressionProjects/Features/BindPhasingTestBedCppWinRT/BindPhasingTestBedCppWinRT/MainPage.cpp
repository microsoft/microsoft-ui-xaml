// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include "MyItem.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace Microsoft::UI::Xaml::Shapes;

namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    MainPage::MainPage()
    {
        m_myItems = single_threaded_vector<BindPhasingTestBedCppWinRT::MyItem>();
        CreateTestItems();
        InitializeComponent();
    }

    // Create a simulated list of 150,000 items.
    void MainPage::CreateTestItems()
    {
        for (int i = 1; i < 150000; i++)
        {
            hstring suffix = to_hstring(i);

            m_myItems.Append(make<implementation::MyItem>(
                L"Title:" + suffix,                                 // Title.
                L"Sub:" + suffix,                                   // Subtitle.
                L"Desc:" + suffix,                                  // Description.
                make<implementation::MyInfo>(
                    L"ImageUrl" + suffix,                           // ImageUrl of MyInfo
                    L"Caption" + suffix),                           // Caption of MyInfo
                make<implementation::ExtraInfo>(
                    L"OtherCaption" + suffix),
                L"DP" + suffix));
        }
    }

    // Connect the grid view to the list of items.
    void MainPage::OnNavigatedTo(NavigationEventArgs const& /* e */)
    {
        myGridView().ItemsSource(m_myItems);
    }

    // Display each item incrementally to improve performance.
    void MainPage::MyGridView_ContainerContentChanging(
        ListViewBase const& /* sender */,
        ContainerContentChangingEventArgs const& args)
    {
        args.Handled(true);

        if (args.Phase() != 0)
        {
            throw hresult_error(E_FAIL);
        }

        // First, show the items' placeholders.
        auto templateRoot = args.ItemContainer().ContentTemplateRoot().try_as<StackPanel>();
        // Qualified: pch.h pulls in windows.h, whose wingdi.h declares a global Rectangle()
        // function that makes the unqualified name ambiguous.
        auto placeholderRectangle = templateRoot.FindName(L"placeholderRectangle").try_as<Shapes::Rectangle>();
        auto titleTextBlock = templateRoot.FindName(L"titleTextBlock").try_as<TextBlock>();
        auto subtitleTextBlock = templateRoot.FindName(L"subtitleTextBlock").try_as<TextBlock>();
        auto descriptionTextBlock = templateRoot.FindName(L"descriptionTextBlock").try_as<TextBlock>();

        // Make the placeholder rectangle opaque.
        placeholderRectangle.Opacity(1);

        // Make everything else invisible.
        titleTextBlock.Opacity(0);
        subtitleTextBlock.Opacity(0);
        descriptionTextBlock.Opacity(0);

        // Show the items' titles in the next phase.
        args.RegisterUpdateCallback({ this, &MainPage::ShowTitle });
    }

    // Show the items' titles.
    void MainPage::ShowTitle(
        ListViewBase const& /* sender */,
        ContainerContentChangingEventArgs const& args)
    {
        if (args.Phase() != 1)
        {
            throw hresult_error(E_FAIL);
        }

        // Next, show the items' titles. Keep everything else invisible.
        auto myItem = args.Item().try_as<BindPhasingTestBedCppWinRT::MyItem>();
        auto itemContainer = args.ItemContainer().try_as<SelectorItem>();
        auto templateRoot = itemContainer.ContentTemplateRoot().try_as<StackPanel>();
        auto titleTextBlock = templateRoot.FindName(L"titleTextBlock").try_as<TextBlock>();

        titleTextBlock.Text(myItem.Title());
        titleTextBlock.Opacity(1);

        // Show the items' subtitles in the next phase.
        args.RegisterUpdateCallback({ this, &MainPage::ShowSubtitle });
    }

    // Show the items' subtitles.
    void MainPage::ShowSubtitle(
        ListViewBase const& /* sender */,
        ContainerContentChangingEventArgs const& args)
    {
        if (args.Phase() != 2)
        {
            throw hresult_error(E_FAIL);
        }

        // Next, show the items' subtitles. Keep everything else invisible.
        auto myItem = args.Item().try_as<BindPhasingTestBedCppWinRT::MyItem>();
        auto itemContainer = args.ItemContainer().try_as<SelectorItem>();
        auto templateRoot = itemContainer.ContentTemplateRoot().try_as<StackPanel>();
        auto subtitleTextBlock = templateRoot.FindName(L"subtitleTextBlock").try_as<TextBlock>();

        subtitleTextBlock.Text(myItem.Subtitle());
        subtitleTextBlock.Opacity(1);

        // Show the items' descriptions in the next phase.
        args.RegisterUpdateCallback({ this, &MainPage::ShowDescription });
    }

    // Show the items' descriptions.
    void MainPage::ShowDescription(
        ListViewBase const& /* sender */,
        ContainerContentChangingEventArgs const& args)
    {
        if (args.Phase() != 3)
        {
            throw hresult_error(E_FAIL);
        }

        // Finally, show the items' descriptions.
        auto myItem = args.Item().try_as<BindPhasingTestBedCppWinRT::MyItem>();
        auto itemContainer = args.ItemContainer().try_as<SelectorItem>();
        auto templateRoot = itemContainer.ContentTemplateRoot().try_as<StackPanel>();

        auto placeholderRectangle = templateRoot.FindName(L"placeholderRectangle").try_as<Shapes::Rectangle>();
        auto descriptionTextBlock = templateRoot.FindName(L"descriptionTextBlock").try_as<TextBlock>();

        descriptionTextBlock.Text(myItem.Description());
        descriptionTextBlock.Opacity(1);

        // Make the placeholder rectangle invisible.
        placeholderRectangle.Opacity(0);

        // Undefer
        templateRoot.FindName(L"deferedTextBlock");
        templateRoot.FindName(L"deferedAndPhasedTextBlock");
    }
}
