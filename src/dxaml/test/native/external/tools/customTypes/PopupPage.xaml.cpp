// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PageWithTemplatedParentBinding.xaml.h"
#include "PopupPage.xaml.h"

using namespace ::Tests::Tools::Shared;

PopupPage::PopupPage()
{
    // Unlike other test pages, we are expecting this one to be navigated to via Frame.Navigate to best simulate
    // an actual app. So this constructor should call LoadComponent. Note we don't call InitializeComponent because
    // the test page won't be in the expected location
    auto xamlUri = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/PopupPage.xaml");
    Microsoft::UI::Xaml::Application::LoadComponent(
        this,
        xamlUri,
        Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);
}
