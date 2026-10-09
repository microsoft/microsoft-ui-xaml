// Direct consumer of the public XAML module contract.
//
// Normal App/MainWindow sources intentionally do not import this module directly;
// their generated C++/WinRT component headers discover the corresponding
// *.xaml.g.h companion and import the umbrella automatically.
#define WINRT_IMPORT_MODULE
import XamlCppWinRTModulesSample.Application_Xaml;

static_assert(sizeof(winrt::XamlCppWinRTModulesSample::implementation::XamlBindings) > 0);
