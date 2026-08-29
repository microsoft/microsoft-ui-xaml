// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WuxLoadedTestPoolFilter.h"
#include "XamlTailored.h"
#include "IXamlTestHooks-win.h"
#include "WindowHelper.h"

using namespace Private::Infrastructure;
using namespace Microsoft::UI::Xaml::Tests::Common; 
using namespace WEX::Common;
using namespace WEX::Logging;

bool WuxLoadedTestPoolFilter::IsDirty()
{
    bool wuxLoaded = GetModuleHandle(L"Windows.UI.Xaml.dll") != nullptr;
    bool wuxpLoaded = GetModuleHandle(L"Windows.UI.Xaml.Phone.dll") != nullptr;
    bool wuxcLoaded = GetModuleHandle(L"Windows.UI.Xaml.Controls.dll") != nullptr;
    bool msftEditLoaded = GetModuleHandle(L"msftedit.dll") != nullptr;
    
    if (wuxLoaded)
    {
        Log::Error(L"The test caused Windows.UI.Xaml.dll to be loaded! WinUI 3 tests should only load Microsoft.UI.Xaml.dll.");
    }
    
    if (wuxpLoaded)
    {
        Log::Error(L"The test caused Windows.UI.Xaml.Phone.dll to be loaded! WinUI 3 tests should only load Microsoft.UI.Xaml.Phone.dll.");
    }
    
    if (wuxcLoaded)
    {
        Log::Error(L"The test caused Windows.UI.Xaml.Controls.dll to be loaded! WinUI 3 tests should only load Microsoft.UI.Xaml.Controls.dll.");
    }
    
    if (msftEditLoaded)
    {
        Log::Error(L"The test caused msftedit.dll to be loaded! WinUI 3 tests should only load WinUIEdit.dll.");
    }
    
    return wuxLoaded || wuxpLoaded || wuxcLoaded || msftEditLoaded;
}