// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SubDictionary.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace winrt::BindTestbed::implementation;
using namespace winrt::BindTestbed::subfolder::implementation;

SubDictionary::SubDictionary()
{
    // TODO: cannot call this until CppWinRT creates subfolders for subnamespaces
    // InitializeComponent();
    //::winrt::BindTestbed::implementation::DetectLeaksPage::TrackObject(*this, ::winrt::xaml_typename<::winrt::BindTestbed::SubDictionary>().Name);
}