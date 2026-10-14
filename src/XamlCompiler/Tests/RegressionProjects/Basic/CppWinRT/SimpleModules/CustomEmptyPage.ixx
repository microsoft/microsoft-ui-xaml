module;
#include <unknwn.h>
#include <winrt/base_macros.h>
#undef GetCurrentTime
export module Simple.CustomEmptyPage;
import std;
export import winrt.Windows.Foundation;
export import winrt.Microsoft.UI.Xaml;
export import winrt.Microsoft.UI.Xaml.Controls;
export import winrt.Microsoft.UI.Xaml.Markup;
#define XAML_IMPL_MODULE
#include "EmptyPage.xaml.g.h"
#undef XAML_IMPL_MODULE
