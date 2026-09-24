// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "subfolder/SubDictionary.h"
#include "subfolder/SubDictionary.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::subfolder::implementation
{
    SubDictionary::SubDictionary()
    {
        InitializeComponent();
        BindTestbed::implementation::DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::subfolder::SubDictionary>().Name);
    }
}