// Direct consumer of the public XAML declaration aggregator.
import XamlCppWinRTModulesSample.Application_Xaml;

static_assert(sizeof(
    winrt::XamlCppWinRTModulesSample::implementation::MainWindowT<struct ModuleSmokeMainWindow>) > 0);
