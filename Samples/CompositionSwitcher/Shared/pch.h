#pragma once

#include <windows.h>
#include <unknwn.h>
#include <inspectable.h>
#include <roapi.h>
#include <restrictederrorinfo.h>
#include <hstring.h>

#include <chrono>
#include <string>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>

#define DISABLE_XAML_GENERATED_MAIN

#ifndef USE_SYSTEM_COMPOSITOR
#define USE_SYSTEM_COMPOSITOR 0
#endif

extern std::wstring g_engineStatus;
