module;
#include <windows.h>
#include <unknwn.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <winrt/base_macros.h>
#include "MainPage.h"
#undef GetCurrentTime
export module Simple.HandwrittenMainPage;
#ifndef WINRT_IMPORT_MODULE
#define WINRT_IMPORT_MODULE
#endif
import winrt.Windows.Foundation;
import winrt.Windows.Foundation.Collections;
import winrt.Microsoft.UI.Xaml;
import winrt.Microsoft.UI.Xaml.Controls;
import winrt.Microsoft.UI.Xaml.Controls.Primitives;
import winrt.Microsoft.UI.Xaml.Data;
import winrt.Microsoft.UI.Xaml.Documents;
import winrt.Microsoft.UI.Xaml.Input;
import winrt.Microsoft.UI.Xaml.Interop;
import winrt.Microsoft.UI.Xaml.Markup;
import winrt.Simple;
import winrt.Simple.Models;
// MainPage.h is textual and therefore belongs to the global module fragment above.
export extern "C++" namespace winrt::Simple::implementation
{
    struct MainPage;
}
// Merge the automatic declarations only after the textual handwritten declarations.
export import Simple.Application_Xaml;
