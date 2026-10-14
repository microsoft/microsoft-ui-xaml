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
#undef GetCurrentTime
#undef WINRT_IMPORT_MODULE
#include "MainPage.h"
#include "EmptyPage.h"
#include "App.h"
#include "Views/PanelPage.h"
#include "Controls/PanelPage.h"
import Simple.Application_Xaml;
import winrt.Simple;
import winrt.Microsoft.UI.Xaml;
static_assert(sizeof(winrt::Simple::implementation::MainPage) > 0);
static_assert(std::is_base_of_v<winrt::Simple::implementation::MainPageT<winrt::Simple::implementation::MainPage>, winrt::Simple::implementation::MainPage>);
static_assert(sizeof(winrt::Simple::implementation::EmptyPage) > 0);
static_assert(sizeof(winrt::Simple::Views::implementation::PanelPage) > 0);
static_assert(sizeof(winrt::Simple::Controls::implementation::PanelPage) > 0);
