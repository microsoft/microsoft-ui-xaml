// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.h"
#include "MainWindow.g.cpp"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::NativeDesktopSample::implementation
{
    MainWindow::MainWindow()
    {
        m_text = L"Edit this text in the TextBox to test x:Bind";
        InitializeComponent();
    }

    int32_t MainWindow::MyProperty()
    {
        return 400;
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    hstring MainWindow::Text()
    {
        return m_text;
    }

    void MainWindow::Text(hstring const& value)
    {
        if (m_text != value)
        {
            m_text = value;
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"Text" });
        }
        m_text = value;
    }

    winrt::event_token MainWindow::PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }
    void MainWindow::PropertyChanged(winrt::event_token const& token) noexcept
    {
        m_propertyChanged.remove(token);
    }

    void MainWindow::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));
        Text(L"Clicked in TextBox");
    }
}
