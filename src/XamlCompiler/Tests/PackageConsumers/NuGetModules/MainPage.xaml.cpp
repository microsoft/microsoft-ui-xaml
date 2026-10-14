#include <windows.h>
#include <cstdint>

#if defined(NUGETMODULES_USE_MODULES)
#define WINRT_IMPORT_MODULE
import winrt_base;
import NuGetModules.Application_Xaml.MainPage;
#endif

#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif
