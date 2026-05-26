// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FirstTestPage.h"

using namespace Microsoft::UI::Xaml;
using namespace Tests::Native::External::Controls::Frame;

Microsoft::UI::Xaml::Navigation::NavigationCacheMode FirstTestPage::m_CacheMode = Microsoft::UI::Xaml::Navigation::NavigationCacheMode::Disabled;
int FirstTestPage::m_Counter = 0;

FirstTestPage::FirstTestPage()
{
    NavigationCacheMode = CacheMode;
    InstanceCounter = Counter;
}
