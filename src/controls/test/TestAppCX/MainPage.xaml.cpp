// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "LeakCycleTestCX.xaml.h"
#include "MenuBarTestPage.xaml.h"
#include "CornerRadiusTestPage.xaml.h"
#include "TreeViewTestPage.xaml.h"
#include "BackdropMaterialTestPage.xaml.h"
#ifdef WEBVIEW2_INCLUDED
#include "WebView2TestPage.xaml.h"
#endif

using namespace TestAppCX;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Microsoft::UI::Xaml::Interop;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

MainPage::MainPage()
{
    InitializeComponent();
}

void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    auto coreTitleBar = Windows::ApplicationModel::Core::CoreApplication::GetCurrentView()->TitleBar;
    coreTitleBar->ExtendViewIntoTitleBar = false;
}

void TestAppCX::MainPage::GoToLeakTestControlPage(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    auto app = dynamic_cast<App^>(Application::Current);
    app->RootFrame->Navigate(TypeName(LeakCycleTestCX::typeid), nullptr);

}

void TestAppCX::MainPage::GoToMenuBarTestPage(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    auto app = dynamic_cast<App^>(Application::Current);
    app->RootFrame->Navigate(TypeName(MenuBarTestPage::typeid), nullptr);
}

void TestAppCX::MainPage::GoToCornerRadiusTestPage(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    auto app = dynamic_cast<App^>(Application::Current);
    app->RootFrame->Navigate(TypeName(CornerRadiusTestPage::typeid), nullptr);
}

void TestAppCX::MainPage::GoToTreeViewTestPage(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    auto app = dynamic_cast<App^>(Application::Current);
    app->RootFrame->Navigate(TypeName(TreeViewTestPage::typeid), nullptr);
}

void TestAppCX::MainPage::GoToBackdropMaterialTestPage(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    auto app = dynamic_cast<App^>(Application::Current);
    app->RootFrame->Navigate(TypeName(BackdropMaterialTestPage::typeid), nullptr);
}

void TestAppCX::MainPage::GoToWebView2TestPage(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    auto app = dynamic_cast<App^>(Application::Current);
#ifdef WEBVIEW2_INCLUDED
    app->RootFrame->Navigate(TypeName(WebView2TestPage::typeid), nullptr);
#endif
}
