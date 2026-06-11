// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThirdTestPage.h"

using namespace Microsoft::UI::Xaml;
using namespace Tests::Native::External::Controls::Frame;

Microsoft::UI::Xaml::Navigation::NavigationCacheMode ThirdTestPage::m_CacheMode = Microsoft::UI::Xaml::Navigation::NavigationCacheMode::Disabled;
int ThirdTestPage::m_Counter = 0;

ThirdTestPage::ThirdTestPage()
{
    NavigationCacheMode = CacheMode;
    InstanceCounter = Counter;
}
