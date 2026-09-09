#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include <Psapi.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;

using PublicXamlCompositionTypes = std::tuple<
    Microsoft::UI::Composition::AnimationPropertyInfo,
    Microsoft::UI::Composition::CompositionBrush,
    Microsoft::UI::Composition::CompositionEasingFunction,
    Microsoft::UI::Composition::CompositionLight,
    Microsoft::UI::Composition::CompositionPropertySet,
    Microsoft::UI::Composition::Compositor,
    Microsoft::UI::Composition::IAnimationObject,
    Microsoft::UI::Composition::ICompositionAnimationBase,
    Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop,
    Microsoft::UI::Composition::ICompositionSurface,
    Microsoft::UI::Composition::IVisualElement,
    Microsoft::UI::Composition::IVisualElement2,
    Microsoft::UI::Composition::Visual,
    Microsoft::UI::Composition::SystemBackdrops::SystemBackdropConfiguration>;

static_assert(std::tuple_size_v<PublicXamlCompositionTypes> == 14);

namespace winrt::SystemComponentExperiment::Cpp::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        ScenarioList().SelectedIndex(0);
    }

    void MainWindow::ScenarioList_SelectionChanged(
        IInspectable const&,
        SelectionChangedEventArgs const&)
    {
        if (auto item = ScenarioList().SelectedItem().try_as<ListViewItem>())
        {
            ScenarioTitle().Text(unbox_value<hstring>(item.Content()));
            ResultText().Text(L"Not run");
            VisualHost().Visibility(Visibility::Visible);
        }
    }

    void MainWindow::RunScenario_Click(
        IInspectable const&,
        RoutedEventArgs const&)
    {
        auto item = ScenarioList().SelectedItem().try_as<ListViewItem>();
        if (!item)
        {
            ResultText().Text(L"Failed: no scenario selected.");
            return;
        }

        auto id = unbox_value<hstring>(item.Tag());
        if (id == L"environment.startup")
        {
            RunEnvironmentScenario();
        }
        else if (id == L"composition.basics")
        {
            RunCompositionScenario();
        }
        else if (id == L"dispatching.basics")
        {
            RunDispatcherQueueScenario();
        }
        else if (id == L"closure.probes")
        {
            RunClosureProbeScenario();
        }
    }

    void MainWindow::RunEnvironmentScenario()
    {
        static constexpr std::array forbiddenModules
        {
            L"CoreMessagingXP.dll",
            L"dcompi.dll",
            L"DwmSceneI.dll",
            L"dwmcorei.dll",
            L"marshal.dll",
            L"Microsoft.UI.Composition.OSSupport.dll",
            L"wuceffectsi.dll",
        };

        std::wstring loadedModules;
        std::wstring forbiddenLoadedModules;
        HMODULE modules[1024]{};
        DWORD needed{};

        if (!EnumProcessModules(
                GetCurrentProcess(),
                modules,
                static_cast<DWORD>(sizeof(modules)),
                &needed))
        {
            ResultText().Text(L"Failed: EnumProcessModules failed.");
            return;
        }

        auto count = std::min<DWORD>(
            needed / sizeof(HMODULE),
            static_cast<DWORD>(std::size(modules)));

        for (DWORD index = 0; index < count; ++index)
        {
            wchar_t name[MAX_PATH]{};
            if (GetModuleBaseNameW(GetCurrentProcess(), modules[index], name, MAX_PATH))
            {
                loadedModules.append(name);
                loadedModules.append(L"\r\n");
                if (std::any_of(
                        forbiddenModules.begin(),
                        forbiddenModules.end(),
                        [&name](auto forbidden)
                        {
                            return _wcsicmp(name, forbidden) == 0;
                        }))
                {
                    forbiddenLoadedModules.append(name);
                    forbiddenLoadedModules.append(L"\r\n");
                }
            }
        }

        std::wstring result = forbiddenLoadedModules.empty() ? L"Passed\r\n" : L"Failed\r\n";
        result.append(L"Architecture: x64\r\nForbidden modules: ");
        if (forbiddenLoadedModules.empty())
        {
            result.append(L"none\r\n");
        }
        else
        {
            result.append(L"\r\n");
            result.append(forbiddenLoadedModules);
        }
        result.append(L"Loaded modules:\r\n");
        result.append(loadedModules);
        ResultText().Text(result);
    }

    void MainWindow::RunCompositionScenario()
    {
        try
        {
            Microsoft::UI::Composition::Visual visual =
                ElementCompositionPreview::GetElementVisual(VisualHost());
            Microsoft::UI::Composition::Compositor compositor = visual.Compositor();
            Microsoft::UI::Composition::SpriteVisual child = compositor.CreateSpriteVisual();
            child.Size({ 120.0f, 120.0f });
            child.Offset({ 20.0f, 20.0f, 0.0f });
            child.Brush(compositor.CreateColorBrush(Microsoft::UI::Colors::CornflowerBlue()));
            ElementCompositionPreview::SetElementChildVisual(VisualHost(), child);
            ResultText().Text(L"Passed");
        }
        catch (hresult_error const& error)
        {
            ResultText().Text(L"Failed: " + error.message());
        }
    }

    void MainWindow::RunDispatcherQueueScenario()
    {
        Microsoft::UI::Dispatching::DispatcherQueue queue =
            Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        if (!queue)
        {
            ResultText().Text(L"Failed: no DispatcherQueue for the UI thread.");
            return;
        }

        if (!queue.TryEnqueue([this]()
            {
                ResultText().Text(L"Passed");
            }))
        {
            ResultText().Text(L"Failed: TryEnqueue returned false.");
        }
    }

    void MainWindow::RunClosureProbeScenario()
    {
        using Windows::Foundation::Metadata::ApiInformation;

        static constexpr std::array systemTypeNames
        {
            L"Windows.UI.Composition.AnimationPropertyInfo",
            L"Windows.UI.Composition.CompositionBrush",
            L"Windows.UI.Composition.CompositionEasingFunction",
            L"Windows.UI.Composition.CompositionLight",
            L"Windows.UI.Composition.CompositionPropertySet",
            L"Windows.UI.Composition.Compositor",
            L"Windows.UI.Composition.IAnimationObject",
            L"Windows.UI.Composition.ICompositionAnimationBase",
            L"Windows.UI.Composition.ICompositionSupportsSystemBackdrop",
            L"Windows.UI.Composition.ICompositionSurface",
            L"Windows.UI.Composition.IVisualElement",
            L"Windows.UI.Composition.IVisualElement2",
            L"Windows.UI.Composition.Visual",
            L"Windows.System.DispatcherQueue",
        };

        std::wstring result = L"Compile-probed system projection types: Compositor, Visual, DispatcherQueue\r\n";
        for (auto const* typeName : systemTypeNames)
        {
            result.append(typeName);
            result.append(ApiInformation::IsTypePresent(typeName) ? L": present\r\n" : L": absent\r\n");
        }

        try
        {
            Windows::UI::Composition::Compositor systemCompositor;
            auto systemVisual = systemCompositor.CreateSpriteVisual();

            constexpr GUID experimentalPropertyChanged =
            {
                0x12b579a9,
                0x6a27,
                0x5cde,
                { 0xa2, 0xa1, 0xc5, 0x57, 0xbb, 0x7d, 0xfd, 0xb3 }
            };
            void* experimentalInterface{};
            HRESULT queryResult = systemVisual.as<::IUnknown>()->QueryInterface(
                experimentalPropertyChanged,
                &experimentalInterface);
            if (experimentalInterface)
            {
                static_cast<::IUnknown*>(experimentalInterface)->Release();
            }

            wchar_t queryText[96]{};
            swprintf_s(
                queryText,
                L"System visual lifted property-change QI: 0x%08X\r\n",
                static_cast<unsigned int>(queryResult));
            result.append(queryText);
            result.append(L"System Compositor activation: passed\r\n");
        }
        catch (hresult_error const& error)
        {
            wchar_t activationText[96]{};
            swprintf_s(
                activationText,
                L"System Compositor activation: failed (0x%08X)\r\n",
                static_cast<unsigned int>(error.code().value));
            result.append(activationText);
        }

        auto queue = Windows::System::DispatcherQueue::GetForCurrentThread();
        result.append(queue
            ? L"System DispatcherQueue current thread: present\r\n"
            : L"System DispatcherQueue current thread: absent\r\n");
        if (queue)
        {
            void* liftedQueue3{};
            HRESULT queryResult = queue.as<::IUnknown>()->QueryInterface(
                guid_of<Microsoft::UI::Dispatching::IDispatcherQueue3>(),
                &liftedQueue3);
            if (liftedQueue3)
            {
                static_cast<::IUnknown*>(liftedQueue3)->Release();
            }

            wchar_t queryText[96]{};
            swprintf_s(
                queryText,
                L"System queue lifted IDispatcherQueue3 QI: 0x%08X\r\n",
                static_cast<unsigned int>(queryResult));
            result.append(queryText);
        }

        ResultText().Text(result);
    }
}
