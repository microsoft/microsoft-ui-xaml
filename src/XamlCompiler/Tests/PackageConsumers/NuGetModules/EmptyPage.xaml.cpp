#include <windows.h>
#include <cstdint>

#if defined(NUGETMODULES_USE_MODULES)
#define WINRT_IMPORT_MODULE
import winrt_base;
import NuGetModules.Application_Xaml.EmptyPage;
#endif

#include "EmptyPage.xaml.h"
#if __has_include("EmptyPage.g.cpp")
#include "EmptyPage.g.cpp"
#endif
