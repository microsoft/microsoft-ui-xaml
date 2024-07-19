﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TreeViewTestPage.xaml.h"

using namespace TestAppCX;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

TreeViewTestPage::TreeViewTestPage()
{
    InitializeComponent();

    TreeViewData^ d1 = ref new TreeViewData("111");
    TreeViewData^ d2 = ref new TreeViewData("222");
    TreeViewData^ d3 = ref new TreeViewData("333");
    Items->Append(d1);
    Items->Append(d2);
    Items->Append(d3);
}


void TestAppCX::TreeViewTestPage::ReplaceAll_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    auto newItems = ref new Platform::Array<TreeViewData^>(2);
    newItems[0] = ref new TreeViewData("444");
    newItems[0]->Children->Append(ref new TreeViewData("123"));
    newItems[1] = ref new TreeViewData("555");
    Items->ReplaceAll(newItems);
}


void TestAppCX::TreeViewTestPage::Clear_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
    Items->Clear();
}
