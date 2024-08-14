#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include "DocumentInfo.h"
#include "MainWindow.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation::Collections;

namespace winrt::TabViewTearOutApp::implementation
{
    MainPage::MainPage()
    {
        m_documentList.Append(winrt::make<DocumentInfo>(L"Document 1", L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."));
        m_documentList.Append(winrt::make<DocumentInfo>(L"Document 2", L"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."));
        m_documentList.Append(winrt::make<DocumentInfo>(L"Document 3", L"Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat."));
    }

    void MainPage::Init(winrt::com_array<IInspectable> const& documentList)
    {
        if (!documentList.empty())
        {
            m_documentList.Clear();

            for (IInspectable documentInfo : documentList)
            {
                m_documentList.Append(documentInfo);
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
            if (m_documentList.IndexOf(item, index))
            {
                m_documentList.RemoveAt(index);
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
            if (m_documentList.IndexOf(items[i], index))
            {
                m_documentList.RemoveAt(index);
            }
        }

        for (uint32_t i = 0; i < items.size(); i++)
        {
            m_documentList.InsertAt(args.DropIndex() + i, items[i]);
        }
    }

    void MainPage::OnAddTabButtonClick(TabView const& sender, IInspectable const& args)
    {
        UNREFERENCED_PARAMETER(args);

        m_documentList.Append(winrt::make<DocumentInfo>(L"New Document", L""));
        sender.SelectedIndex(m_documentList.Size() - 1);
    }

    void MainPage::OnTabCloseRequested(TabView const& sender, TabViewTabCloseRequestedEventArgs const& args)
    {
        UNREFERENCED_PARAMETER(sender);

        uint32_t index;
        if (m_documentList.IndexOf(args.Item(), index))
        {
            m_documentList.RemoveAt(index);
        }
    }
}
