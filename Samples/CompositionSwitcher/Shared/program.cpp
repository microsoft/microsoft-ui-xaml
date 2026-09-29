#include "pch.h"
#include "App.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

std::wstring g_engineStatus;

#if USE_SYSTEM_COMPOSITOR
namespace
{
    struct __declspec(uuid("6CCF1385-C6C7-54D8-ACA1-E7D66CFEE704"))
        ICompositionEngineStatics : ::IInspectable
    {
        virtual HRESULT __stdcall TrySetProcessEngine(
            int32_t requested,
            boolean* result) noexcept = 0;
        virtual HRESULT __stdcall GetForSystemEngine(
            ::IInspectable* compositionObject,
            ::IInspectable** result) noexcept = 0;
        virtual HRESULT __stdcall GetForInProcessEngine(
            ::IInspectable* compositionObject,
            ::IInspectable** result) noexcept = 0;
    };

    bool TrySelectSystemCompositionEngine()
    {
        com_ptr<ICompositionEngineStatics> statics;
        hstring className(L"Microsoft.UI.Composition.CompositionEngine");
        const HRESULT activationResult = RoGetActivationFactory(
            static_cast<HSTRING>(get_abi(className)),
            __uuidof(ICompositionEngineStatics),
            statics.put_void());
        if (activationResult == REGDB_E_CLASSNOTREG)
        {
            return false;
        }
        check_hresult(activationResult);

        boolean selected{};
        check_hresult(statics->TrySetProcessEngine(0, &selected));
        return selected;
    }
}
#endif

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    try
    {
        init_apartment(apartment_type::single_threaded);

#if USE_SYSTEM_COMPOSITOR
        if (!TrySelectSystemCompositionEngine())
        {
            MessageBoxW(
                nullptr,
                L"TrySetProcessEngine(System) returned false. Enable the OS switcher feature and LAF, then retry.",
                L"Composition switcher unavailable",
                MB_OK | MB_ICONERROR);
            return ERROR_NOT_SUPPORTED;
        }
        g_engineStatus = L"Switcher enabled: system composition engine";
#else
        g_engineStatus = L"Switcher disabled: default lifted composition engine";
#endif

        Application::Start([](auto&&)
        {
            make<CompositionSwitcherSample::implementation::App>();
        });
        return 0;
    }
    catch (hresult_error const& error)
    {
        MessageBoxW(nullptr, error.message().c_str(), L"Composition sample failed", MB_OK | MB_ICONERROR);
        return static_cast<int>(error.code());
    }
}
