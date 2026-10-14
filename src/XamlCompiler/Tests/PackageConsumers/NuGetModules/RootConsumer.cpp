#if defined(XAML_IMPL_MODULE)
#error Ordinary consumers must not receive XamlC producer macros.
#endif
#define WINRT_IMPORT_MODULE
import winrt_base;
import NuGetModules.Application_Xaml;
template <typename D> using MainPageScaffold = winrt::NuGetModules::implementation::MainPageT<D>;
template <typename D> using EmptyPageScaffold = winrt::NuGetModules::implementation::EmptyPageT<D>;
template <typename D> using AppScaffold = winrt::NuGetModules::implementation::AppT<D>;
