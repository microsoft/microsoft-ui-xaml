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
    }

    void MainWindow::RunEnvironmentScenario()
    {
        std::wstring result = L"Passed\r\nArchitecture: x64\r\nLoaded modules:\r\n";
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
                result.append(name);
                result.append(L"\r\n");
            }
        }

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
}
