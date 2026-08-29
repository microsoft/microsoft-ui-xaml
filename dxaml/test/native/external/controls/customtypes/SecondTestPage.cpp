// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SecondTestPage.h"

using namespace Microsoft::UI::Xaml;
using namespace Tests::Native::External::Controls::Frame;

Microsoft::UI::Xaml::Navigation::NavigationCacheMode SecondTestPage::m_CacheMode = Microsoft::UI::Xaml::Navigation::NavigationCacheMode::Disabled;
int SecondTestPage::m_Counter = 0;

SecondTestPage::SecondTestPage()
{
    NavigationCacheMode = CacheMode;
    InstanceCounter = Counter;
}
