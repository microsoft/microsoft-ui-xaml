#ifdef XAML_FIXTURE_HEADER_MODE
#include <winrt/StaticControlsModuleLib.h>
static_assert(sizeof(winrt::StaticControlsModuleLib::BlankUserControl) > 0);
#else
import StaticControlsModuleLib.Application_Xaml;
static_assert(sizeof(winrt::StaticControlsModuleLib::implementation::XamlBindings) > 0);
#endif
