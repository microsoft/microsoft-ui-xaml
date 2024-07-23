#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include "MainWindow.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation::Collections;

namespace winrt::TabViewTearOutApp::implementation
{
    MainPage::MainPage()
    {
        m_stringList.Append(winrt::box_value(L"Item 1"));
        m_stringList.Append(winrt::box_value(L"Item 2"));
        m_stringList.Append(winrt::box_value(L"Item 3"));
    }

    void MainPage::Init(winrt::com_array<IInspectable> const& stringList)
    {
        if (!stringList.empty())
        {
            m_stringList.Clear();

            for (IInspectable string : stringList)
            {
                m_stringList.Append(string);
            }
        }
    }

    void MainPage::OnTabTearOutWindowRequested(TabView const& sender, TabViewTabTearOutWindowRequestedEventArgs const& args)
    {
        UNREFERENCED_PARAMETER(sender);

        HWND newWindowHwnd = MainWindow::CreateNewWindow(SW_HIDE, args.Items());
        args.NewWindowId(winrt::Microsoft::UI::GetWindowIdFromWindow(newWindowHwnd));
    }

    void MainPage::OnTabTearOutRequested(TabView const& sender, TabViewTabTearOutRequestedEventArgs const& args)
    {
        UNREFERENCED_PARAMETER(sender);
        auto items = args.Items();

        for (uint32_t i = 0; i < items.size(); i++)
        {
            auto item = items[i];
            uint32_t index;
            if (m_stringList.IndexOf(item, index))
            {
                m_stringList.RemoveAt(index);
            }
        }
    }

    void MainPage::OnExternalTornOutTabsDropping(TabView const& sender, TabViewExternalTornOutTabsDroppingEventArgs const& args)
    {
        UNREFERENCED_PARAMETER(sender);

        args.AllowDrop(true);
    }

    void MainPage::OnExternalTornOutTabsDropped(TabView const& sender, TabViewExternalTornOutTabsDroppedEventArgs const& args)
    {
        UNREFERENCED_PARAMETER(sender);
        auto items = args.Items();

        for (uint32_t i = 0; i < items.size(); i++)
        {
            uint32_t index;
            if (m_stringList.IndexOf(items[i], index))
            {
                m_stringList.RemoveAt(index);
            }
        }

        for (uint32_t i = 0; i < items.size(); i++)
        {
            m_stringList.InsertAt(args.DropIndex() + i, items[i]);
        }
    }
}
