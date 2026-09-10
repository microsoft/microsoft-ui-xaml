#include "pch.h"
#include "App.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

std::wstring g_launchMarker;

// This sample defines DISABLE_XAML_GENERATED_MAIN (see pch.h) AND intentionally omits a
// parameterless App constructor. The developer supplies their own entry point below and
// constructs the App explicitly using its parameterized constructor.
//
// The XamlCompiler still generates a wXamlGeneratedMain() helper, but because the App has
// no parameterless constructor it must NOT emit the make<App>() call there (otherwise this
// project would fail to compile). This project exists to verify that behavior.
int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    Application::Start([](auto&&)
    {
        ::winrt::make<::winrt::DisableXamlGeneratedMainNoCtorCpp::implementation::App>(42);
    });
    return 0;
}