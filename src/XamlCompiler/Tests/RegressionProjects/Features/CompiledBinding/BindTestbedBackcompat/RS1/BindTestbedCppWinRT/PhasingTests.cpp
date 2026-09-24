// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PhasingTests.h"
#include "DetectLeaksPage.h"
#include "MyItem.h"
#include "MyInfo.h"
#include "ExtraInfo.h"

namespace winrt::BindTestbed::implementation
{
    wux::DependencyProperty MyItem::dpOnMyItemProperty =
        RegisterDependencyProperty(
            L"DPOnMyItem",
            xaml_typename<hstring>(),
            xaml_typename<BindTestbed::MyItem>(),
            nullptr);

    PhasingTests::PhasingTests()
    {
        myItems = single_threaded_observable_vector<IInspectable>();
        InitializeComponent();
        InitializeValues();
        myGridView().ItemsSource(myItems);
        Initialized = true;
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::PhasingTests>().Name);
    }

    void PhasingTests::InitializeValues()
    {
        std::wstring title(L"Title");
        std::wstring subtitle(L"Sub");
        std::wstring description(L"Description");
        std::wstring imageUrl(L"ImageUrl");
        std::wstring caption(L"Caption");
        std::wstring otherCaption(L"OtherCaption");
        std::wstring dp(L"DP");

        for (int i = 1; i < itemsCount; i++)
        {
            auto no = std::to_wstring(i);
            auto myItem = make<MyItem>(i,
                title.append(no).data(),
                subtitle.append(no).data(),
                description.append(no).data(),
                make<MyInfo>(i, imageUrl.append(no).data(), caption.append(no).data()),
                make<ExtraInfo>(otherCaption.append(no).data()), dp.append(no).data());
            myItems.Append(myItem);
        }
    }

    void PhasingTests::Reset_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        myItems.Clear();
        InitializeValues();
    }

    void PhasingTests::Reload_Click(IInspectable const&, wux::RoutedEventArgs const&)
    {
        myItems = single_threaded_observable_vector<IInspectable>();
        InitializeValues();
        myGridView().ItemsSource(myItems);
    }

    void PhasingTests::MyGridView_ContainerContentChanging(
        wuxc::ListViewBase const&,
        wuxc::ContainerContentChangingEventArgs const& args)
    {
        wait(1);
        args.Handled(true);
        if (args.Phase() < 20)
        {
            args.RegisterUpdateCallback({ this, &PhasingTests::MyGridView_ContainerContentChanging });
        }
    }

    void PhasingTests::wait(int)
    {}

    void PhasingTests::StackPanel_PointerReleased(IInspectable const& sender, wux::Input::PointerRoutedEventArgs const&)
    {
        auto root = sender.as<wuxc::StackPanel>();
        root.Background(wux::Media::SolidColorBrush(::Windows::UI::Colors::White()));
        root.FindName(L"deferedTextBlock");
        root.FindName(L"deferedAndPhasedTextBlock");
    }

    void PhasingTests::SlowPhasing_UnChecked(IInspectable const& sender, wux::RoutedEventArgs const& e)
    {
        SlowPhasing_Checked(sender, e);
    }

    void PhasingTests::SlowPhasing_Checked(IInspectable const&, wux::RoutedEventArgs const&)
    {
        if (Initialized)
        {
            if (SlowPhasing().IsChecked().Value())
            {
                cccEventToken = myGridView().ContainerContentChanging({ this, &PhasingTests::MyGridView_ContainerContentChanging });
            }
            else
            {
                myGridView().ContainerContentChanging(cccEventToken);
            }
         }
    }

    void PhasingTests::PhasedTemplate_UnChecked(IInspectable const& sender, wux::RoutedEventArgs const& e)
    {
        PhasedTemplate_Checked(sender, e);
    }

    void PhasingTests::PhasedTemplate_Checked(IInspectable const&, wux::RoutedEventArgs const&)
    {
        if (Initialized)
        {
            if (PhasedTemplateCbx().IsChecked().Value())
            {
                myGridView().ItemTemplate(unbox_value<wux::DataTemplate>(Resources().Lookup(box_value(hstring(L"PhasedTemplate")))));
            }
            else
            {
                myGridView().ItemTemplate(unbox_value<wux::DataTemplate>(Resources().Lookup(box_value(hstring(L"NonPhasedTemplate")))));
            }
        }
    }
}